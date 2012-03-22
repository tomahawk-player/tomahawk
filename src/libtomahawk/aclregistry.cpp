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
    //qRegisterMetaType< QHash< QString, QHash< QString, ACL > > >("ACLRegistry::ACLCacheHash");

    m_cache = TomahawkSettings::instance()->aclEntries();
}


ACLRegistry::~ACLRegistry()
{
}


ACLRegistry::ACL
ACLRegistry::isAuthorizedPeer( const QString& dbid, ACLRegistry::ACL globalType, const QString &username )
{
    if ( QThread::currentThread() != TOMAHAWK_APPLICATION::instance()->thread() )
        return globalType;
    
    qDebug() << "Current cache keys =" << m_cache.keys();
    if( m_cache.contains( dbid ) )
    {
        QVariantHash peerHash = m_cache[ dbid ].toHash();
        if( peerHash.contains( "global" ) )
        {
            registerAlias( dbid, username );
            return ACLRegistry::ACL( peerHash[ "global" ].toInt() );
        }

        if ( globalType == ACLRegistry::NotFound )
            return globalType;

        peerHash[ "global" ] = int( globalType );
        m_cache[ dbid ] = peerHash;
        save();
        registerAlias( dbid, username );
        return globalType;
    }

    ACLRegistry::ACL acl = globalType;
    tDebug( LOGVERBOSE ) << "ACL is intially" << acl;
#ifndef ENABLE_HEADLESS
    acl = getUserDecision( username );
    tDebug( LOGVERBOSE ) << "after getUserDecision acl is" << acl;
#endif
    
    if ( acl == ACLRegistry::NotFound || acl == ACLRegistry::AllowOnce || acl == ACLRegistry::DenyOnce )
        return acl;

    QVariantHash peerHash;
    peerHash[ "global" ] = int( acl );
    m_cache[ dbid ] = peerHash;
    save();
    registerAlias( dbid, username );
    return acl;
}


void
ACLRegistry::registerPeer( const QString& dbid, ACLRegistry::ACL globalType, const QString &username )
{
    if ( QThread::currentThread() != TOMAHAWK_APPLICATION::instance()->thread() )
        return;
    
    if ( globalType == ACLRegistry::NotFound || globalType == ACLRegistry::DenyOnce || globalType == ACLRegistry::AllowOnce )
        return;

    QVariantHash peerHash;
    if ( m_cache.contains( dbid ) )
        peerHash = m_cache[ dbid ].toHash();
    peerHash[ "global" ] = int( globalType );
    m_cache[ dbid ] = peerHash;
    save();
    registerAlias( dbid, username );
}


QPair< QString, ACLRegistry::ACL >
ACLRegistry::isAuthorizedUser( const QString &username, ACLRegistry::ACL globalType )
{
    if ( QThread::currentThread() != TOMAHAWK_APPLICATION::instance()->thread() )
        return QPair< QString, ACLRegistry::ACL >( QString(), ACLRegistry::NotFound );

    qDebug() << "Current cache keys =" << m_cache.keys();
    foreach ( QString dbid, m_cache.keys() )
    {
        //    qDebug() << "Looking up dbid";
        QVariantHash peerHash = m_cache[ dbid ].toHash();
        if ( !peerHash.contains( "usernames" ) )
            continue;
        
        if ( !peerHash[ "usernames" ].toStringList().contains( username ) )
            continue;
        
        if ( globalType != ACLRegistry::NotFound )
        {
            peerHash[ "global" ] = int( globalType );
            m_cache[ dbid ] = peerHash;
            save();
            return QPair< QString, ACLRegistry::ACL >( dbid, globalType );
        }
        
        return QPair< QString, ACLRegistry::ACL >( dbid, ACLRegistry::ACL( peerHash[ "global" ].toInt() ) );
    }

    return QPair< QString, ACLRegistry::ACL >( QString(), ACLRegistry::NotFound );
}


void
ACLRegistry::registerAlias( const QString& dbid, const QString &username )
{
    if ( QThread::currentThread() != TOMAHAWK_APPLICATION::instance()->thread() )
        return;

    if ( dbid.isEmpty() || username.isEmpty() )
        return;

    if ( !m_cache.contains( dbid ) )
        return;

    QVariantHash peerHash = m_cache[ dbid ].toHash();
    if ( !peerHash.contains( "usernames" ) )
        peerHash[ "usernames" ] = QStringList( username );
    else if ( !peerHash[ "usernames" ].toStringList().contains( username ) )
        peerHash[ "usernames" ] = peerHash[ "usernames" ].toStringList() + QStringList( username );
    else
        return;

    m_cache[ dbid ] = peerHash;
    save();
}


// ACLRegistry::ACL
// ACLRegistry::isAuthorizedPath( const QString& dbid, const QString& path )
// {
//     QMutexLocker locker( &m_cacheMutex );
// 
//     if( !m_cache.contains( dbid ) )
//         return ACLRegistry::NotFound;
// 
//     QHash< QString, ACL > peerHash = m_cache[dbid];
//     if( !peerHash.contains( path ) )
//     {
//         if( peerHash.contains( "global" ) )
//             return peerHash["global"];
//         else
//             return ACLRegistry::Deny;
//     }
//     return peerHash[path];
// }
// 
// void
// ACLRegistry::authorizePath( const QString& dbid, const QString& path, ACLRegistry::ACL type )
// {
//     TomahawkSettings *s = TomahawkSettings::instance();
//     if( !s->scannerPaths().contains( path ) )
//     {
//         qDebug() << "path selected is not in our scanner path!";
//         return;
//     }
//     QMutexLocker locker( &m_cacheMutex );
//     QHash< QString, ACLRegistry::ACL > peerHash;
//     if ( m_cache.contains( dbid ) )
//         peerHash = m_cache[dbid];
//     peerHash[path] = type;
//     m_cache[dbid] = peerHash;
// }

#ifndef ENABLE_HEADLESS

#include <QMessageBox>

ACLRegistry::ACL
ACLRegistry::getUserDecision( const QString &username )
{
    QMessageBox msgBox;
    msgBox.setIcon( QMessageBox::Question );
    msgBox.setText( tr( "Connect to Peer?" ) );
    msgBox.setInformativeText( tr( "Another Tomahawk instance that claims to be owned by %1 is attempting to connect to you. Select whether to allow or deny this connection.\n\nRemember: Only allow peers to connect if you trust who they are and if you have the legal right for them to stream music from you.").arg( username ) );
    QPushButton *denyButton = msgBox.addButton( tr( "Deny" ), QMessageBox::HelpRole );
    QPushButton *alwaysDenyButton = msgBox.addButton( tr( "Always Deny" ), QMessageBox::YesRole );
    QPushButton *allowButton = msgBox.addButton( tr( "Allow" ), QMessageBox::NoRole );
    QPushButton *alwaysAllowButton = msgBox.addButton( tr( "Always Allow" ), QMessageBox::ActionRole );

    msgBox.setDefaultButton( allowButton );
    msgBox.setEscapeButton( denyButton );

    msgBox.exec();

    if( msgBox.clickedButton() == denyButton )
        return ACLRegistry::DenyOnce;
    else if( msgBox.clickedButton() == alwaysDenyButton )
        return ACLRegistry::Deny;
    else if( msgBox.clickedButton() == allowButton )
        return ACLRegistry::AllowOnce;
    else if( msgBox.clickedButton() == alwaysAllowButton )
        return ACLRegistry::Allow;

    //How could we get here?
    tDebug( LOGVERBOSE ) << "ERROR: returning NotFound";
    Q_ASSERT( false );
    return ACLRegistry::NotFound;
}

#endif

void
ACLRegistry::save()
{
    TomahawkSettings::instance()->setAclEntries( m_cache );
}