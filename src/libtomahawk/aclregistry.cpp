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

#include "aclregistry.h"

#include <QThread>
#include <QVariant>

#include "tomahawksettings.h"
#include "tomahawkapp.h"

#include "utils/logger.h"


ACLRegistry* ACLRegistry::s_instance = 0;

ACLRegistry*
ACLRegistry::instance()
{
    if( !s_instance )
        new ACLRegistry();
    return s_instance;
}


ACLRegistry::ACLRegistry( QObject* parent )
    : QObject( parent )
{
    s_instance = this;
    qRegisterMetaType< ACLRegistry::ACL >( "ACLRegistry::ACL" );
    qRegisterMetaType< ACLRegistry::User >( "ACLRegistry::User" );

    load();
}


ACLRegistry::~ACLRegistry()
{
}


void
ACLRegistry::isAuthorizedUser( const QString& dbid, const QString &username, ACLRegistry::ACL globalType )
{
    if ( QThread::currentThread() != TOMAHAWK_APPLICATION::instance()->thread() )
    {
        emit aclResult( dbid, username, globalType );
        return;
    }

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
            emit aclResult( dbid, username, user.acl );
            i.setValue( user );
            return;
        }
    }

    // User was not found, create a new user entry
    ACLRegistry::User user;
    user.knownDbids.append( dbid );
    user.knownAccountIds.append( username );
    if ( globalType != ACLRegistry::NotFound )
        user.acl = globalType;
    else
    {    
        ACLRegistry::ACL acl = globalType;
        tDebug( LOGVERBOSE ) << "ACL is intially" << acl;
        #ifndef ENABLE_HEADLESS
            acl = getUserDecision( username );
            tDebug( LOGVERBOSE ) << "after getUserDecision acl is" << acl;
        #endif

        if ( acl == ACLRegistry::NotFound )
        {
            emit aclResult( dbid, username, acl );
            return;
        }

        user.acl = acl;
    }

    m_cache.append( user );
    emit aclResult( dbid, username, user.acl );
    return;
}


#ifndef ENABLE_HEADLESS

#include <QMessageBox>

ACLRegistry::ACL
ACLRegistry::getUserDecision( const QString &username )
{
    return ACLRegistry::Stream;
    QMessageBox msgBox;
    msgBox.setIcon( QMessageBox::Question );
    msgBox.setText( tr( "Connect to Peer?" ) );
    msgBox.setInformativeText( tr( "Another Tomahawk instance that claims to be owned by %1 is attempting to connect to you. Select whether to allow or deny this connection.\n\nRemember: Only allow peers to connect if you trust who they are and if you have the legal right for them to stream music from you.").arg( username ) );
    QPushButton *denyButton = msgBox.addButton( tr( "Deny" ), QMessageBox::YesRole );
    QPushButton *allowButton = msgBox.addButton( tr( "Allow" ), QMessageBox::ActionRole );

    msgBox.setDefaultButton( allowButton );
    msgBox.setEscapeButton( denyButton );

    msgBox.exec();

    if( msgBox.clickedButton() == denyButton )
        return ACLRegistry::Deny;
    else if( msgBox.clickedButton() == allowButton )
        return ACLRegistry::Stream;

    //How could we get here?
    tDebug( LOGVERBOSE ) << "ERROR: returning NotFound";
    Q_ASSERT( false );
    return ACLRegistry::NotFound;
}

#endif


void
ACLRegistry::load()
{
    QVariantList entryList = TomahawkSettings::instance()->aclEntries();
    foreach ( QVariant entry, entryList )
    {
        if ( !entry.isValid() || !entry.canConvert< ACLRegistry::User >() )
            continue;
        ACLRegistry::User entryUser = entry.value< ACLRegistry::User >();
        m_cache.append( entryUser );
    }
}


void
ACLRegistry::save()
{
    QVariantList entryList;
    foreach ( ACLRegistry::User user, m_cache )
        entryList.append( QVariant::fromValue< ACLRegistry::User >( user ) );
    TomahawkSettings::instance()->setAclEntries( entryList );
}