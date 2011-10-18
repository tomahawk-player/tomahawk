/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2010-2011, Hugo Lindström <hugolm84@gmail.com>
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

#include "spotifyparser.h"

#include "utils/logger.h"
#include "utils/tomahawkutils.h"
#include "query.h"
#include "sourcelist.h"
#include "dropjob.h"
#include "jobview/JobStatusView.h"
#include "jobview/JobStatusModel.h"
#include "dropjobnotifier.h"
#include "viewmanager.h"

#include <qjson/parser.h>

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>

using namespace Tomahawk;

QPixmap* SpotifyParser::s_pixmap = 0;

SpotifyParser::SpotifyParser( const QStringList& Urls, bool createNewPlaylist, QObject* parent )
    : QObject ( parent )
    , m_single( false )
    , m_trackMode( true )
    , m_createNewPlaylist( createNewPlaylist )
    , m_limit ( 40 )
    , m_browseJob( 0 )

{
    foreach ( const QString& url, Urls )
        lookupUrl( url );
}

SpotifyParser::SpotifyParser( const QString& Url, bool createNewPlaylist, QObject* parent )
    : QObject ( parent )
    , m_single( true )
    , m_trackMode( true )
    , m_createNewPlaylist( createNewPlaylist )
    , m_limit ( 40 )
    , m_browseJob( 0 )
{
    lookupUrl( Url );
}

SpotifyParser::~SpotifyParser()
{
}


void
SpotifyParser::lookupUrl( const QString& link )
{
    if( link.contains( "track" ) )
    {
        m_trackMode = true;
        lookupTrack( link );
    }
    else if( link.contains( "playlist" ) ||  link.contains( "album" ) || link.contains( "artist" ) )
    {
        if( !m_createNewPlaylist )
            m_trackMode = true;
        else
            m_trackMode = false;

        lookupSpotifyBrowse( link );
    }
    else
        return; // Not valid spotify item

}


void
SpotifyParser::lookupSpotifyBrowse( const QString& linkRaw )
{
    tLog() << "Parsing Spotify Browse URI:" << linkRaw;
    QString browseUri = linkRaw;
    if ( browseUri.contains( "open.spotify.com/" ) ) // convert to a URI
    {
        browseUri.replace( "http://open.spotify.com/", "" );
        browseUri.replace( "/", ":" );
        browseUri = "spotify:" + browseUri;
    }


    DropJob::DropType type;

    if ( browseUri.contains( "spotify:user" ) )
        type = DropJob::Playlist;
    if ( browseUri.contains( "spotify:artist" ) )
        type = DropJob::Artist;
    if ( browseUri.contains( "spotify:album" ) )
        type = DropJob::Album;
    if ( browseUri.contains( "spotify:track" ) )
        type = DropJob::Track;

    QUrl url;

    if( type != DropJob::Artist )
         url = QUrl( QString( SPOTIFY_PLAYLIST_API_URL "/browse/%1" ).arg( browseUri ) );
    else
         url = QUrl( QString( SPOTIFY_PLAYLIST_API_URL "/browse/%1/%2" ).arg( browseUri )
                                                                        .arg ( m_limit ) );
    tDebug() << "Looking up URL..." << url.toString();

    QNetworkReply* reply = TomahawkUtils::nam()->get( QNetworkRequest( url ) );
    connect( reply, SIGNAL( finished() ), this, SLOT( spotifyBrowseFinished() ) );

    m_browseJob = new DropJobNotifier( pixmap(), "Spotify", type, reply );
    JobStatusView::instance()->model()->addJob( m_browseJob );

    m_queries.insert( reply );
}

void
SpotifyParser::lookupTrack( const QString& link )
{
    tDebug() << "Got a QString " << link;
    if ( !link.contains( "track" )) // we only support track links atm
        return;

    // we need Spotify URIs such as spotify:track:XXXXXX, so if we by chance get a http://open.spotify.com url, convert it
    QString uri = link;
    if ( link.contains( "open.spotify.com" ) )
    {
        QString hash = link;
        hash.replace( "http://open.spotify.com/track/", "" );
        uri = QString( "spotify:track:%1" ).arg( hash );
    }

    QUrl url = QUrl( QString( "http://ws.spotify.com/lookup/1/.json?uri=%1" ).arg( uri ) );
    tDebug() << "Looking up URL..." << url.toString();

    QNetworkReply* reply = TomahawkUtils::nam()->get( QNetworkRequest( url ) );
    connect( reply, SIGNAL( finished() ), this, SLOT( spotifyTrackLookupFinished() ) );

    DropJobNotifier* j = new DropJobNotifier( pixmap(), QString( "Spotify" ), DropJob::Track, reply );
    JobStatusView::instance()->model()->addJob( j );

    m_queries.insert( reply );

}


void
SpotifyParser::spotifyBrowseFinished()
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
            tLog() << "Failed to parse json from Spotify browse item :" << p.errorString() << "On line" << p.errorLine();
            checkTrackFinished();
            return;
        }

        QVariantMap resultResponse = res.value( res.value( "type" ).toString() ).toMap();


        if ( !resultResponse.isEmpty() )
        {
            m_title = resultResponse.value( "name" ).toString();
            m_single = false;

            if ( res.value( "type" ).toString() == "playlist" )
                m_creator = resultResponse.value( "creator" ).toString();

            // TODO for now only take the first artist
            foreach ( QVariant result, resultResponse.value( "result" ).toList() )
            {
                QVariantMap trackResult = result.toMap();

                QString title, artist, album;

                title = trackResult.value( "title", QString() ).toString();
                artist = trackResult.value( "artist", QString() ).toString();
                album = trackResult.value( "album", QString() ).toString();

                if ( title.isEmpty() && artist.isEmpty() ) // don't have enough...
                {
                    tLog() << "Didn't get an artist and track name from spotify, not enough to build a query on. Aborting" << title << artist << album;
                    return;
                }

                Tomahawk::query_ptr q = Tomahawk::Query::get( artist, title, album, uuid(), m_trackMode );
                m_tracks << q;
            }

        }

    } else
    {
        tLog() << "Error in network request to Spotify for track decoding:" << r->errorString();
    }

    if ( m_trackMode )
        checkTrackFinished();
    else
        checkBrowseFinished();
}



void
SpotifyParser::spotifyTrackLookupFinished()
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
        } else if ( !res.contains( "track" ) )
        {
            tLog() << "No 'track' item in the spotify track lookup result... not doing anything";
            checkTrackFinished();
            return;
        }

        // lets parse this baby
        QVariantMap t = res.value( "track" ).toMap();

        QString title, artist, album;

        title = t.value( "name", QString() ).toString();
        // TODO for now only take the first artist
        if ( t.contains( "artists" ) && t[ "artists" ].canConvert< QVariantList >() && t[ "artists" ].toList().size() > 0 )
            artist = t[ "artists" ].toList().first().toMap().value( "name", QString() ).toString();
        if ( t.contains( "album" ) && t[ "album" ].canConvert< QVariantMap >() )
            album = t[ "album" ].toMap().value( "name", QString() ).toString();

        if ( title.isEmpty() && artist.isEmpty() ) // don't have enough...
        {
            tLog() << "Didn't get an artist and track name from spotify, not enough to build a query on. Aborting" << title << artist << album;
            return;
        }

        Tomahawk::query_ptr q = Tomahawk::Query::get( artist, title, album, uuid(), m_trackMode );
        m_tracks << q;
    } else
    {
        tLog() << "Error in network request to Spotify for track decoding:" << r->errorString();
    }

    if ( m_trackMode )
        checkTrackFinished();
    else
        checkBrowseFinished();

}

void
SpotifyParser::checkBrowseFinished()
{
    tDebug() << "Checking for spotify batch playlist job finished" << m_queries.isEmpty() << m_createNewPlaylist;
    if ( m_queries.isEmpty() ) // we're done
    {
        if ( m_browseJob )
            m_browseJob->setFinished();

        if( m_createNewPlaylist && !m_tracks.isEmpty() )
        {
            m_playlist = Playlist::create( SourceList::instance()->getLocal(),
                                       uuid(),
                                       m_title,
                                       m_info,
                                       m_creator,
                                       false,
                                       m_tracks );
            connect( m_playlist.data(), SIGNAL( revisionLoaded( Tomahawk::PlaylistRevision ) ), this, SLOT( playlistCreated() ) );
            return;
        }

        else if ( m_single && !m_tracks.isEmpty() )
            emit track( m_tracks.first() );
        else if ( !m_single && !m_tracks.isEmpty() )
            emit tracks( m_tracks );

        deleteLater();
    }
}

void
SpotifyParser::checkTrackFinished()
{
    tDebug() << "Checking for spotify batch track job finished" << m_queries.isEmpty();
    if ( m_queries.isEmpty() ) // we're done
    {
        if ( m_browseJob )
            m_browseJob->setFinished();

        if ( m_single && !m_tracks.isEmpty() )
            emit track( m_tracks.first() );
        else if ( !m_single && !m_tracks.isEmpty() )
            emit tracks( m_tracks );

        deleteLater();
    }

}

void
SpotifyParser::playlistCreated()
{

    ViewManager::instance()->show( m_playlist );

    deleteLater();
}


QPixmap
SpotifyParser::pixmap() const
{
    if ( !s_pixmap )
        s_pixmap = new QPixmap( RESPATH "images/spotify-logo.png" );

    return *s_pixmap;
}
