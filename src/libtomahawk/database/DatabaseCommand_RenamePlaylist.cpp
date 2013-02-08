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

#include "DatabaseCommand_RenamePlaylist.h"

#include <QSqlQuery>

#include "DatabaseImpl.h"
#include "collection/Collection.h"
#include "Source.h"
#include "network/Servent.h"
#include "utils/Logger.h"

using namespace Tomahawk;


DatabaseCommand_RenamePlaylist::DatabaseCommand_RenamePlaylist( const source_ptr& source, const QString& playlistguid, const QString& playlistTitle )
    : DatabaseCommandLoggable( source )
{
    setPlaylistguid( playlistguid );
    setPlaylistTitle( playlistTitle );
}


void
DatabaseCommand_RenamePlaylist::exec( DatabaseImpl* lib )
{
    TomahawkSqlQuery cre = lib->newquery();

    QString sql = QString( "UPDATE playlist SET title = :title WHERE guid = :id AND source %1" )
                     .arg( source()->isLocal() ? "IS NULL" : QString( "= %1" ).arg( source()->id() ) );

    cre.prepare( sql );
    cre.bindValue( ":id", m_playlistguid );
    cre.bindValue( ":title", m_playlistTitle );

    qDebug() << Q_FUNC_INFO << m_playlistTitle << m_playlistguid;

    cre.exec();
}


void
DatabaseCommand_RenamePlaylist::postCommitHook()
{
    playlist_ptr playlist = source()->dbCollection()->playlist( m_playlistguid );
    // fallback, check for auto and stations too
    if( playlist.isNull() )
        playlist = source()->dbCollection()->autoPlaylist( m_playlistguid );
    if( playlist.isNull() )
        playlist = source()->dbCollection()->station( m_playlistguid );

    Q_ASSERT( !playlist.isNull() );

    qDebug() << "Renaming old playlist" << playlist->title() << "to" << m_playlistTitle << m_playlistguid;
    playlist->setTitle( m_playlistTitle );

    if( source()->isLocal() )
        Servent::instance()->triggerDBSync();
}
