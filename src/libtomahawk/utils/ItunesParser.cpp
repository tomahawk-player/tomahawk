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

#include "ItunesParser.h"

#include <QtNetwork/QNetworkAccessManager>
#include <QRegExp>

#include <qjson/parser.h>

#include "Query.h"
#include "SourceList.h"
#include "jobview/JobStatusView.h"
#include "jobview/JobStatusModel.h"
#include "jobview/ErrorStatusMessage.h"
#include "utils/NetworkReply.h"
#include "utils/TomahawkUtils.h"
#include "utils/Logger.h"

using namespace Tomahawk;

QPixmap* ItunesParser::s_pixmap = 0;


ItunesParser::ItunesParser( const QStringList& urls, QObject* parent )
    : QObject ( parent )
    , m_single( false )
{
    foreach ( const QString& url, urls )
    {
        lookupItunesUri( url );
    }
}


ItunesParser::ItunesParser( const QString& Url, QObject* parent )
    : QObject ( parent )
    , m_single( true )
{
    lookupItunesUri( Url );
}


ItunesParser::~ItunesParser()
{
}


void
ItunesParser::lookupItunesUri( const QString& link )
{
    // Itunes uri parsing, using regex
    // (\d+)(?:\?i=*)(\d+) = AlbumId and trackId
    // (\d+)(?:\s*) = id
    QRegExp rxAlbumTrack( "(\\d+)(?:\\?i=*)(\\d+)" );
    QRegExp rxId( "(\\d+)(?:\\s*)" );
    QString id, trackId;

    // Doing a parse on regex in 2 stages,
    // first, look if we have both album and track id
    int pos = rxAlbumTrack.indexIn( link );
    if ( pos > -1 )
    {
        id = rxAlbumTrack.cap( 1 );
        trackId = rxAlbumTrack.cap( 2 );
    }
    else
    {
        // Second, if we dont have trackId, check for just Id
        int pos = rxId.indexIn( link );
        if ( pos > -1 )
        {
            id = rxId.cap( 1 );
        }
        else
            return;
    }

    QUrl url;
    DropJob::DropType type;
    if ( link.contains( "artist" ) )
    {
        type = DropJob::Artist;
        url = QUrl( QString( "http://ax.phobos.apple.com.edgesuite.net/WebObjects/MZStoreServices.woa/wa/wsLookup?id=%1&entity=song&limit=30" ).arg( id ) );
    }
    else
    {
        type = ( trackId.isEmpty() ? DropJob::Album : DropJob::Track );
        url = QUrl( QString( "http://ax.phobos.apple.com.edgesuite.net/WebObjects/MZStoreServices.woa/wa/wsLookup?id=%1&entity=song" ).arg( ( trackId.isEmpty() ? id : trackId ) ) );
    }

    NetworkReply* reply = new NetworkReply( TomahawkUtils::nam()->get( QNetworkRequest( url ) ) );
    connect( reply, SIGNAL( finished() ), SLOT( itunesResponseLookupFinished() ) );

#ifndef ENABLE_HEADLESS
    DropJobNotifier* j = new DropJobNotifier( pixmap(), QString( "Itunes" ), type, reply );
    JobStatusView::instance()->model()->addJob( j );
#endif

    m_queries.insert( reply );
}


void
ItunesParser::itunesResponseLookupFinished()
{
    NetworkReply* r = qobject_cast< NetworkReply* >( sender() );
    Q_ASSERT( r );
    m_queries.remove( r );
    r->deleteLater();

    if ( r->reply()->error() == QNetworkReply::NoError )
    {
        QJson::Parser p;
        bool ok;
        QVariantMap res = p.parse( r->reply(), &ok ).toMap();

        if ( !ok )
        {
            tLog() << "Failed to parse json from Spotify track lookup:" << p.errorString() << "On line" << p.errorLine();
            checkTrackFinished();
            return;
        }
        else if ( !res.contains( "results" ) )
        {
            tLog() << "No 'results' item in the itunes track lookup result... not doing anything";
            checkTrackFinished();
            return;
        }
        QVariantList itunesResponse = res.value( "results" ).toList();

        foreach ( QVariant itune, itunesResponse )
        {
            QString title, artist, album;
            QVariantMap ituneMap = itune.toMap();

            if ( ituneMap.value( "wrapperType" ).toString().contains( "track" ) )
            {
                title = ituneMap.value( "trackName" ).toString();
                artist = ituneMap.value( "artistName" ).toString();
                album = ituneMap.value( "collectionName" ).toString();
                if ( title.isEmpty() && artist.isEmpty() ) // don't have enough...
                {
                    tLog() << "Didn't get an artist and track name from itunes, not enough to build a query on. Aborting" << title << artist << album;
                }
                else
                {
                    Tomahawk::query_ptr q = Tomahawk::Query::get( artist, title, album, uuid(), true );
                    if ( q.isNull() )
                        continue;

                    m_tracks << q;
                }
            }
        }
    }
    else
    {
#ifndef ENABLE_HEADLESS
        JobStatusView::instance()->model()->addJob( new ErrorStatusMessage( tr( "Error fetching iTunes information from the network!" ) ) );
#endif
        tLog() << "Error in network request to Itunes for track decoding:" << r->reply()->errorString();
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


QPixmap
ItunesParser::pixmap() const
{
    if ( !s_pixmap )
        s_pixmap = new QPixmap( RESPATH "images/itunes.png" );

    return *s_pixmap;
}
