/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Tomahawk is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */

#include "AclRegistryImpl.h"

#include <QThread>
#include <QVariant>

#include "TomahawkSettings.h"
#include "TomahawkApp.h"
#include "Source.h"

#ifndef ENABLE_HEADLESS
    #include "accounts/AccountManager.h"
    #include "accounts/Account.h"
    #include "jobview/AclJobItem.h"
    #include "jobview/JobStatusView.h"
    #include "jobview/JobStatusModel.h"
#endif

#include "utils/Logger.h"


AclRegistryImpl::AclRegistryImpl( QObject* parent )
    : AclRegistry( parent )
    , m_jobCount( 0 )
{
    AclRegistry::setInstance( this );
    load();
}


AclRegistryImpl::~AclRegistryImpl()
{
    save();
}


AclRegistry::ACL
AclRegistryImpl::isAuthorizedUser( const QString& dbid, const QString &username, AclRegistry::ACL globalType, bool skipEmission )
{
    tLog() << Q_FUNC_INFO;
    if ( QThread::currentThread() != TOMAHAWK_APPLICATION::instance()->thread() )
    {
        if ( !skipEmission )
            QMetaObject::invokeMethod( this, "isAuthorizedUser", Qt::QueuedConnection, Q_ARG( const QString&, dbid ), Q_ARG( const QString &, username ), Q_ARG( AclRegistry::ACL, globalType ), Q_ARG( bool, skipEmission ) );
        return AclRegistry::NotFound;
    }

#ifndef ENABLE_HEADLESS
    if ( Tomahawk::Accounts::AccountManager::instance() )
    {
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Checking account friendly names against" << username;
        Tomahawk::Accounts::AccountManager* accountManager = Tomahawk::Accounts::AccountManager::instance();
        QList< Tomahawk::Accounts::Account* > accounts = accountManager->accounts();
        foreach( Tomahawk::Accounts::Account* account, accounts )
        {
            if ( !( account->types() & Tomahawk::Accounts::SipType ) )
                continue;
            tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Checking against account friendly name" << account->accountFriendlyName();
            if ( account->accountFriendlyName() == username )
            {
                if ( !skipEmission )
                    emit aclResult( dbid, username, AclRegistry::Stream );
                return AclRegistry::Stream;
            }
        }
    }
#endif
    
    bool found = false;
    QMutableListIterator< AclRegistry::User > i( m_cache );
    while ( i.hasNext() )
    {
        AclRegistry::User user = i.next();
        foreach ( QString knowndbid, user.knownDbids )
        {
            if ( dbid == knowndbid )
            {
                if ( !user.knownAccountIds.contains( username ) )
                    user.knownAccountIds.append( username );
                found = true;
            }
        }

        foreach ( QString knownaccountid, user.knownAccountIds )
        {
            if ( username == knownaccountid )
            {
                if ( !user.knownDbids.contains( dbid ) )
                    user.knownDbids.append( dbid );
                found = true;
            }
        }

        if ( found )
        {
            if ( !skipEmission )
                emit aclResult( dbid, username, user.acl );
            i.setValue( user );
            return user.acl;
        }
    }

    if ( skipEmission )
        return AclRegistry::NotFound;

    // User was not found, create a new user entry
    AclRegistry::User user;
    user.knownDbids.append( dbid );
    user.knownAccountIds.append( username );
    if ( globalType != AclRegistry::NotFound )
        user.acl = globalType;
#ifdef ENABLE_HEADLESS
    user.acl = AclRegistry::Stream;
#else
    if ( !TomahawkUtils::headless() )
    {
        getUserDecision( user, username );
        return AclRegistry::NotFound;
    }
    else
        user.acl = AclRegistry::Stream;
#endif
    m_cache.append( user );
    save();
    emit aclResult( dbid, username, user.acl );
    return user.acl;
}


#ifndef ENABLE_HEADLESS
void
AclRegistryImpl::getUserDecision( AclRegistry::User user, const QString &username )
{
    if ( TomahawkUtils::headless() )
        return;

    tLog() << Q_FUNC_INFO;
    AclJobItem* job = new AclJobItem( user, username );
    m_jobQueue.enqueue( job );
    QTimer::singleShot( 0, this, SLOT( queueNextJob() ) );
}


void
AclRegistryImpl::userDecision( AclRegistry::User user )
{
    if ( TomahawkUtils::headless() )
        return;

    tLog() << Q_FUNC_INFO;
    m_cache.append( user );
    save();
    emit aclResult( user.knownDbids.first(), user.knownAccountIds.first(), user.acl );

    m_jobCount--;
    if ( !m_jobQueue.isEmpty() )
        QTimer::singleShot( 0, this, SLOT( queueNextJob() ) );
}


void
AclRegistryImpl::queueNextJob()
{
    if ( TomahawkUtils::headless() )
        return;

    if ( QThread::currentThread() != TOMAHAWK_APPLICATION::instance()->thread() )
    {
        QMetaObject::invokeMethod( this, "queueNextJob", Qt::QueuedConnection );
        return;
    }
    tLog() << Q_FUNC_INFO << "jobCount = " << m_jobCount;
    tLog() << Q_FUNC_INFO << "jobQueue size = " << m_jobQueue.length();
    if ( m_jobCount != 0 )
        return;

    if ( !m_jobQueue.isEmpty() )
    {
        AclJobItem* job = m_jobQueue.dequeue();
        AclRegistry::User user = job->user();
        bool found = false;
        foreach( QString dbid, user.knownDbids )
        {
            AclRegistry::ACL acl = isAuthorizedUser( dbid, job->username(), AclRegistry::NotFound, true );
            if ( acl != AclRegistry::NotFound )
            {
                tLog() << Q_FUNC_INFO << "Found existing acl entry for = " << user.knownAccountIds.first();
                found = true;
                break;
            }
        }
        if ( found )
        {
            tLog() << Q_FUNC_INFO << "deleting job, already have ACL for " << user.knownAccountIds.first();
            delete job;
            QTimer::singleShot( 0, this, SLOT( queueNextJob() ) );
            return;
        }
        else
        {
            tLog() << Q_FUNC_INFO << "activating job for user" << user.knownAccountIds.first();
            m_jobCount++;
            JobStatusView::instance()->model()->addJob( job );
            connect( job, SIGNAL( userDecision( AclRegistry::User ) ), this, SLOT( userDecision( AclRegistry::User ) ) );
        }
    }
}
#endif

void
AclRegistryImpl::wipeEntries()
{
    AclRegistry::wipeEntries();
    save();
}
