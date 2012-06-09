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

#include "DatabaseImpl.h"
#include "SourceList.h"
#include "utils/Logger.h"

using namespace Tomahawk;


DatabaseCommand_TrackStats::DatabaseCommand_TrackStats( const query_ptr& query, QObject* parent )
    : DatabaseCommand( parent )
    , m_query( query )
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

    if ( !m_query.isNull() )
    {
        int artid = dbi->artistId( m_query->artist(), false );
        if( artid < 1 )
            return;

        int trkid = dbi->trackId( artid, m_query->track(), false );
        if( trkid < 1 )
            return;

        query.prepare( "SELECT * "
                       "FROM playback_log "
                       "WHERE track = ? ORDER BY playtime ASC" );
        query.addBindValue( trkid );
        query.exec();
    }
    else if ( !m_artist.isNull() )
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

        if ( !log.source.isNull() )
            playbackData.append( log );
    }

    if ( !m_query.isNull() )
        m_query->setPlaybackHistory( playbackData );
    else
        m_artist->setPlaybackHistory( playbackData );

    emit done( playbackData );
}
