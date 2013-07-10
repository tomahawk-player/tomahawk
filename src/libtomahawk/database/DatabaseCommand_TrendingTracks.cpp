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

#include "DatabaseCommand_TrendingTracks_p.h"

#include "database/DatabaseImpl.h"
#include "database/TomahawkSqlQuery.h"
#include "Track.h"

namespace Tomahawk {

DatabaseCommand_TrendingTracks::DatabaseCommand_TrendingTracks( QObject* parent )
    : DatabaseCommand( parent, new DatabaseCommand_TrendingTracksPrivate( this ) )
{
}


DatabaseCommand_TrendingTracks::~DatabaseCommand_TrendingTracks()
{
}


void
DatabaseCommand_TrendingTracks::exec( DatabaseImpl* dbi )
{
    Q_D( DatabaseCommand_TrendingTracks );
    TomahawkSqlQuery query = dbi->newquery();

    QString limit;
    if ( d->amount > 0 )
    {
        limit = QString( "LIMIT 0, %1" ).arg( d->amount );
    }

    QDateTime now = QDateTime::currentDateTime();
    QDateTime _1WeekAgo = now.addDays( -7 );
    QDateTime _2WeeksAgo = now.addDays( -14 );
    QDateTime _3WeeksAgo = now.addDays( -21 );

    // TODO:
    // -> Look at absolute playcount, an increase from 1 to 4 plays per week is currently considered very high
    QString timespanSql = QString(
                " SELECT COUNT(*) as counter, track "
                " FROM playback_log "
                " WHERE playback_log.source IS NOT NULL " // exclude self
                " AND playback_log.playtime >= %1 AND playback_log.playtime <= %2 "
                " GROUP BY playback_log.track "
                );
    QString lastWeekSql = timespanSql.arg( _1WeekAgo.toTime_t() ).arg( now.toTime_t() );
    QString _1BeforeLastWeekSql = timespanSql.arg( _2WeeksAgo.toTime_t() ).arg( _1WeekAgo.toTime_t() );
    QString _2BeforeLastWeekSql = timespanSql.arg( _3WeeksAgo.toTime_t() ).arg( _2WeeksAgo.toTime_t() );
    QString sql = QString(
                " SELECT track.name, artist.name, ( lastweek.counter - weekbefore.counter ) as slope1, max( weekbefore.counter - week2before.counter, 1 ) as slope2, ( ( lastweek.counter - weekbefore.counter ) / max( weekbefore.counter - week2before.counter, 1 ) ) as trending "
                " FROM ( %1 ) lastweek, ( %2 ) weekbefore, ( %3 ) week2before, track, artist "
                " WHERE lastweek.track = weekbefore.track AND weekbefore.track = week2before.track "
                " AND track.id = lastweek.track AND artist.id = track.artist "
                " AND ( lastweek.counter - weekbefore.counter ) > 0"
                " ORDER BY slope1 DESC %4 "
                ).arg( lastWeekSql ).arg( _1BeforeLastWeekSql ).arg( _2BeforeLastWeekSql ).arg( limit );

    query.prepare( sql );
    query.exec();



    QList< QPair< double, Tomahawk::track_ptr > > tracks;
    while ( query.next() )
    {
            Tomahawk::track_ptr track = Tomahawk::Track::get( query.value( 1 ).toString(), query.value( 0 ).toString() );
            if ( !track )
                continue;

            tracks << QPair< double, track_ptr >( query.value( 4 ).toDouble(), track );
    }

    emit done( tracks );
}


void
DatabaseCommand_TrendingTracks::setLimit( unsigned int amount )
{
    Q_D( DatabaseCommand_TrendingTracks );
    d->amount = amount;
}

} // namespace Tomahawk
