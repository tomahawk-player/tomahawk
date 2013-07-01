/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "DatabaseCommand_TrackStats.h"

#include "utils/Logger.h"

#include "Artist.h"
#include "DatabaseImpl.h"
#include "PlaylistEntry.h"
#include "SourceList.h"

// Forward Declarations breaking QSharedPointer
#if QT_VERSION < QT_VERSION_CHECK( 5, 0, 0 )
    #include "collection/Collection.h"
#endif

using namespace Tomahawk;


DatabaseCommand_TrackStats::DatabaseCommand_TrackStats( const trackdata_ptr& track, QObject* parent )
    : DatabaseCommand( parent )
    , m_track( track )
{
}


DatabaseCommand_TrackStats::DatabaseCommand_TrackStats( const artist_ptr& artist, QObject* parent )
    : DatabaseCommand( parent )
    , m_artist( artist )
{
}


void
DatabaseCommand_TrackStats::exec( DatabaseImpl* dbi )
{
    TomahawkSqlQuery query = dbi->newquery();

    if ( m_track )
    {
        if ( m_track->trackId() == 0 )
            return;

        query.prepare( "SELECT COUNT(*) AS counter, track.id "
                       "FROM playback_log, track "
                       "WHERE playback_log.source IS NULL AND track.id = playback_log.track "
                       "GROUP BY track.id "
                       "ORDER BY counter DESC" );
        query.exec();

        unsigned int chartPos = 0;
        unsigned int chartCount = 0;
        const unsigned int trackId = m_track->trackId();

        QHash< QString, unsigned int > charts;
        while ( query.next() )
        {
            if ( query.value( 0 ).toUInt() < 2 )
                break;

            chartCount++;
            if ( chartPos == 0 && query.value( 1 ).toUInt() == trackId )
            {
                chartPos = chartCount;
            }
        }

        if ( chartPos == 0 )
            chartPos = chartCount;

        emit trackStats( chartPos, chartCount );

        query.prepare( "SELECT * "
                       "FROM playback_log "
                       "WHERE track = ? ORDER BY playtime ASC" );
        query.addBindValue( m_track->trackId() );
        query.exec();
    }
    else if ( m_artist )
    {
        query.prepare( "SELECT playback_log.* "
                       "FROM playback_log, track "
                       "WHERE playback_log.track = track.id AND track.artist = ?" );
        query.addBindValue( m_artist->id() );
        query.exec();
    }

    QList< Tomahawk::PlaybackLog > playbackData;
    while ( query.next() )
    {
        Tomahawk::PlaybackLog log;
        log.source = SourceList::instance()->get( query.value( 1 ).toInt() );  // source
        log.timestamp = query.value( 3 ).toUInt();
        log.secsPlayed = query.value( 4 ).toUInt();

        if ( log.source )
            playbackData.append( log );
    }

    if ( m_track )
        m_track->setPlaybackHistory( playbackData );
    else
        m_artist->setPlaybackHistory( playbackData );

    emit done( playbackData );
}
