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

#include "aclsystem.h"

#include <QtDebug>
#include <QMutexLocker>
#include <QVariant>

#include <tomahawksettings.h>

ACLSystem* ACLSystem::s_instance = 0;

ACLSystem*
ACLSystem::instance()
{
    if( !s_instance )
        new ACLSystem();
    return s_instance;
}


ACLSystem::ACLSystem( QObject* parent )
    : QObject( parent ),
      m_saveTimer( this )
{
    s_instance = this;
    //qRegisterMetaType< QHash< QString, QHash< QString, ACL > > >("ACLSystem::ACLCacheHash");

    QStringList savedEntries = TomahawkSettings::instance()->aclEntries();
    if( !savedEntries.empty() && savedEntries.size() % 3 == 0 )
    {
        int index = 0;
        while( index < savedEntries.length() )
        {
            if( !m_cache.contains( savedEntries.at( index ) ) )
                m_cache[savedEntries.at( index ) ] = QHash< QString, ACL >();
            m_cache[savedEntries.at( index )][savedEntries.at( index + 1 )] = (ACL)(savedEntries.at( index + 2 ).toInt() );
            index += 3;
        }
    }

    m_saveTimer.setSingleShot( false );
    m_saveTimer.setInterval( 60000 );
    connect( &m_saveTimer, SIGNAL( timeout() ), this, SLOT( saveTimerFired() ) );
    m_saveTimer.start();
}

ACLSystem::~ACLSystem()
{
    m_saveTimer.stop();
    saveTimerFired();
}

ACLSystem::ACL
ACLSystem::isAuthorizedUser( const QString& dbid )
{
//    qDebug() << Q_FUNC_INFO;
    QMutexLocker locker( &m_cacheMutex );
    qDebug() << "Current cache keys =" << m_cache.keys();
//    qDebug() << "Looking up dbid";
    if( !m_cache.contains( dbid ) )
        return ACLSystem::NotFound;
    else
    {
        QHash< QString, ACL > peerHash = m_cache[dbid];
        if( peerHash.contains( "global" ) )
            return peerHash["global"];
        return ACLSystem::NotFound;
    }
}

void
ACLSystem::authorizeUser( const QString& dbid, ACLSystem::ACL globalType )
{
//    qDebug() << Q_FUNC_INFO;
    if( globalType == ACLSystem::NotFound )
        return;

    QMutexLocker locker( &m_cacheMutex );

    QHash< QString, ACL > peerHash;
    if( m_cache.contains( dbid ) )
        peerHash = m_cache[dbid];
    peerHash["global"] = globalType;
    m_cache[dbid] = peerHash;
}

ACLSystem::ACL
ACLSystem::isAuthorizedPath( const QString& dbid, const QString& path )
{
    QMutexLocker locker( &m_cacheMutex );

    if( !m_cache.contains( dbid ) )
        return ACLSystem::NotFound;

    QHash< QString, ACL > peerHash = m_cache[dbid];
    if( !peerHash.contains( path ) )
    {
        if( peerHash.contains( "global" ) )
            return peerHash["global"];
        else
            return ACLSystem::Deny;
    }
    return peerHash[path];
}

void
ACLSystem::authorizePath( const QString& dbid, const QString& path, ACLSystem::ACL type )
{
    TomahawkSettings *s = TomahawkSettings::instance();
    if( !s->scannerPaths().contains( path ) )
    {
        qDebug() << "path selected is not in our scanner path!";
        return;
    }
    QMutexLocker locker( &m_cacheMutex );
    QHash< QString, ACLSystem::ACL > peerHash;
    if ( m_cache.contains( dbid ) )
        peerHash = m_cache[dbid];
    peerHash[path] = type;
    m_cache[dbid] = peerHash;
}

void
ACLSystem::saveTimerFired()
{
    QStringList saveCache;
    foreach( QString dbid, m_cache.keys() )
    {
        foreach( QString path, m_cache[dbid].keys() )
            saveCache << dbid << path << QString::number( (int)(m_cache[dbid][path]) );
    }
    TomahawkSettings::instance()->setAclEntries( saveCache );
}