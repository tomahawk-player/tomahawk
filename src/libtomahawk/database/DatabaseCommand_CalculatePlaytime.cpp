/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Uwe L. Korn <uwelk@xhochy.com>
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

#include "DatabaseCommand_CalculatePlaytime_p.h"

#include "database/DatabaseImpl.h"
#include "Source.h"
#include "Track.h"

namespace Tomahawk {

DatabaseCommand_CalculatePlaytime::DatabaseCommand_CalculatePlaytime( const playlist_ptr& playlist, QDateTime from, QDateTime to, QObject* parent )
    : DatabaseCommand( parent, new DatabaseCommand_CalculatePlaytimePrivate( this , from, to ) )
{
    Q_D( DatabaseCommand_CalculatePlaytime );
    foreach( plentry_ptr entry, playlist->entries() )
    {
        d->tracks.append( entry->query()->track() );
    }
}

DatabaseCommand_CalculatePlaytime::DatabaseCommand_CalculatePlaytime( const track_ptr& track, QDateTime from, QDateTime to, QObject* parent )
    : DatabaseCommand( parent, new DatabaseCommand_CalculatePlaytimePrivate( this , from, to ) )
{
    Q_D( DatabaseCommand_CalculatePlaytime );
    d->tracks.append( track );
}


DatabaseCommand_CalculatePlaytime::DatabaseCommand_CalculatePlaytime( const QList<track_ptr>& tracks, QDateTime from, QDateTime to, QObject* parent )
    : DatabaseCommand( parent, new DatabaseCommand_CalculatePlaytimePrivate( this , from, to ) )
{
    Q_D( DatabaseCommand_CalculatePlaytime );
    d->tracks = tracks;
}


DatabaseCommand_CalculatePlaytime::DatabaseCommand_CalculatePlaytime( const query_ptr& query, QDateTime from, QDateTime to, QObject* parent)
    : DatabaseCommand( parent, new DatabaseCommand_CalculatePlaytimePrivate( this , from, to ) )
{
    Q_D( DatabaseCommand_CalculatePlaytime );
    d->tracks.append( query->track() );
}


DatabaseCommand_CalculatePlaytime::DatabaseCommand_CalculatePlaytime( const QList<query_ptr>& queries, QDateTime from, QDateTime to, QObject* parent)
    : DatabaseCommand( parent, new DatabaseCommand_CalculatePlaytimePrivate( this , from, to ) )
{
    Q_D( DatabaseCommand_CalculatePlaytime );
    foreach ( query_ptr query, queries )
    {
        d->tracks.append( query->track() );
    }
}


DatabaseCommand_CalculatePlaytime::~DatabaseCommand_CalculatePlaytime()
{
}

void
DatabaseCommand_CalculatePlaytime::exec( DatabaseImpl *dbi )
{
    Q_D( DatabaseCommand_CalculatePlaytime );

    // Get all trackIds
    QStringList trackIds;
    foreach ( track_ptr track, d->tracks )
    {
        trackIds.append( QString::number( track->trackId() ) );
    }

    QString sql = QString(
                " SELECT SUM(secs_played) "
                " FROM playback_log "
                " WHERE track in ( %1 ) AND playtime >= %2 AND playtime <= %3 "
                ).arg( trackIds.join(", ") ).arg( d->from.toTime_t() ).arg( d->to.toTime_t() );

    TomahawkSqlQuery query = dbi->newquery();
    query.prepare( sql );
    query.exec();

    uint playtime;
    while ( query.next() )
    {
        playtime = query.value( 0 ).toUInt();
    }
    emit done( playtime );
}


} // namespace Tomahawk
