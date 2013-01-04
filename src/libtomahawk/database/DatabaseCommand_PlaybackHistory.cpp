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

#include <QSqlQuery>

#include "DatabaseImpl.h"
#include "SourceList.h"

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

    QList<Tomahawk::query_ptr> ql;
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
            Tomahawk::query_ptr q = Tomahawk::Query::get( query_track.value( 1 ).toString(), query_track.value( 0 ).toString(), QString() );
            if ( q.isNull() )
                continue;

            if ( query.value( 3 ).toUInt() == 0 )
            {
                q->setPlayedBy( SourceList::instance()->getLocal(), query.value( 1 ).toUInt() );
            }
            else
            {
                q->setPlayedBy( SourceList::instance()->get( query.value( 3 ).toUInt() ), query.value( 1 ).toUInt() );
            }

            ql << q;
        }
    }

    emit tracks( ql );
}
