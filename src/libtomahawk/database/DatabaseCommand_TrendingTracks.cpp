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

#include <QDateTime>

#include <algorithm>

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

    QString limit;
    if ( d->amount > 0 )
    {
        limit = QString( "LIMIT 0, %1" ).arg( d->amount );
    }

    QDateTime now = QDateTime::currentDateTime();
    QDateTime _1WeekAgo = now.addDays( -7 );
    QDateTime _2WeeksAgo = now.addDays( -14 );

    uint peersLastWeek = 1; // Use a default of 1 to be able to do certain mathematical computations without Div-by-0 Errors.
    {
        // Get the number of active peers in the last week.
        // We could just use the number of peers instead but that would include old peers that may have been inactive for a long while.

        QString peersLastWeekSql = QString(
                    " SELECT COUNT(DISTINCT source ) "
                    " FROM playback_log "
                    " WHERE playback_log.source IS NOT NULL " // exclude self
                    " AND playback_log.playtime >= %1 "
                    ).arg( _1WeekAgo.toTime_t() );
        TomahawkSqlQuery query = dbi->newquery();
        query.prepare( peersLastWeekSql );
        query.exec();
        while ( query.next() )
        {
            peersLastWeek = std::max( 1u, query.value( 0 ).toUInt() );
        }
    }


    QString timespanSql = QString(
                " SELECT COUNT(*) as counter, track "
                " FROM playback_log "
                " WHERE playback_log.source IS NOT NULL " // exclude self
                " AND playback_log.playtime >= %1 AND playback_log.playtime <= %2 "
                " GROUP BY playback_log.track "
                " HAVING counter > 0 "
                );
    QString lastWeekSql = timespanSql.arg( _1WeekAgo.toTime_t() ).arg( now.toTime_t() );
    QString _1BeforeLastWeekSql = timespanSql.arg( _2WeeksAgo.toTime_t() ).arg( _1WeekAgo.toTime_t() );
    QString formula = QString(
                " (  lastweek.counter /  weekbefore.counter ) "
                " * "
                " max(0, 1 - (%1 / (4*min(lastweek.counter, weekbefore.counter )) ) )"
                ).arg( peersLastWeek );
    QString sql = QString(
                " SELECT track.name, artist.name, ( %4 ) as trending "
                " FROM ( %1 ) lastweek, ( %2 ) weekbefore, track, artist "
                " WHERE lastweek.track = weekbefore.track "
                " AND track.id = lastweek.track AND artist.id = track.artist "
                " AND ( lastweek.counter - weekbefore.counter ) > 0"
                " ORDER BY trending DESC %3 "
                ).arg( lastWeekSql ).arg( _1BeforeLastWeekSql ).arg( limit ).arg( formula );
    TomahawkSqlQuery query = dbi->newquery();
    query.prepare( sql );
    query.exec();



    QList< QPair< double, Tomahawk::track_ptr > > tracks;
    while ( query.next() )
    {
            Tomahawk::track_ptr track = Tomahawk::Track::get( query.value( 1 ).toString(), query.value( 0 ).toString() );
            if ( !track )
                continue;

            tracks << QPair< double, track_ptr >( query.value( 2 ).toDouble(), track );
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
