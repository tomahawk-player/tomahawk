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

#include "DatabaseCommand_DeleteDynamicPlaylist.h"

#include "playlist/dynamic/DynamicPlaylist.h"
#include "network/Servent.h"
#include "utils/Logger.h"

#include <QSqlQuery>

using namespace Tomahawk;


DatabaseCommand_DeleteDynamicPlaylist::DatabaseCommand_DeleteDynamicPlaylist( const source_ptr& source, const QString& playlistguid )
    : DatabaseCommand_DeletePlaylist( source, playlistguid )
{
}


void
DatabaseCommand_DeleteDynamicPlaylist::exec( DatabaseImpl* lib )
{
    qDebug() << Q_FUNC_INFO;
    qDebug() << "deleting dynamic playlist:" << m_playlistguid;
    DatabaseCommand_DeletePlaylist::exec( lib );
    TomahawkSqlQuery cre = lib->newquery();

    cre.prepare( "DELETE FROM dynamic_playlist WHERE guid = :id" );
    cre.bindValue( ":id", m_playlistguid );

    cre.exec();
}


void
DatabaseCommand_DeleteDynamicPlaylist::postCommitHook()
{
    qDebug() << Q_FUNC_INFO << "..reporting..:" << m_playlistguid;
    if ( source().isNull() || source()->dbCollection().isNull() )
    {
        qDebug() << "Source has gone offline, not emitting to GUI.";
        return;
    }
    // we arent sure which it is, but it can't be more th an one. so try both
    dynplaylist_ptr playlist = source()->dbCollection()->autoPlaylist( m_playlistguid );
    if( playlist.isNull() )
        playlist = source()->dbCollection()->station( m_playlistguid );

    tLog( LOGVERBOSE ) << "Just tried to load playlist for deletion:" << m_playlistguid << "Did we get a null one?" << playlist.isNull();
    Q_ASSERT( !playlist.isNull() );
    if ( !playlist.isNull() )
    {
        tLog( LOGVERBOSE ) << "is it a station?" << ( playlist->mode() == OnDemand );
        playlist->reportDeleted( playlist );
    }

    if( source()->isLocal() )
        Servent::instance()->triggerDBSync();
}
