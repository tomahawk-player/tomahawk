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

#include <tomahawksettings.h>

ACLSystem::ACLSystem( QObject* parent )
    : QObject( parent ),
      m_saveTimer( this )
{
    //TODO: read from settings file into cache
    m_saveTimer.setSingleShot( false );
    m_saveTimer.setInterval( 60000 );
    connect( &m_saveTimer, SIGNAL( timeout() ), this, SLOT( saveTimerFired() ) );
    m_saveTimer.start();
}

ACLSystem::~ACLSystem()
{
    m_saveTimer.stop();
    //TODO: save from cache into settings file
}

ACLSystem::ACL
ACLSystem::isAuthorizedUser(const QString& dbid) const
{
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
    if( globalType == ACLSystem::NotFound )
        return;
    
    QHash< QString, ACL > peerHash;
    if( m_cache.contains( dbid ) )
        peerHash = m_cache[dbid];
    
    peerHash["global"] = globalType;
}

ACLSystem::ACL
ACLSystem::isAuthorizedPath( const QString& dbid, const QString& path ) const
{
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
    if( !s->scannerPath().contains( path ) )
    {
        qDebug() << "path selected is not in our scanner path!";
        return;
    }
    QHash< QString, ACLSystem::ACL > peerHash;
    if ( m_cache.contains( dbid ) )
        peerHash = m_cache[dbid];
    peerHash[path] = type;
    m_cache[dbid] = peerHash;
}

void
ACLSystem::saveTimerFired()
{
    //TODO: save from cache into settings file
}