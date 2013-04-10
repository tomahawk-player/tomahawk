/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Teo Mrnjavac <teo@kde.org>
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

#include "InboxModel.h"

#include "database/Database.h"
#include "database/DatabaseCommand_GenericSelect.h"


InboxModel::InboxModel( QObject* parent )
    : PlaylistModel( parent )
{
    loadTracks();
}


InboxModel::~InboxModel()
{}


void
InboxModel::loadTracks()
{
    startLoading();

    //extra fields end up in Tomahawk query objects as qt properties
    QString sql = QString( "SELECT track.name as title, artist.name as artist, source, v as unlistened, social_attributes.timestamp "
                           "FROM social_attributes, track, artist "
                           "WHERE social_attributes.id = track.id AND artist.id = track.artist AND social_attributes.k = 'Inbox' "
                           "GROUP BY track.id "
                           "ORDER BY social_attributes.timestamp DESC" );

    DatabaseCommand_GenericSelect* cmd = new DatabaseCommand_GenericSelect( sql, DatabaseCommand_GenericSelect::Track, -1, 0 );
    connect( cmd, SIGNAL( tracks( QList<Tomahawk::query_ptr> ) ), this, SLOT( tracksLoaded( QList<Tomahawk::query_ptr> ) ) );
    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
}


void
InboxModel::tracksLoaded( QList< Tomahawk::query_ptr > newTracks )
{
    finishLoading();

    QList< Tomahawk::query_ptr > tracks;

    foreach ( const Tomahawk::plentry_ptr ple, playlistEntries() )
        tracks << ple->query();

    bool changed = false;
    QList< Tomahawk::query_ptr > mergedTracks = TomahawkUtils::mergePlaylistChanges( tracks, newTracks, changed );

    if ( changed )
    {
        QList< Tomahawk::plentry_ptr > el = playlist()->entriesFromQueries( mergedTracks, true );

        clear();
        appendEntries( el );
    }
}
