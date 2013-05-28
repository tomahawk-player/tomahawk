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

#include "DatabaseCommand_PlaybackHistory.h"

#include "DatabaseImpl.h"
#include "SourceList.h"
#include "Source.h"
#include "database/TomahawkSqlQuery.h"

#include "utils/Logger.h"

void
DatabaseCommand_PlaybackHistory::exec( DatabaseImpl* dbi )
{
    TomahawkSqlQuery query = dbi->newquery();
    QString whereToken;

    if ( !source().isNull() )
    {
        whereToken = QString( "WHERE source %1" ).arg( source()->isLocal() ? "IS NULL" : QString( "= %1" ).arg( source()->id() ) );
    }

    QString sql = QString(
            "SELECT track, playtime, secs_played, source "
            "FROM playback_log "
            "%1 "
            "ORDER BY playtime DESC "
            "%2" ).arg( whereToken )
                  .arg( m_amount > 0 ? QString( "LIMIT 0, %1" ).arg( m_amount ) : QString() );

    query.prepare( sql );
    query.exec();

    QList<Tomahawk::track_ptr> tl;
    QList<Tomahawk::PlaybackLog> logs;
    while ( query.next() )
    {
        TomahawkSqlQuery query_track = dbi->newquery();

        QString sql = QString(
                "SELECT track.name, artist.name "
                "FROM track, artist "
                "WHERE artist.id = track.artist "
                "AND track.id = %1"
                ).arg( query.value( 0 ).toUInt() );

        query_track.prepare( sql );
        query_track.exec();

        if ( query_track.next() )
        {
            Tomahawk::track_ptr track = Tomahawk::Track::get( query_track.value( 1 ).toString(), query_track.value( 0 ).toString(), QString() );
            if ( !track )
                continue;

            Tomahawk::PlaybackLog log;
            log.timestamp = query.value( 1 ).toUInt();
            log.secsPlayed = query.value( 2 ).toUInt();
            log.source = SourceList::instance()->get( query.value( 3 ).toUInt() );

            logs << log;
            tl << track;
        }
    }

    emit tracks( tl, logs );
}
