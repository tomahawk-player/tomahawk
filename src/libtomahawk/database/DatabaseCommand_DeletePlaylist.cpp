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

#include "DatabaseCommand_DeletePlaylist.h"

#include <QSqlQuery>

#include "network/Servent.h"
#include "utils/Logger.h"

using namespace Tomahawk;


DatabaseCommand_DeletePlaylist::DatabaseCommand_DeletePlaylist( const source_ptr& source, const QString& playlistguid )
    : DatabaseCommandLoggable( source )
{
    setPlaylistguid( playlistguid );
}


void
DatabaseCommand_DeletePlaylist::exec( DatabaseImpl* lib )
{
    qDebug() << Q_FUNC_INFO;

    TomahawkSqlQuery cre = lib->newquery();

    QString sql = QString( "DELETE FROM playlist WHERE guid = :id AND source %1" )
                  .arg( source()->isLocal() ? "IS NULL" : QString("= %1").arg( source()->id() ) );
    cre.prepare( sql );
    cre.bindValue( ":id", m_playlistguid );

    cre.exec();
}


void
DatabaseCommand_DeletePlaylist::postCommitHook()
{
    qDebug() << Q_FUNC_INFO << "..reporting..";
    if ( source().isNull() || source()->dbCollection().isNull() )
    {
        qDebug() << "Source has gone offline, not emitting to GUI.";
        return;
    }

    playlist_ptr playlist = source()->dbCollection()->playlist( m_playlistguid );
    Q_ASSERT( !playlist.isNull() );

    playlist->reportDeleted( playlist );

    if( source()->isLocal() )
        Servent::instance()->triggerDBSync();
}
