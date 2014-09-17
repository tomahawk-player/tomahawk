/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include "DatabaseCommand_AllTracks.h"

#include <QSqlQuery>

#include "utils/Logger.h"

#include "Album.h"
#include "Artist.h"
#include "DatabaseImpl.h"
#include "Result.h"
#include "SourceList.h"
#include "Track.h"

namespace Tomahawk
{

void
DatabaseCommand_AllTracks::exec( DatabaseImpl* dbi )
{
    TomahawkSqlQuery query = dbi->newquery();
    QList<Tomahawk::query_ptr> ql;

    QString m_orderToken, sourceToken;
    switch ( m_sortOrder )
    {
        case 0:
            break;

        case Album:
            m_orderToken = "album.name, file_join.discnumber, file_join.albumpos";
            break;

        case ModificationTime:
            m_orderToken = "file.mtime";
            break;

        case AlbumPosition:
            m_orderToken = "file_join.discnumber, file_join.albumpos";
            break;
    }

    if ( !m_collection.isNull() )
        sourceToken = QString( "AND file.source %1" ).arg( m_collection->source()->isLocal() ? "IS NULL" : QString( "= %1" ).arg( m_collection->source()->id() ) );

    QString albumToken;
    if ( m_album )
    {
        if ( m_album->id() == 0 )
        {
            m_artist = m_album->artist();
            albumToken = QString( "AND album.id IS NULL" );
        }
        else
            albumToken = QString( "AND album.id = %1" ).arg( m_album->id() );
    }

    QString sql = QString(
            "SELECT file.id, artist.name, album.name, track.name, composer.name, file.size, "   //0
                   "file.duration, file.bitrate, file.url, file.source, file.mtime, "           //6
                   "file.mimetype, file_join.discnumber, file_join.albumpos, track.id "       //11
            "FROM file, artist, track, file_join "
            "LEFT OUTER JOIN album "
            "ON file_join.album = album.id "
            "LEFT OUTER JOIN artist AS composer "
            "ON file_join.composer = composer.id "
            "WHERE file.id = file_join.file "
            "AND file_join.artist = artist.id "
            "AND file_join.track = track.id "
            "%1 "
            "%2 %3 "
            "%4 %5 %6"
            ).arg( sourceToken )
             .arg( !m_artist ? QString() : QString( "AND artist.id = %1" ).arg( m_artist->id() ) )
             .arg( !m_album ? QString() : albumToken )
             .arg( m_sortOrder > 0 ? QString( "ORDER BY %1" ).arg( m_orderToken ) : QString() )
             .arg( m_sortDescending ? "DESC" : QString() )
             .arg( m_amount > 0 ? QString( "LIMIT 0, %1" ).arg( m_amount ) : QString() );

    query.prepare( sql );
    query.exec();

    // Small cache to keep already created source objects.
    // This saves some mutex locking.
    std::map<uint, Tomahawk::source_ptr> sourceCache;

    while( query.next() )
    {
        QString artist = query.value( 1 ).toString();
        QString album = query.value( 2 ).toString();
        QString track = query.value( 3 ).toString();
        QString composer = query.value( 4 ).toString();
        uint size = query.value( 5 ).toUInt();
        uint duration = query.value( 6 ).toUInt();
        uint bitrate = query.value( 7 ).toUInt();
        QString url = query.value( 8 ).toString();
        uint sourceId = query.value( 9 ).toUInt();
        uint modificationTime = query.value( 10 ).toUInt();
        QString mimetype = query.value( 11 ).toString();
        uint discnumber = query.value( 12 ).toUInt();
        uint albumpos = query.value( 13 ).toUInt();
        uint trackId = query.value( 14 ).toUInt();

        std::map<uint, Tomahawk::source_ptr>::const_iterator _s = sourceCache.find( sourceId );
        Tomahawk::source_ptr s;
        if ( _s == sourceCache.end() )
        {
            s = SourceList::instance()->get( sourceId );
            sourceCache[sourceId] = s;
        }
        else
        {
            s = _s->second;
        }
        if ( !s )
        {
            Q_ASSERT( false );
            continue;
        }
        if ( !s->isLocal() )
            url = QString( "servent://%1\t%2" ).arg( s->nodeId() ).arg( url );

        Tomahawk::result_ptr result = Tomahawk::Result::get( url );
        Tomahawk::track_ptr t = Tomahawk::Track::get( trackId,
                                                      artist, track, album,
                                                      duration, composer,
                                                      albumpos, discnumber );
        if ( m_album || m_artist ) {
            t->loadAttributes();
        }
        result->setTrack( t );

        result->setSize( size );
        result->setBitrate( bitrate );
        result->setModificationTime( modificationTime );
        result->setMimetype( mimetype );
        result->setScore( 1.0f );
        result->setCollection( s->dbCollection(), false );

        ql << Tomahawk::Query::getFixed( t, result );
    }

    emit tracks( ql, data() );
    emit tracks( ql );
    emit done( m_collection );
}

}
