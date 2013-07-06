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

#include "DatabaseCommand_NetworkCharts.h"

#include "Track.h"
#include "DatabaseImpl.h"
#include "TomahawkSqlQuery.h"

// Forward Declarations breaking QSharedPointer
#if QT_VERSION < QT_VERSION_CHECK( 5, 0, 0 )
    #include "Source.h"
#endif


namespace Tomahawk
{

DatabaseCommand_NetworkCharts::DatabaseCommand_NetworkCharts( const QDateTime &from, const QDateTime &to, QObject *parent )
    : DatabaseCommand( parent )
    , m_amount( 0 )
    , m_from( from )
    , m_to( to )
{
}

DatabaseCommand_NetworkCharts::DatabaseCommand_NetworkCharts( QObject *parent )
    : DatabaseCommand( parent )
    , m_amount( 0 )
{
}

DatabaseCommand_NetworkCharts::~DatabaseCommand_NetworkCharts()
{
}

void
DatabaseCommand_NetworkCharts::exec( DatabaseImpl * dbi )
{
    TomahawkSqlQuery query = dbi->newquery();

    QString limit;
    if ( m_amount > 0 )
    {
        limit = QString( "LIMIT 0, %1" ).arg( m_amount );
    }
    QString timespan;
    if ( m_from.isValid() && m_to.isValid() )
    {
        timespan = QString(
                    " AND playback_log.playtime >= %1 AND playback_log.playtime <= %2 "
                    ).arg( m_from.toTime_t() ).arg( m_to.toTime_t() );
    }

    QString sql = QString(
                "SELECT COUNT(*) as counter, track.name, artist.name "
                " FROM playback_log, track, artist "
                " WHERE track.id = playback_log.track AND artist.id = track.artist "
                " AND playback_log.source IS NOT NULL %1 " // exclude self
                " GROUP BY playback_log.track "
                " ORDER BY counter DESC "
                " %2"
                ).arg( timespan ).arg( limit );

    query.prepare( sql );
    query.exec();

    QList<Tomahawk::track_ptr> tracks;
    while ( query.next() )
    {
            Tomahawk::track_ptr track = Tomahawk::Track::get( query.value( 2 ).toString(), query.value( 1 ).toString() );
            if ( !track )
                continue;

            tracks << track;
    }

    emit done( tracks );
}

}
