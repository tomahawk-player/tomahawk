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

#include "databasecommand_alltracks.h"

#include <QSqlQuery>

#include "databaseimpl.h"
#include "artist.h"
#include "album.h"
#include "sourcelist.h"


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
            m_orderToken = "album.name, file_join.albumpos";
            break;

        case ModificationTime:
            m_orderToken = "file.mtime";
            break;

        case AlbumPosition:
            m_orderToken = "file_join.albumpos";
            break;
    }

    if ( !m_collection.isNull() )
        sourceToken = QString( "AND file.source %1" ).arg( m_collection->source()->isLocal() ? "IS NULL" : QString( "= %1" ).arg( m_collection->source()->id() ) );

    QString albumToken;
    if ( m_album )
    {
        if ( m_album->id() == 0 )
        {
            m_artist = m_album->artist().data();
            albumToken = QString( "AND album.id IS NULL" );
        }
        else
            albumToken = QString( "AND album.id = %1" ).arg( m_album->id() );
    }

    QString sql = QString(
            "SELECT file.id, artist.name, album.name, track.name, file.size, "
                   "file.duration, file.bitrate, file.url, file.source, file.mtime, file.mimetype, file_join.albumpos, artist.id, album.id, track.id "
            "FROM file, artist, track, file_join "
            "LEFT OUTER JOIN album "
            "ON file_join.album = album.id "
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

    while( query.next() )
    {
        Tomahawk::result_ptr result = Tomahawk::result_ptr( new Tomahawk::Result() );
        Tomahawk::source_ptr s;

        if( query.value( 8 ).toUInt() == 0 )
        {
            s = SourceList::instance()->getLocal();
            result->setUrl( query.value( 7 ).toString() );
        }
        else
        {
            s = SourceList::instance()->get( query.value( 8 ).toUInt() );
            if( s.isNull() )
            {
                Q_ASSERT( false );
                continue;
            }

            result->setUrl( QString( "servent://%1\t%2" ).arg( s->userName() ).arg( query.value( 7 ).toString() ) );
        }

        QString artist, track, album;
        artist = query.value( 1 ).toString();
        album = query.value( 2 ).toString();
        track = query.value( 3 ).toString();

        Tomahawk::query_ptr qry = Tomahawk::Query::get( artist, track, album );
        Tomahawk::artist_ptr artistptr = Tomahawk::Artist::get( query.value( 12 ).toUInt(), artist );
        Tomahawk::album_ptr albumptr = Tomahawk::Album::get( query.value( 13 ).toUInt(), album, artistptr );

        result->setId( query.value( 14 ).toUInt() );
        result->setArtist( artistptr );
        result->setAlbum( albumptr );
        result->setTrack( query.value( 3 ).toString() );
        result->setSize( query.value( 4 ).toUInt() );
        result->setDuration( query.value( 5 ).toUInt() );
        result->setBitrate( query.value( 6 ).toUInt() );
        result->setModificationTime( query.value( 9 ).toUInt() );
        result->setMimetype( query.value( 10 ).toString() );
        result->setAlbumPos( query.value( 11 ).toUInt() );
        result->setScore( 1.0 );
        result->setCollection( s->collection() );

        TomahawkSqlQuery attrQuery = dbi->newquery();
        QVariantMap attr;

        attrQuery.prepare( "SELECT k, v FROM track_attributes WHERE id = ?" );
        attrQuery.bindValue( 0, result->dbid() );
        attrQuery.exec();
        while ( attrQuery.next() )
        {
            attr[ attrQuery.value( 0 ).toString() ] = attrQuery.value( 1 ).toString();
        }

        result->setAttributes( attr );

        QList<Tomahawk::result_ptr> results;
        results << result;
        qry->addResults( results );
        qry->setResolveFinished( true );

        ql << qry;
    }

    qDebug() << Q_FUNC_INFO << ql.length();

    emit tracks( ql, data() );
    emit done( m_collection );
}
