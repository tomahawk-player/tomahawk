/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include <QMutexLocker>
#include <QVariant>

#include "tomahawksettings.h"

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
ACLRegistry::isAuthorizedPeer( const QString& dbid, ACLRegistry::ACL globalType )
{
//    qDebug() << Q_FUNC_INFO;
    QMutexLocker locker( &m_cacheMutex );
    qDebug() << "Current cache keys =" << m_cache.keys();
//    qDebug() << "Looking up dbid";
    if( m_cache.contains( dbid ) )
    {
        QVariantHash peerHash = m_cache[ dbid ].toHash();
        if( peerHash.contains( "global" ) )
            return ACLRegistry::ACL( peerHash[ "global" ].toInt() );

        if ( globalType == ACLRegistry::NotFound )
            return globalType;

        peerHash[ "global" ] = int( globalType );
        m_cache[ dbid ] = peerHash;
        save();
        return globalType;
    }

    //not found
    if ( globalType == ACLRegistry::NotFound )
        return globalType;

    QVariantHash peerHash;
    peerHash[ "global" ] = int( globalType );
    m_cache[ dbid ] = peerHash;
    save();
    return globalType;
}


void
ACLRegistry::registerPeer( const QString& dbid, ACLRegistry::ACL globalType, const QString &username )
{
//    qDebug() << Q_FUNC_INFO;
    if( globalType == ACLRegistry::NotFound )
        return;

    QMutexLocker locker( &m_cacheMutex );

    QVariantHash peerHash;
    if( m_cache.contains( dbid ) )
        peerHash = m_cache[ dbid ].toHash();
    peerHash[ "global" ] = int( globalType );
    if ( !username.isEmpty() )
    {
        if ( peerHash.contains( "usernames" ) )
        {
            if ( !peerHash[ "usernames" ].toStringList().contains( username ) )
                peerHash[ "usernames" ] = peerHash[ "usernames" ].toStringList() + QStringList( username );
        }
        else
            peerHash[ "usernames" ] = QStringList( username );
    }
    m_cache[ dbid ] = peerHash;
    save();
}


QPair< QString, ACLRegistry::ACL >
ACLRegistry::isAuthorizedUser( const QString &username, ACLRegistry::ACL globalType )
{
//    qDebug() << Q_FUNC_INFO;
    QMutexLocker locker( &m_cacheMutex );
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


void
ACLRegistry::save()
{
    TomahawkSettings::instance()->setAclEntries( m_cache );
}