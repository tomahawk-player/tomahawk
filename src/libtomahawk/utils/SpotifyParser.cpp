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

#include "SpotifyParser.h"

#include <QtNetwork/QNetworkAccessManager>

#include <qjson/parser.h>

#include "Query.h"
#include "SourceList.h"
#include "DropJob.h"
#include "DropJobNotifier.h"
#include "ViewManager.h"
#include "jobview/JobStatusView.h"
#include "jobview/JobStatusModel.h"
#include "jobview/ErrorStatusMessage.h"
#include "utils/NetworkReply.h"
#include "utils/TomahawkUtils.h"
#include "utils/Logger.h"

using namespace Tomahawk;

QPixmap* SpotifyParser::s_pixmap = 0;


SpotifyParser::SpotifyParser( const QStringList& Urls, bool createNewPlaylist, QObject* parent )
    : QObject ( parent )
    , m_limit ( 40 )
    , m_single( false )
    , m_trackMode( true )
    , m_collaborative( false )
    , m_createNewPlaylist( createNewPlaylist )
    , m_browseJob( 0 )
    , m_subscribers( 0 )
{
    foreach ( const QString& url, Urls )
        lookupUrl( url );
}


SpotifyParser::SpotifyParser( const QString& Url, bool createNewPlaylist, QObject* parent )
    : QObject ( parent )
    , m_limit ( 40 )
    , m_single( true )
    , m_trackMode( true )
    , m_collaborative( false )
    , m_createNewPlaylist( createNewPlaylist )
    , m_browseJob( 0 )
    , m_subscribers( 0 )
{
    lookupUrl( Url );
}


SpotifyParser::~SpotifyParser()
{
}


void
SpotifyParser::lookupUrl( const QString& rawLink )
{
    tLog() << "Looking up Spotify rawURI:" << rawLink;
    QString link = rawLink;

    QRegExp isHttp( "(?:((play|open)\\.spotify.com))(.*)" );

    // Some spotify apps contain the link to the playlist as url-encoded in their link (e.g. ShareMyPlaylists)
    if ( link.contains( "%253A" ) )
    {
        link = QUrl::fromPercentEncoding( link.toUtf8() );
    }

    if( link.contains( "%3A" ) )
    {
        link = QUrl::fromPercentEncoding( link.toUtf8() );
    }

    if( isHttp.indexIn( link, 0 ) != -1 )
    {
        link = "spotify"+isHttp.cap( 3 ).replace( "/", ":" );
    }

    // TODO: Ignoring search and user querys atm
    // (spotify:(?:(?:artist|album|track|user:[^:]+:playlist):[a-zA-Z0-9]+|user:[^:]+|search:(?:[-\w$\.+!*'(),<>:\s]+|%[a-fA-F0-9\s]{2})+))
    QRegExp rx( "(spotify:(?:(?:artist|album|track|user:[^:]+:playlist):[a-zA-Z0-9]+[^:]))" );
    if ( rx.indexIn( link, 0 ) != -1 )
    {
        link = rx.cap( 1 );
    }
    else
    {
        tLog() << "Bad SpotifyURI!" << link;
        return;
    }

    if ( link.contains( "track" ) )
    {
        m_trackMode = true;
        lookupTrack( link );
    }
    else if ( link.contains( "playlist" ) ||  link.contains( "album" ) || link.contains( "artist" ) )
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
SpotifyParser::lookupSpotifyBrowse( const QString& link )
{
    tLog() << "Parsing Spotify Browse URI:" << link;

    // Used in checkBrowseFinished as identifier
    m_browseUri = link;

    if ( m_browseUri.contains( "playlist" ) &&
         Tomahawk::Accounts::SpotifyAccount::instance() != 0 &&
         Tomahawk::Accounts::SpotifyAccount::instance()->loggedIn() )
    {
        // Do a playlist lookup locally
        // Running resolver, so do the lookup through that
        qDebug() << Q_FUNC_INFO << "Doing playlist lookup through spotify resolver:" << m_browseUri;
        QVariantMap message;
        message[ "_msgtype" ] = "playlistListing";
        message[ "id" ] = m_browseUri;

        QMetaObject::invokeMethod( Tomahawk::Accounts::SpotifyAccount::instance(), "sendMessage", Qt::QueuedConnection, Q_ARG( QVariantMap, message ),
                                                                                                                        Q_ARG( QObject*, this ),
                                                                                                                        Q_ARG( QString, "playlistListingResult" ) );

        return;
    }

    DropJob::DropType type;

    if ( m_browseUri.contains( "spotify:user" ) )
        type = DropJob::Playlist;
    if ( m_browseUri.contains( "spotify:artist" ) )
        type = DropJob::Artist;
    if ( m_browseUri.contains( "spotify:album" ) )
        type = DropJob::Album;
    if ( m_browseUri.contains( "spotify:track" ) )
        type = DropJob::Track;

    QUrl url;

    if ( type != DropJob::Artist )
         url = QUrl( QString( SPOTIFY_PLAYLIST_API_URL "/browse/%1" ).arg( m_browseUri ) );
    else
         url = QUrl( QString( SPOTIFY_PLAYLIST_API_URL "/browse/%1/%2" ).arg( m_browseUri )
                                                                        .arg ( m_limit ) );

    NetworkReply* reply = new NetworkReply( TomahawkUtils::nam()->get( QNetworkRequest( url ) ) );
    connect( reply, SIGNAL( finished() ), SLOT( spotifyBrowseFinished() ) );

#ifndef ENABLE_HEADLESS
    m_browseJob = new DropJobNotifier( pixmap(), "Spotify", type, reply );
    JobStatusView::instance()->model()->addJob( m_browseJob );
#endif

    m_queries.insert( reply );
}


void
SpotifyParser::lookupTrack( const QString& link )
{
    if ( !link.contains( "track" ) ) // we only support track links atm
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

    NetworkReply* reply = new NetworkReply( TomahawkUtils::nam()->get( QNetworkRequest( url ) ) );
    connect( reply, SIGNAL( finished() ), SLOT( spotifyTrackLookupFinished() ) );

#ifndef ENABLE_HEADLESS
    DropJobNotifier* j = new DropJobNotifier( pixmap(), QString( "Spotify" ), DropJob::Track, reply );
    JobStatusView::instance()->model()->addJob( j );
#endif

    m_queries.insert( reply );
}


void
SpotifyParser::spotifyBrowseFinished()
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
            tLog() << "Failed to parse json from Spotify browse item:" << p.errorString() << "On line" << p.errorLine();
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
                if ( q.isNull() )
                    continue;

                tLog() << "Setting resulthint to " << trackResult.value( "trackuri" );
                q->setResultHint( trackResult.value( "trackuri" ).toString() );
                q->setProperty( "annotation", trackResult.value( "trackuri" ).toString() );

                m_tracks << q;
            }
        }
    }
    else
    {
        JobStatusView::instance()->model()->addJob( new ErrorStatusMessage( tr( "Error fetching Spotify information from the network!" ) ) );
        tLog() << "Error in network request to Spotify for track decoding:" << r->reply()->errorString();
    }

    if ( m_trackMode )
        checkTrackFinished();
    else
        checkBrowseFinished();
}


void
SpotifyParser::spotifyTrackLookupFinished()
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
        else if ( !res.contains( "track" ) )
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
        if ( !q.isNull() )
        {
            q->setResultHint( t.value( "trackuri" ).toString() );

            m_tracks << q;
        }
    }
    else
    {
        tLog() << "Error in network request to Spotify for track decoding:" << r->reply()->errorString();
    }

    if ( m_trackMode )
        checkTrackFinished();
    else
        checkBrowseFinished();
}


void
SpotifyParser::playlistListingResult( const QString& msgType, const QVariantMap& msg, const QVariant& extraData )
{
    Q_UNUSED( extraData );

    Q_ASSERT( msgType == "playlistListing" );

    m_title = msg.value( "name" ).toString();
    m_single = false;
    m_creator = msg.value( "creator" ).toString();
    m_collaborative = msg.value( "collaborative" ).toBool();
    m_subscribers = msg.value( "subscribers" ).toInt();

    const QVariantList tracks = msg.value( "tracks" ).toList();
    foreach ( const QVariant& blob, tracks )
    {
        QVariantMap trackMap = blob.toMap();
        const query_ptr q = Query::get( trackMap.value( "artist" ).toString(), trackMap.value( "track" ).toString(), trackMap.value( "album" ).toString(), uuid(), false );

        if ( q.isNull() )
            continue;

        const QString id = trackMap.value( "id" ).toString();
        if( !id.isEmpty() )
        {
            q->setResultHint( id );
            q->setProperty( "annotation", id );
        }

        m_tracks << q;
    }

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

        if ( m_createNewPlaylist && !m_tracks.isEmpty() )
        {
            QString spotifyUsername;
            bool spotifyAccountLoggedIn = Accounts::SpotifyAccount::instance() && Accounts::SpotifyAccount::instance()->loggedIn();

            if ( spotifyAccountLoggedIn )
            {
                QVariantHash creds = Accounts::SpotifyAccount::instance()->credentials();
                spotifyUsername = creds.value( "username" ).toString();
            }

            if ( spotifyAccountLoggedIn &&  Accounts::SpotifyAccount::instance()->hasPlaylist( m_browseUri ) )
            {
                // The playlist is already registered with Tomahawk, so just open it instead of adding another instance.
                m_playlist = Accounts::SpotifyAccount::instance()->playlistForURI( m_browseUri );
                playlistCreated();
            }
            else
            {
                m_playlist = Playlist::create( SourceList::instance()->getLocal(),
                                       uuid(),
                                       m_title,
                                       m_info,
                                       spotifyUsername == m_creator ? QString() : m_creator,
                                       false,
                                       m_tracks );

                connect( m_playlist.data(), SIGNAL( revisionLoaded( Tomahawk::PlaylistRevision ) ), this, SLOT( playlistCreated() ) );

                if ( spotifyAccountLoggedIn )
                {
                    SpotifyPlaylistUpdater* updater = new SpotifyPlaylistUpdater(
                                                        Accounts::SpotifyAccount::instance(), m_playlist->currentrevision(), m_browseUri, m_playlist );


                    // If the user isnt dropping a playlist the he owns, its subscribeable
                    if ( !m_browseUri.contains( spotifyUsername ) )
                        updater->setCanSubscribe( true );
                    else
                        updater->setOwner( true );

                    updater->setCollaborative( m_collaborative );
                    updater->setSubscribers( m_subscribers );
                    // Just register the infos
                    Accounts::SpotifyAccount::instance()->registerPlaylistInfo( m_title, m_browseUri, m_browseUri, false, false, updater->owner() );
                    Accounts::SpotifyAccount::instance()->registerUpdaterForPlaylist( m_browseUri, updater );
                    // On default, set the playlist as subscribed
                    if( !updater->owner() )
                        Accounts::SpotifyAccount::instance()->setSubscribedForPlaylist( m_playlist, true );

                }
            }
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
