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

#include "DatabaseCommand_Resolve.h"

#include "utils/Logger.h"

#include "Album.h"
#include "Artist.h"
#include "Pipeline.h"
#include "PlaylistEntry.h"
#include "SourceList.h"
#include "Track.h"

using namespace Tomahawk;


DatabaseCommand_Resolve::DatabaseCommand_Resolve( const query_ptr& query )
    : DatabaseCommand()
    , m_query( query )
{
    // FIXME: We need to run tests of this DbCmd without a Pipeline
    // Q_ASSERT( Pipeline::instance()->isRunning() );
}


DatabaseCommand_Resolve::~DatabaseCommand_Resolve()
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
        tDebug() << "Using result-hint to speed up resolving:" << m_query->resultHint();

        Tomahawk::result_ptr result = lib->resultFromHint( m_query );
        if ( result && ( !result->resolvedByCollection() || result->resolvedByCollection()->isOnline() ) )
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

    // STEP 1
    QList< QPair<int, float> > tracks = lib->search( m_query );

    if ( tracks.isEmpty() )
    {
        qDebug() << "No candidates found in first pass, aborting resolve" << m_query->queryTrack()->toString();
        emit results( m_query->id(), res );
        return;
    }

    // STEP 2
    TomahawkSqlQuery files_query = lib->newquery();

    QStringList trksl;
    for ( int k = 0; k < tracks.count(); k++ )
        trksl.append( QString::number( tracks.at( k ).first ) );

    QString trksToken = QString( "file_join.track IN (%1)" ).arg( trksl.join( "," ) );

    QString sql = QString( "SELECT "
                            "url, mtime, size, md5, mimetype, duration, bitrate, "  //0
                            "file_join.artist, file_join.album, file_join.track, "  //7
                            "file_join.composer, file_join.discnumber, "            //10
                            "artist.name as artname, "                              //12
                            "album.name as albname, "                               //13
                            "track.name as trkname, "                               //14
                            "composer.name as cmpname, "                            //15
                            "file.source, "                                         //16
                            "file_join.albumpos, "                                  //17
                            "artist.id as artid, "                                  //18
                            "album.id as albid, "                                   //19
                            "composer.id as cmpid, "                                //20
                            "albumArtist.id as albumartistid, "                     //21
                            "albumArtist.name as albumartistname "                  //22
                            "FROM file, file_join, artist, track "
                            "LEFT JOIN album ON album.id = file_join.album "
                            "LEFT JOIN artist AS composer ON composer.id = file_join.composer "
                            "LEFT JOIN artist AS albumArtist ON albumArtist.id = album.artist "
                            "WHERE "
                            "artist.id = file_join.artist AND "
                            "track.id = file_join.track AND "
                            "file.id = file_join.file AND "
                            "(%1)" )
         .arg( trksToken );

    files_query.prepare( sql );
    files_query.exec();

    while ( files_query.next() )
    {
        QString url = files_query.value( 0 ).toString();
        source_ptr s = SourceList::instance()->get( files_query.value( 16 ).toUInt() );
        if ( !s )
        {
            tDebug() << "Could not find source" << files_query.value( 16 ).toUInt();
            continue;
        }
        if ( !s->isLocal() )
            url = QString( "servent://%1\t%2" ).arg( s->nodeId() ).arg( url );

        Tomahawk::result_ptr result = Tomahawk::Result::getCached( url );
        if ( result )
        {
            tDebug( LOGVERBOSE ) << "Result already cached:" << result->toString();
            res << result;
            continue;
        }

        track_ptr track = Track::get( files_query.value( 9 ).toUInt(), files_query.value( 12 ).toString(), files_query.value( 14 ).toString(),
                                      files_query.value( 13 ).toString(), files_query.value( 22 ).toString(), files_query.value( 5 ).toUInt(),
                                      files_query.value( 15 ).toString(), files_query.value( 17 ).toUInt(), files_query.value( 11 ).toUInt() );
        if ( !track )
            continue;
        track->loadAttributes();

        result = Result::get( url, track );
        if ( !result )
            continue;

        result->setModificationTime( files_query.value( 1 ).toUInt() );
        result->setSize( files_query.value( 2 ).toUInt() );
        result->setMimetype( files_query.value( 4 ).toString() );
        result->setBitrate( files_query.value( 6 ).toUInt() );
        result->setRID( uuid() );
        result->setResolvedByCollection( s->dbCollection() );

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
    QList< QPair<int, float> > trackPairs = lib->search( m_query );
    QList< QPair<int, float> > albumPairs = lib->searchAlbum( m_query, 20 );

    TomahawkSqlQuery query = lib->newquery();
    query.prepare( "SELECT album.name, artist.id, artist.name FROM album, artist WHERE artist.id = album.artist AND album.id = ?" );

    foreach ( const scorepair_t& albumPair, albumPairs )
    {
        query.bindValue( 0, albumPair.first );
        query.exec();

        QList<Tomahawk::album_ptr> albumList;
        while ( query.next() )
        {
            Tomahawk::artist_ptr artist = Tomahawk::Artist::get( query.value( 1 ).toUInt(), query.value( 2 ).toString() );
            Tomahawk::album_ptr album = Tomahawk::Album::get( albumPair.first, query.value( 0 ).toString(), artist );
            albumList << album;
        }

        emit albums( m_query->id(), albumList );
    }

    if ( trackPairs.isEmpty() )
    {
        qDebug() << "No candidates found in first pass, aborting resolve" << m_query->fullTextQuery();
        emit results( m_query->id(), res );
        return;
    }

    // STEP 2
    TomahawkSqlQuery files_query = lib->newquery();

    QStringList trksl;
    for ( int k = 0; k < trackPairs.count(); k++ )
        trksl.append( QString::number( trackPairs.at( k ).first ) );

    QString trksToken = QString( "file_join.track IN (%1)" ).arg( trksl.join( "," ) );
    QString sql = QString( "SELECT "
                            "url, mtime, size, md5, mimetype, duration, bitrate, "  //0
                            "file_join.artist, file_join.album, file_join.track, "  //7
                            "file_join.composer, file_join.discnumber, "            //10
                            "artist.name as artname, "                              //12
                            "album.name as albname, "                               //13
                            "track.name as trkname, "                               //14
                            "composer.name as cmpname, "                            //15
                            "file.source, "                                         //16
                            "file_join.albumpos, "                                  //17
                            "artist.id as artid, "                                  //18
                            "album.id as albid, "                                   //19
                            "composer.id as cmpid, "                                //20
                            "albumArtist.id as albumartistid, "                     //21
                            "albumArtist.name as albumartistname "                  //22
                            "FROM file, file_join, artist, track "
                            "LEFT JOIN album ON album.id = file_join.album "
                            "LEFT JOIN artist AS composer ON composer.id = file_join.composer "
                            "LEFT JOIN artist AS albumArtist ON albumArtist.id = album.artist "
                            "WHERE "
                            "artist.id = file_join.artist AND "
                            "track.id = file_join.track AND "
                            "file.id = file_join.file AND "
                            "%1" )
                        .arg( !trksl.isEmpty() ? trksToken : QString( "0" ) );

    files_query.prepare( sql );
    files_query.exec();

    while ( files_query.next() )
    {
        QString url = files_query.value( 0 ).toString();
        source_ptr s = SourceList::instance()->get( files_query.value( 16 ).toUInt() );
        if ( !s )
        {
            tDebug() << "Could not find source" << files_query.value( 16 ).toUInt();
            continue;
        }
        if ( !s->isLocal() )
            url = QString( "servent://%1\t%2" ).arg( s->nodeId() ).arg( url );

        Tomahawk::result_ptr result = Tomahawk::Result::getCached( url );
        if ( result )
        {
            tDebug( LOGVERBOSE ) << "Result already cached:" << result->toString();
            res << result;
            continue;
        }

        track_ptr track = Track::get( files_query.value( 9 ).toUInt(), files_query.value( 12 ).toString(), files_query.value( 14 ).toString(),
                                      files_query.value( 13 ).toString(), files_query.value( 22 ).toString(), files_query.value( 5 ).toUInt(),
                                      files_query.value( 15 ).toString(), files_query.value( 17 ).toUInt(), files_query.value( 11 ).toUInt() );
        track->loadAttributes();

        result = Result::get( url, track );
        result->setModificationTime( files_query.value( 1 ).toUInt() );
        result->setSize( files_query.value( 2 ).toUInt() );
        result->setMimetype( files_query.value( 4 ).toString() );
        result->setBitrate( files_query.value( 6 ).toUInt() );
        result->setRID( uuid() );
        result->setResolvedByCollection( s->dbCollection() );

        res << result;
    }

    emit results( m_query->id(), res );
}
