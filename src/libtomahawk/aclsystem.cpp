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
}

ACLSystem::~ACLSystem()
{
    //TODO: save from cache into settings file
}

void
ACLSystem::authorize( const QString& dbid, const QString& path, ACLType type )
{
    TomahawkSettings *s = TomahawkSettings::instance();
    if ( !s->scannerPath().contains( path ) )
    {
        qDebug() << "path selected is not in our scanner path!";
        return;
    }
    QHash< QString, ACLType > peerHash;
    if ( m_cache.contains( "dbid" ) )
        peerHash = m_cache["dbid"];
    peerHash[path] = type;
}

bool
ACLSystem::isAuthorized( const QString& dbid, const QString& path )
{
    if ( !m_cache.contains( "dbid" ) )
        return false;
    
    QHash< QString, ACLType > peerHash = m_cache["dbid"];
    if ( !peerHash.contains( path ) )
        return false;
    
    return peerHash[path] == ACLSystem::Allow;
}

void
ACLSystem::saveTimerFired()
{
    //TODO: save from cache into settings file
}