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

#include "DatabaseCommand_TrendingArtists_p.h"

#include "database/DatabaseImpl.h"
#include "Artist.h"

#include <QDateTime>

namespace Tomahawk {


DatabaseCommand_TrendingArtists::DatabaseCommand_TrendingArtists( QObject* parent )
    : DatabaseCommand( parent, new DatabaseCommand_TrendingArtistsPrivate( this ) )
{
}


DatabaseCommand_TrendingArtists::~DatabaseCommand_TrendingArtists()
{
}


void
DatabaseCommand_TrendingArtists::exec( DatabaseImpl* dbi )
{
    Q_D( DatabaseCommand_TrendingArtists );

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
                " SELECT COUNT(*) as counter, track.artist as artistid "
                " FROM playback_log "
                " JOIN track ON track.id = playback_log.track "
                " WHERE playback_log.source IS NOT NULL " // exclude self
                " AND playback_log.playtime >= %1 AND playback_log.playtime <= %2 "
                " GROUP BY track.artist "
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
                " SELECT artist.name, ( %4 ) as trending "
                " FROM ( %1 ) lastweek, ( %2 ) weekbefore, artist "
                " WHERE lastweek.artistid = weekbefore.artistid "
                " AND artist.id = lastweek.artistid "
                " AND ( lastweek.counter - weekbefore.counter ) > 0"
                " ORDER BY trending DESC %3 "
                ).arg( lastWeekSql ).arg( _1BeforeLastWeekSql ).arg( limit ).arg( formula );
    TomahawkSqlQuery query = dbi->newquery();
    query.prepare( sql );
    query.exec();



    QList< QPair< double, Tomahawk::artist_ptr > > artists;
    while ( query.next() )
    {
        Tomahawk::artist_ptr artist = Artist::get( query.value( 0 ).toString() );
        if ( !artist )
            continue;

        artists << QPair< double, artist_ptr >( query.value( 1 ).toDouble(), artist );
    }

    emit done( artists );
}


void
DatabaseCommand_TrendingArtists::setLimit( unsigned int amount )
{
    Q_D( DatabaseCommand_TrendingArtists );
    d->amount = amount;
}


} // namespace Tomahawk
