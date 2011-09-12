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

#include "databasecommand_resolve.h"

#include "artist.h"
#include "album.h"
#include "sourcelist.h"
#include "utils/logger.h"

using namespace Tomahawk;


DatabaseCommand_Resolve::DatabaseCommand_Resolve( const query_ptr& query )
    : DatabaseCommand()
    , m_query( query )
{
}


void
DatabaseCommand_Resolve::exec( DatabaseImpl* lib )
{
    /*
     *        Resolving is a 2 stage process.
     *        1) find list of trk/art/alb IDs that are reasonable matches to the metadata given
     *        2) find files in database by permitted sources and calculate score, ignoring
     *           results that are less than MINSCORE
     */

    if ( !m_query->resultHint().isEmpty() )
    {
        qDebug() << "Using result-hint to speed up resolving:" << m_query->resultHint();

        Tomahawk::result_ptr result = lib->resultFromHint( m_query );
        /*        qDebug() << "Result null:" << result.isNull();
         *        qDebug() << "Collection null:" << result->collection().isNull();
         *        qDebug() << "Source null:" << result->collection()->source().isNull();*/
        if ( !result.isNull() && !result->collection().isNull() && result->collection()->source()->isOnline() )
        {
            QList<Tomahawk::result_ptr> res;
            res << result;
            emit results( m_query->id(), res );
            return;
        }
    }

    if ( m_query->isFullTextQuery() )
        fullTextResolve( lib );
    else
        resolve( lib );
}


void
DatabaseCommand_Resolve::resolve( DatabaseImpl* lib )
{
    QList<Tomahawk::result_ptr> res;
    typedef QPair<int, float> scorepair_t;

    // STEP 1
    QList< QPair<int, float> > artists = lib->searchTable( "artist", m_query->artist(), 10 );
    QList< QPair<int, float> > tracks = lib->searchTable( "track", m_query->track(), 10 );
    QList< QPair<int, float> > albums = lib->searchTable( "album", m_query->album(), 10 );

    if ( artists.length() == 0 || tracks.length() == 0 )
    {
        qDebug() << "No candidates found in first pass, aborting resolve" << m_query->artist() << m_query->track();
        emit results( m_query->id(), res );
        return;
    }

    // STEP 2
    TomahawkSqlQuery files_query = lib->newquery();

    QStringList artsl, trksl;
    for ( int k = 0; k < artists.count(); k++ )
        artsl.append( QString::number( artists.at( k ).first ) );
    for ( int k = 0; k < tracks.count(); k++ )
        trksl.append( QString::number( tracks.at( k ).first ) );

    QString artsToken = QString( "file_join.artist IN (%1)" ).arg( artsl.join( "," ) );
    QString trksToken = QString( "file_join.track IN (%1)" ).arg( trksl.join( "," ) );

    QString sql = QString( "SELECT "
                            "url, mtime, size, md5, mimetype, duration, bitrate, file_join.artist, file_join.album, file_join.track, "
                            "artist.name as artname, "
                            "album.name as albname, "
                            "track.name as trkname, "
                            "file.source, "
                            "file_join.albumpos, "
                            "artist.id as artid, "
                            "album.id as albid "
                            "FROM file, file_join, artist, track "
                            "LEFT JOIN album ON album.id = file_join.album "
                            "WHERE "
                            "artist.id = file_join.artist AND "
                            "track.id = file_join.track AND "
                            "file.id = file_join.file AND "
                            "(%1 AND %2)" )
         .arg( artsToken )
         .arg( trksToken );

    files_query.prepare( sql );
    files_query.exec();

    while ( files_query.next() )
    {
        source_ptr s;
        QString url = files_query.value( 0 ).toString();

        if ( files_query.value( 13 ).toUInt() == 0 )
        {
            s = SourceList::instance()->getLocal();
        }
        else
        {
            s = SourceList::instance()->get( files_query.value( 13 ).toUInt() );
            if( s.isNull() )
            {
                qDebug() << "Could not find source" << files_query.value( 13 ).toUInt();
                continue;
            }

            url = QString( "servent://%1\t%2" ).arg( s->userName() ).arg( url );
        }

        Tomahawk::result_ptr result = Tomahawk::Result::get( url );
        Tomahawk::artist_ptr artist = Tomahawk::Artist::get( files_query.value( 15 ).toUInt(), files_query.value( 10 ).toString() );
        Tomahawk::album_ptr album = Tomahawk::Album::get( files_query.value( 16 ).toUInt(), files_query.value( 11 ).toString(), artist );

        result->setModificationTime( files_query.value( 1 ).toUInt() );
        result->setSize( files_query.value( 2 ).toUInt() );
        result->setMimetype( files_query.value( 4 ).toString() );
        result->setDuration( files_query.value( 5 ).toUInt() );
        result->setBitrate( files_query.value( 6 ).toUInt() );
        result->setArtist( artist );
        result->setAlbum( album );
        result->setTrack( files_query.value( 12 ).toString() );
        result->setRID( uuid() );
        result->setAlbumPos( files_query.value( 14 ).toUInt() );
        result->setId( files_query.value( 9 ).toUInt() );
        result->setYear( files_query.value( 17 ).toUInt() );

        TomahawkSqlQuery attrQuery = lib->newquery();
        QVariantMap attr;

        attrQuery.prepare( "SELECT k, v FROM track_attributes WHERE id = ?" );
        attrQuery.bindValue( 0, result->dbid() );
        attrQuery.exec();
        while ( attrQuery.next() )
        {
            attr[ attrQuery.value( 0 ).toString() ] = attrQuery.value( 1 ).toString();
        }

        result->setAttributes( attr );
        result->setCollection( s->collection() );
        res << result;
    }

    emit results( m_query->id(), res );
}


void
DatabaseCommand_Resolve::fullTextResolve( DatabaseImpl* lib )
{
    QList<Tomahawk::result_ptr> res;
    typedef QPair<int, float> scorepair_t;

    // STEP 1
    QList< QPair<int, float> > artists = lib->searchTable( "artist", m_query->fullTextQuery(), 10 );
    QList< QPair<int, float> > tracks = lib->searchTable( "track", m_query->fullTextQuery(), 10 );
    QList< QPair<int, float> > albums = lib->searchTable( "album", m_query->fullTextQuery(), 10 );

    if ( artists.length() == 0 && tracks.length() == 0 && albums.length() == 0 )
    {
        qDebug() << "No candidates found in first pass, aborting resolve" << m_query->artist() << m_query->track();
        emit results( m_query->id(), res );
        return;
    }

    // STEP 2
    TomahawkSqlQuery files_query = lib->newquery();

    QStringList artsl, trksl, albsl;
    for ( int k = 0; k < artists.count(); k++ )
        artsl.append( QString::number( artists.at( k ).first ) );
    for ( int k = 0; k < tracks.count(); k++ )
        trksl.append( QString::number( tracks.at( k ).first ) );
    for ( int k = 0; k < albums.count(); k++ )
        albsl.append( QString::number( albums.at( k ).first ) );

    QString artsToken = QString( "file_join.artist IN (%1)" ).arg( artsl.join( "," ) );
    QString trksToken = QString( "file_join.track IN (%1)" ).arg( trksl.join( "," ) );
    QString albsToken = QString( "file_join.album IN (%1)" ).arg( albsl.join( "," ) );

    QString sql = QString( "SELECT "
                            "url, mtime, size, md5, mimetype, duration, bitrate, file_join.artist, file_join.album, file_join.track, "
                            "artist.name as artname, "
                            "album.name as albname, "
                            "track.name as trkname, "
                            "file.source, "
                            "file_join.albumpos, "
                            "artist.id as artid, "
                            "album.id as albid "
                            "FROM file, file_join, artist, track "
                            "LEFT JOIN album ON album.id = file_join.album "
                            "WHERE "
                            "artist.id = file_join.artist AND "
                            "track.id = file_join.track AND "
                            "file.id = file_join.file AND "
                            "%1" )
                        .arg( tracks.length() > 0 ? trksToken : QString( "0" ) );

    files_query.prepare( sql );
    files_query.exec();

    while( files_query.next() )
    {
        source_ptr s;
        QString url = files_query.value( 0 ).toString();

        if ( files_query.value( 13 ).toUInt() == 0 )
        {
            s = SourceList::instance()->getLocal();
        }
        else
        {
            s = SourceList::instance()->get( files_query.value( 13 ).toUInt() );
            if( s.isNull() )
            {
                qDebug() << "Could not find source" << files_query.value( 13 ).toUInt();
                continue;
            }

            url = QString( "servent://%1\t%2" ).arg( s->userName() ).arg( url );
        }

        Tomahawk::result_ptr result = Tomahawk::Result::get( url );
        Tomahawk::artist_ptr artist = Tomahawk::Artist::get( files_query.value( 15 ).toUInt(), files_query.value( 10 ).toString() );
        Tomahawk::album_ptr album = Tomahawk::Album::get( files_query.value( 16 ).toUInt(), files_query.value( 11 ).toString(), artist );

        result->setModificationTime( files_query.value( 1 ).toUInt() );
        result->setSize( files_query.value( 2 ).toUInt() );
        result->setMimetype( files_query.value( 4 ).toString() );
        result->setDuration( files_query.value( 5 ).toUInt() );
        result->setBitrate( files_query.value( 6 ).toUInt() );
        result->setArtist( artist );
        result->setAlbum( album );
        result->setTrack( files_query.value( 12 ).toString() );
        result->setRID( uuid() );
        result->setAlbumPos( files_query.value( 14 ).toUInt() );
        result->setId( files_query.value( 9 ).toUInt() );
        result->setYear( files_query.value( 17 ).toUInt() );

        for ( int k = 0; k < tracks.count(); k++ )
        {
            if ( tracks.at( k ).first == (int)result->dbid() )
            {
                result->setScore( tracks.at( k ).second );
                break;
            }
        }

        TomahawkSqlQuery attrQuery = lib->newquery();
        QVariantMap attr;

        attrQuery.prepare( "SELECT k, v FROM track_attributes WHERE id = ?" );
        attrQuery.bindValue( 0, result->dbid() );
        attrQuery.exec();
        while ( attrQuery.next() )
        {
            attr[ attrQuery.value( 0 ).toString() ] = attrQuery.value( 1 ).toString();
        }

        result->setAttributes( attr );

        result->setCollection( s->collection() );
        res << result;
    }

    emit results( m_query->id(), res );
}
