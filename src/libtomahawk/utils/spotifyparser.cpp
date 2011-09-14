/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
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

#include "spotifyparser.h"

#include "utils/logger.h"
#include "utils/tomahawkutils.h"
#include "query.h"
#include "sourcelist.h"
#include <qjson/parser.h>

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>

using namespace Tomahawk;

SpotifyParser::SpotifyParser( const QStringList& Urls, bool createNewPlaylist, QObject* parent )
    : QObject ( parent )
    , m_single( false )
    , m_trackMode( true )
    , m_createNewPlaylist( createNewPlaylist )

{
    foreach ( const QString& url, Urls )
        lookupUrl( url );
}

SpotifyParser::SpotifyParser( const QString& Url, bool createNewPlaylist, QObject* parent )
    : QObject ( parent )
    , m_single( true )
    , m_trackMode( true )
    , m_createNewPlaylist( createNewPlaylist )
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
    else if( link.contains( "playlist" ) )
    {
        m_trackMode = false;
        lookupPlaylist( link );
    }
    else
        return; // We only support tracks and playlists

}


void
SpotifyParser::lookupPlaylist( const QString& link )
{
    if ( !link.contains( "spotify:user:" ) ) // we only support playlist here
        return;

    tLog() << "Parsing Spotify Playlist URI:" << link;
    QUrl url = QUrl( QString( SPOTIFY_PLAYLIST_API_URL "/playlist/%1" ).arg( link ) );
    tDebug() << "Looking up URL..." << url.toString();

    QNetworkReply* reply = TomahawkUtils::nam()->get( QNetworkRequest( url ) );
    connect( reply, SIGNAL( finished() ), this, SLOT( spotifyPlaylistLookupFinished() ) );

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
    tLog() << "Looking up spotify track information..." << url.toString();

    QNetworkReply* reply = TomahawkUtils::nam()->get( QNetworkRequest( url ) );
    connect( reply, SIGNAL( finished() ), this, SLOT( spotifyTrackLookupFinished() ) );

    m_queries.insert( reply );

}


void
SpotifyParser::spotifyPlaylistLookupFinished()
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
            tLog() << "Failed to parse json from Spotify playlist lookup:" << p.errorString() << "On line" << p.errorLine();
            checkTrackFinished();
            return;
        }
        else if ( !res.contains( "tracks" ) )
        {
            tLog() << "No tracks' item in the spotify playlist lookup result... not doing anything";
            checkTrackFinished();
            return;
        }

        QVariantList trackResponse = res.value( "tracks" ).toList();
        if ( !trackResponse.isEmpty() )
        {
           m_title = res.value( "title" ).toString();
           m_creator = res.value( "creator" ).toString();
           qDebug() << "playlist owner: " << m_creator;
        }

        foreach( QVariant track, trackResponse )
        {
            lookupTrack( track.toString() );
        }

    } else
    {
        tLog() << "Error in network request to Spotify for track decoding:" << r->errorString();
    }

    checkPlaylistFinished();
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
        checkPlaylistFinished();
}

void
SpotifyParser::checkPlaylistFinished()
{
    if ( m_queries.isEmpty() ) // we're done
    {
        if( m_createNewPlaylist )
            m_playlist = Playlist::create( SourceList::instance()->getLocal(),
                                       uuid(),
                                       m_title,
                                       m_info,
                                       m_creator,
                                       false,
                                       m_tracks );

        else if ( !m_tracks.isEmpty() && !m_createNewPlaylist)
            emit tracks( m_tracks );

        deleteLater();
    }

}

void
SpotifyParser::checkTrackFinished()
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
