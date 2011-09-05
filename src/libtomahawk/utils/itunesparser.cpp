/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Written by Hugo Lindström <hugolm84@gmail.com>
 *   But based on Leo Franchi's work from spotifyParser
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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

#include "itunesparser.h"

#include "utils/logger.h"
#include "utils/tomahawkutils.h"
#include "query.h"
#include "sourcelist.h"
#include <qjson/parser.h>

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QRegExp>

using namespace Tomahawk;

ItunesParser::ItunesParser( const QStringList& Urls, QObject* parent, bool createNewPlaylist)
    : QObject ( parent )
    , m_single( false )

{
    m_createNewPlaylist = createNewPlaylist;
    foreach ( const QString& url, Urls )
        lookupUrl( url );
}

ItunesParser::ItunesParser( const QString& Url, QObject* parent, bool createNewPlaylist )
    : QObject ( parent )
    , m_single( true )
{
    m_createNewPlaylist = createNewPlaylist;
    lookupUrl( Url );
}

ItunesParser::~ItunesParser()
{

}


void
ItunesParser::lookupUrl( const QString& link )
{
    qDebug() << Q_FUNC_INFO;
    if( link.contains( "album" ) )// && link.contains( "?i="))
        lookupTrack(link);

    else return; // We only support tracks and playlists

}

void
ItunesParser::lookupTrack( const QString& link )
{

    tDebug() << "Got a QString " << link;
    if ( !link.contains( "album" ) ) //&& !link.contains( "?i=" )) // we only support track links atm
        return;

    // Itunes uri parsing, using regex
    // (\d+)(?:\?i=*)(\d+) = AlbumId and trackId
    // (\d+)(?:\s*) = albumId
    QRegExp rxAlbumTrack( "(\\d+)(?:\\?i=*)(\\d+)" );
    QRegExp rxAlbum( "(\\d+)(?:\\s*)" );
    QString albumId, trackId;

    // Doing a parse on regex in 2 stages,
    // first, look if we have both album and track id
     int pos = rxAlbumTrack.indexIn(link);

     if (pos > -1) {
         albumId = rxAlbumTrack.cap(1);
         trackId = rxAlbumTrack.cap(2);
     }else{

         // Second, if we dont have trackId, check for just albumid
         int pos = rxAlbum.indexIn(link);
         if (pos > -1) {
             albumId = rxAlbum.cap(1);
         }else return;

     }

    qDebug() << "Got Itunes link with Albumid " << albumId << "and trackid " <<trackId;
    tLog() << "Parsing itunes track:" << link;

    QUrl url = QUrl( QString( "http://ax.phobos.apple.com.edgesuite.net/WebObjects/MZStoreServices.woa/wa/wsLookup?id=%1&entity=song" ).arg( ( trackId.isEmpty() ? albumId : trackId ) ) );
    tDebug() << "Looking up..." << url.toString();

    QNetworkReply* reply = TomahawkUtils::nam()->get( QNetworkRequest( url ) );
    connect( reply, SIGNAL( finished() ), this, SLOT( itunesTrackLookupFinished() ) );

    m_queries.insert( reply );

}
void
ItunesParser::itunesTrackLookupFinished()
{
    QNetworkReply* r = qobject_cast< QNetworkReply* >( sender() );
    Q_ASSERT( r );
    m_queries.remove( r );
    r->deleteLater();

    if ( r->error() == QNetworkReply::NoError )
    {
        QJson::Parser p;
        bool ok;
        QVariantMap res = p.parse( r, &ok ).toMap();

        if ( !ok )
        {
            tLog() << "Failed to parse json from Spotify track lookup:" << p.errorString() << "On line" << p.errorLine();
            checkTrackFinished();
            return;
        } else if ( !res.contains( "results" ) )
        {
            tLog() << "No 'results' item in the itunes track lookup result... not doing anything";
            checkTrackFinished();
            return;
        }
        QVariantList itunesResponse = res.value( "results" ).toList();

        foreach(QVariant itune, itunesResponse){
            QString title, artist, album;
            if( !itune.toMap().value( "wrapperType" ).toString().contains( "track" ) )
                continue;

            title = itune.toMap().value( "trackName" ).toString();
            artist = itune.toMap().value( "artistName" ).toString();
            album = itune.toMap().value( "collectionName" ).toString();
            if ( title.isEmpty() && artist.isEmpty() ) // don't have enough...
            {
                tLog() << "Didn't get an artist and track name from itunes, not enough to build a query on. Aborting" << title << artist << album;

            }else{

                Tomahawk::query_ptr q = Tomahawk::Query::get( artist, title, album, uuid(), true );
                m_tracks << q;
            }
        }


    } else
    {
        tLog() << "Error in network request to Spotify for track decoding:" << r->errorString();
    }

    checkTrackFinished();
}



void
ItunesParser::checkTrackFinished()
{
    if ( m_queries.isEmpty() ) // we're done
    {
        if ( m_single && !m_tracks.isEmpty() )
            emit track( m_tracks.first() );
        else if ( !m_single && !m_tracks.isEmpty() )
            emit tracks( m_tracks );

        deleteLater();
    }

}
