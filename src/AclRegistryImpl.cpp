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

#include <QThread>
#include <QVariant>


ACLRegistryImpl::ACLRegistryImpl( QObject* parent )
    : ACLRegistry( parent )
    , m_jobCount( 0 )
{
    ACLRegistry::setInstance( this );
    load();
}


ACLRegistryImpl::~ACLRegistryImpl()
{
    save();
}


ACLRegistry::ACL
ACLRegistryImpl::isAuthorizedUser( const QString& dbid, const QString &username, ACLRegistry::ACL globalType, bool skipEmission )
{
    if ( QThread::currentThread() != TOMAHAWK_APPLICATION::instance()->thread() )
    {
        if ( !skipEmission )
            QMetaObject::invokeMethod( this, "isAuthorizedUser", Qt::QueuedConnection, Q_ARG( const QString&, dbid ), Q_ARG( const QString &, username ), Q_ARG( ACLRegistry::ACL, globalType ), Q_ARG( bool, skipEmission ) );
        return ACLRegistry::NotFound;
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
                    emit aclResult( dbid, username, ACLRegistry::Stream );
                return ACLRegistry::Stream;
            }
        }
    }
#endif

    bool found = false;
    QMutableListIterator< ACLRegistry::User > i( m_cache );
    while ( i.hasNext() )
    {
        ACLRegistry::User user = i.next();
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
        return ACLRegistry::NotFound;

    // User was not found, create a new user entry
    ACLRegistry::User user;
    user.knownDbids.append( dbid );
    user.knownAccountIds.append( username );
    if ( globalType != ACLRegistry::NotFound )
        user.acl = globalType;
#ifdef ENABLE_HEADLESS
    user.acl = ACLRegistry::Stream;
#else
    if ( !TomahawkUtils::headless() )
    {
        getUserDecision( user, username );
        return ACLRegistry::NotFound;
    }
    else
        user.acl = ACLRegistry::Stream;
#endif
    m_cache.append( user );
    save();
    emit aclResult( dbid, username, user.acl );
    return user.acl;
}


#ifndef ENABLE_HEADLESS
void
ACLRegistryImpl::getUserDecision( ACLRegistry::User user, const QString &username )
{
    if ( TomahawkUtils::headless() )
        return;

    tDebug( LOGVERBOSE ) << Q_FUNC_INFO;
    ACLJobItem* job = new ACLJobItem( user, username );
    m_jobQueue.enqueue( job );
    QTimer::singleShot( 0, this, SLOT( queueNextJob() ) );
}


void
ACLRegistryImpl::userDecision( ACLRegistry::User user )
{
    if ( TomahawkUtils::headless() )
        return;

    tDebug( LOGVERBOSE ) << Q_FUNC_INFO;
    m_cache.append( user );
    save();
    emit aclResult( user.knownDbids.first(), user.knownAccountIds.first(), user.acl );

    m_jobCount--;
    if ( !m_jobQueue.isEmpty() )
        QTimer::singleShot( 0, this, SLOT( queueNextJob() ) );
}


void
ACLRegistryImpl::queueNextJob()
{
    if ( TomahawkUtils::headless() )
        return;

    if ( QThread::currentThread() != TOMAHAWK_APPLICATION::instance()->thread() )
    {
        QMetaObject::invokeMethod( this, "queueNextJob", Qt::QueuedConnection );
        return;
    }

    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "jobCount =" << m_jobCount;
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "jobQueue size =" << m_jobQueue.length();
    if ( m_jobCount != 0 )
        return;

    if ( !m_jobQueue.isEmpty() )
    {
        ACLJobItem* job = m_jobQueue.dequeue();
        ACLRegistry::User user = job->user();
        bool found = false;
        foreach( QString dbid, user.knownDbids )
        {
            ACLRegistry::ACL acl = isAuthorizedUser( dbid, job->username(), ACLRegistry::NotFound, true );
            if ( acl != ACLRegistry::NotFound )
            {
                tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Found existing acl entry for =" << user.knownAccountIds.first();
                found = true;
                break;
            }
        }
        if ( found )
        {
            tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "deleting job, already have ACL for" << user.knownAccountIds.first();
            delete job;
            QTimer::singleShot( 0, this, SLOT( queueNextJob() ) );
            return;
        }
        else
        {
            tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "activating job for user" << user.knownAccountIds.first();
            m_jobCount++;
            JobStatusView::instance()->model()->addJob( job );
            connect( job, SIGNAL( userDecision( ACLRegistry::User ) ), this, SLOT( userDecision( ACLRegistry::User ) ) );
        }
    }
}
#endif


void
ACLRegistryImpl::wipeEntries()
{
    ACLRegistry::wipeEntries();
    save();
}


void
ACLRegistryImpl::load()
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO;
    QVariantList entryList = TomahawkSettings::instance()->aclEntries();
    foreach ( QVariant entry, entryList )
    {
        if ( !entry.isValid() || !entry.canConvert< ACLRegistry::User >() )
        {
            tDebug() << Q_FUNC_INFO << "entry is invalid";
            continue;
        }
        ACLRegistry::User entryUser = entry.value< ACLRegistry::User >();
        if ( entryUser.knownAccountIds.empty() || entryUser.knownDbids.empty() )
        {
            tDebug() << Q_FUNC_INFO << "user known account/dbids is empty";
            continue;
        }
        m_cache.append( entryUser );
    }
}


void
ACLRegistryImpl::save()
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO;
    QVariantList entryList;
    foreach ( ACLRegistry::User user, m_cache )
    {
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "user is" << user.uuid << "with known name" << user.knownAccountIds.first();
        QVariant val = QVariant::fromValue< ACLRegistry::User >( user );
        if ( val.isValid() )
            entryList.append( val );
    }
    TomahawkSettings::instance()->setAclEntries( entryList );
}
