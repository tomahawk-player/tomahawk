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

#include "lastfmplugin.h"

#include <QDir>
#include <QSettings>
#include <QNetworkConfiguration>
#include <QDomElement>

#include "album.h"
#include "typedefs.h"
#include "audio/audioengine.h"
#include "tomahawksettings.h"
#include "utils/tomahawkutils.h"
#include "utils/logger.h"

#include <lastfm/ws.h>
#include <lastfm/XmlQuery>

#include <qjson/parser.h>

using namespace Tomahawk::InfoSystem;


LastFmPlugin::LastFmPlugin()
    : InfoPlugin()
    , m_scrobbler( 0 )
{
    m_supportedGetTypes << InfoAlbumCoverArt << InfoArtistImages << InfoArtistSimilars << InfoArtistSongs << InfoChart << InfoChartCapabilities;
    m_supportedPushTypes << InfoSubmitScrobble << InfoSubmitNowPlaying << InfoLove << InfoUnLove;

    // Flush session key cache
    TomahawkSettings::instance()->setLastFmSessionKey( QByteArray() );

    lastfm::ws::ApiKey = "7194b85b6d1f424fe1668173a78c0c4a";
    lastfm::ws::SharedSecret = "ba80f1df6d27ae63e9cb1d33ccf2052f";
    lastfm::ws::Username = TomahawkSettings::instance()->lastFmUsername();
    lastfm::setNetworkAccessManager( TomahawkUtils::nam() );

    m_pw = TomahawkSettings::instance()->lastFmPassword();

    //HACK work around a bug in liblastfm---it doesn't create its config dir, so when it
    // tries to write the track cache, it fails silently. until we have a fixed version, do this
    // code taken from Amarok (src/services/lastfm/ScrobblerAdapter.cpp)
#ifdef Q_WS_X11
    QString lpath = QDir::home().filePath( ".local/share/Last.fm" );
    QDir ldir = QDir( lpath );
    if( !ldir.exists() )
    {
        ldir.mkpath( lpath );
    }
#endif

    m_badUrls << QUrl( "http://cdn.last.fm/flatness/catalogue/noimage" );

    connect( TomahawkSettings::instance(), SIGNAL( changed() ),
                                             SLOT( settingsChanged() ), Qt::QueuedConnection );

    QTimer::singleShot( 0, this, SLOT( settingsChanged() ) );
}


LastFmPlugin::~LastFmPlugin()
{
    qDebug() << Q_FUNC_INFO;
    delete m_scrobbler;
    m_scrobbler = 0;
}


void
LastFmPlugin::dataError( Tomahawk::InfoSystem::InfoRequestData requestData )
{
    emit info( requestData, QVariant() );
    return;
}


void
LastFmPlugin::getInfo( Tomahawk::InfoSystem::InfoRequestData requestData )
{
    switch ( requestData.type )
    {
        case InfoArtistImages:
            fetchArtistImages( requestData );
            break;

        case InfoAlbumCoverArt:
            fetchCoverArt( requestData );
            break;

        case InfoArtistSimilars:
            fetchSimilarArtists( requestData );
            break;

        case InfoArtistSongs:
            fetchTopTracks( requestData );
            break;

        case InfoChart:
            fetchChart( requestData );
            break;

        case InfoChartCapabilities:
            fetchChartCapabilities( requestData );
            break;
        default:
            dataError( requestData );
    }
}


void
LastFmPlugin::pushInfo( const QString caller, const Tomahawk::InfoSystem::InfoType type, const QVariant input )
{
    Q_UNUSED( caller )
    switch ( type )
    {
        case InfoSubmitNowPlaying:
            nowPlaying( input );
            break;

        case InfoSubmitScrobble:
            scrobble();
            break;

        case InfoLove:
        case InfoUnLove:
            sendLoveSong( type, input );
            break;

        default:
            return;
    }
}


void
LastFmPlugin::nowPlaying( const QVariant &input )
{
    if ( !input.canConvert< Tomahawk::InfoSystem::InfoStringHash >() || !m_scrobbler )
    {
        tLog() << "LastFmPlugin::nowPlaying no m_scrobbler, or cannot convert input!";
        if ( !m_scrobbler )
            tLog() << "No scrobbler!";
        return;
    }

    InfoStringHash hash = input.value< Tomahawk::InfoSystem::InfoStringHash >();
    if ( !hash.contains( "title" ) || !hash.contains( "artist" ) || !hash.contains( "album" ) || !hash.contains( "duration" ) )
        return;

    m_track = lastfm::MutableTrack();
    m_track.stamp();

    m_track.setTitle( hash["title"] );
    m_track.setArtist( hash["artist"] );
    m_track.setAlbum( hash["album"] );
    bool ok;
    m_track.setDuration( hash["duration"].toUInt( &ok ) );
    m_track.setSource( lastfm::Track::Player );

    m_scrobbler->nowPlaying( m_track );
}


void
LastFmPlugin::scrobble()
{
    if ( !m_scrobbler || m_track.isNull() )
        return;

    tLog() << Q_FUNC_INFO << "Scrobbling now:" << m_track.toString();
    m_scrobbler->cache( m_track );
    m_scrobbler->submit();
}


void
LastFmPlugin::sendLoveSong( const InfoType type, QVariant input )
{
    qDebug() << Q_FUNC_INFO;

    if ( !input.canConvert< Tomahawk::InfoSystem::InfoStringHash >() )
    {
        tLog() << "LastFmPlugin::nowPlaying cannot convert input!";
        return;
    }

    InfoStringHash hash = input.value< Tomahawk::InfoSystem::InfoStringHash >();
    if ( !hash.contains( "title" ) || !hash.contains( "artist" ) || !hash.contains( "album" ) )
        return;

    lastfm::MutableTrack track;
    track.stamp();

    track.setTitle( hash["title"] );
    track.setArtist( hash["artist"] );
    track.setAlbum( hash["album"] );
    bool ok;
    track.setDuration( hash["duration"].toUInt( &ok ) );
    track.setSource( lastfm::Track::Player );

    if ( type == Tomahawk::InfoSystem::InfoLove )
    {
        track.love();
    }
    else if ( type == Tomahawk::InfoSystem::InfoUnLove )
    {
        track.unlove();
    }
}


void
LastFmPlugin::fetchSimilarArtists( Tomahawk::InfoSystem::InfoRequestData requestData )
{
    if ( !requestData.input.canConvert< Tomahawk::InfoSystem::InfoStringHash >() )
    {
        dataError( requestData );
        return;
    }
    InfoStringHash hash = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash >();
    if ( !hash.contains( "artist" ) )
    {
        dataError( requestData );
        return;
    }

    Tomahawk::InfoSystem::InfoStringHash criteria;
    criteria["artist"] = hash["artist"];

    emit getCachedInfo( criteria, 2419200000, requestData );
}


void
LastFmPlugin::fetchTopTracks( Tomahawk::InfoSystem::InfoRequestData requestData )
{
    if ( !requestData.input.canConvert< Tomahawk::InfoSystem::InfoStringHash >() )
    {
        dataError( requestData );
        return;
    }
    InfoStringHash hash = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash >();
    if ( !hash.contains( "artist" ) )
    {
        dataError( requestData );
        return;
    }

    Tomahawk::InfoSystem::InfoStringHash criteria;
    criteria["artist"] = hash["artist"];

    emit getCachedInfo( criteria, 2419200000, requestData );
}

void
LastFmPlugin::fetchChart( Tomahawk::InfoSystem::InfoRequestData requestData )
{
    if ( !requestData.input.canConvert< Tomahawk::InfoSystem::InfoStringHash >() )
    {
        dataError( requestData );
        return;
    }
    InfoStringHash hash = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash >();
    Tomahawk::InfoSystem::InfoStringHash criteria;
    if ( !hash.contains( "chart_id" ) )
    {
        dataError( requestData );
        return;
    } else {
        criteria["chart_id"] = hash["chart_id"];
    }

    emit getCachedInfo( criteria, 0, requestData );
}

void
LastFmPlugin::fetchChartCapabilities( Tomahawk::InfoSystem::InfoRequestData requestData )
{
    if ( !requestData.input.canConvert< Tomahawk::InfoSystem::InfoStringHash >() )
    {
        dataError( requestData );
        return;
    }
    InfoStringHash hash = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash >();
    Tomahawk::InfoSystem::InfoStringHash criteria;

    emit getCachedInfo( criteria, 0, requestData );
}

void
LastFmPlugin::fetchCoverArt( Tomahawk::InfoSystem::InfoRequestData requestData )
{
    if ( !requestData.input.canConvert< Tomahawk::InfoSystem::InfoStringHash >() )
    {
        dataError( requestData );
        return;
    }
    InfoStringHash hash = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash >();
    if ( !hash.contains( "artist" ) || !hash.contains( "album" ) )
    {
        dataError( requestData );
        return;
    }

    Tomahawk::InfoSystem::InfoStringHash criteria;
    criteria["artist"] = hash["artist"];
    criteria["album"] = hash["album"];

    emit getCachedInfo( criteria, 2419200000, requestData );
}


void
LastFmPlugin::fetchArtistImages( Tomahawk::InfoSystem::InfoRequestData requestData )
{
    if ( !requestData.input.canConvert< Tomahawk::InfoSystem::InfoStringHash >() )
    {
        dataError( requestData );
        return;
    }
    InfoStringHash hash = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash >();
    if ( !hash.contains( "artist" ) )
    {
        dataError( requestData );
        return;
    }

    Tomahawk::InfoSystem::InfoStringHash criteria;
    criteria["artist"] = hash["artist"];

    emit getCachedInfo( criteria, 2419200000, requestData );
}


void
LastFmPlugin::notInCacheSlot( QHash<QString, QString> criteria, Tomahawk::InfoSystem::InfoRequestData requestData )
{
    if ( !TomahawkUtils::nam() )
    {
        tLog() << "Have a null QNAM, uh oh";
        emit info( requestData, QVariant() );
        return;
    }

    InfoStringHash hash = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash >();
    switch ( requestData.type )
    {
        case InfoChart:
        {
             /// We need something to check if the request is actually ment to go to this plugin
            if ( !hash.contains( "chart_source" ) )
            {
                dataError( requestData );
                break;
            }
            else
            {
                if( "last.fm" != hash["chart_source"] )
                {
                    dataError( requestData );
                    break;
                }

            }
            QMap<QString, QString> args;
            args["method"] = criteria["chart_id"];
            args["limit"] = "100";
            QNetworkReply* reply = lastfm::ws::get(args);
            reply->setProperty( "requestData", QVariant::fromValue< Tomahawk::InfoSystem::InfoRequestData >( requestData ) );

            connect( reply, SIGNAL( finished() ), SLOT( chartReturned() ) );
            return;
        }

        case InfoChartCapabilities:
        {
            QList< InfoStringHash > track_charts;
            InfoStringHash c;
            c[ "type" ] = "tracks";
            c[ "id" ] = "chart.getTopTracks";
            c[ "label" ] = tr( "Top Tracks" );
            track_charts.append( c );
            c[ "id" ] = "chart.getLovedTracks";
            c[ "label" ] = tr( "Loved Tracks" );
            track_charts.append( c );
            c[ "id" ] = "chart.getHypedTracks";
            c[ "label" ] = tr( "Hyped Tracks" );
            track_charts.append( c );

            QList< InfoStringHash > artist_charts;
            c[ "type" ] = "artists";
            c[ "id" ] = "chart.getTopArtists";
            c[ "label" ] = tr( "Top Artists" );
            artist_charts.append( c );
            c[ "id" ] = "chart.getHypedArtists";
            c[ "label" ] = tr( "Hyped Artists" );
            artist_charts.append( c );

            QVariantMap charts;
            charts.insert( "Tracks", QVariant::fromValue< QList< InfoStringHash > >( track_charts ) );
            charts.insert( "Artists", QVariant::fromValue< QList< InfoStringHash > >( artist_charts ) );

            QVariantMap result;
            result.insert( "Last.fm", QVariant::fromValue<QVariantMap>( charts ) );

            emit info( requestData, result );
            return;
        }

        case InfoArtistSimilars:
        {
            lastfm::Artist a( criteria["artist"] );
            QNetworkReply* reply = a.getSimilar();
            reply->setProperty( "requestData", QVariant::fromValue< Tomahawk::InfoSystem::InfoRequestData >( requestData ) );

            connect( reply, SIGNAL( finished() ), SLOT( similarArtistsReturned() ) );
            return;
        }

        case InfoArtistSongs:
        {
            lastfm::Artist a( criteria["artist"] );
            QNetworkReply* reply = a.getTopTracks();
            reply->setProperty( "requestData", QVariant::fromValue< Tomahawk::InfoSystem::InfoRequestData >( requestData ) );

            connect( reply, SIGNAL( finished() ), SLOT( topTracksReturned() ) );
            return;
        }

        case InfoAlbumCoverArt:
        {
            QString artistName = criteria["artist"];
            QString albumName = criteria["album"];

            QUrl imgurl( "http://ws.audioscrobbler.com/2.0/" );
            imgurl.addQueryItem( "method", "album.imageredirect" );
            imgurl.addQueryItem( "artist", artistName );
            imgurl.addQueryItem( "album", albumName );
            imgurl.addQueryItem( "autocorrect", QString::number( 1 ) );
            imgurl.addQueryItem( "size", "large" );
            imgurl.addQueryItem( "api_key", "7a90f6672a04b809ee309af169f34b8b" );

            QNetworkRequest req( imgurl );
            QNetworkReply* reply = TomahawkUtils::nam()->get( req );
            reply->setProperty( "requestData", QVariant::fromValue< Tomahawk::InfoSystem::InfoRequestData >( requestData ) );

            connect( reply, SIGNAL( finished() ), SLOT( coverArtReturned() ) );
            return;
        }

        case InfoArtistImages:
        {
            QString artistName = criteria["artist"];

            QUrl imgurl( "http://ws.audioscrobbler.com/2.0/" );
            imgurl.addQueryItem( "method", "artist.imageredirect" );
            imgurl.addQueryItem( "artist", artistName );
            imgurl.addQueryItem( "autocorrect", QString::number( 1 ) );
            imgurl.addQueryItem( "size", "large" );
            imgurl.addQueryItem( "api_key", "7a90f6672a04b809ee309af169f34b8b" );

            QNetworkRequest req( imgurl );
            QNetworkReply* reply = TomahawkUtils::nam()->get( req );
            reply->setProperty( "requestData", QVariant::fromValue< Tomahawk::InfoSystem::InfoRequestData >( requestData ) );

            connect( reply, SIGNAL( finished() ), SLOT( artistImagesReturned() ) );
            return;
        }

        default:
        {
            tLog() << "Couldn't figure out what to do with this type of request after cache miss";
            emit info( requestData, QVariant() );
            return;
        }
    }
}


void
LastFmPlugin::similarArtistsReturned()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>( sender() );

    QMap< int, QString > similarArtists = lastfm::Artist::getSimilar( reply );

    QStringList sortedArtists;
    QStringList sortedScores;
    QStringList al;
    QStringList sl;

    foreach ( const QString& artist, similarArtists.values() )
        al << artist;
    foreach ( int score, similarArtists.keys() )
        sl << QString::number( score );

    for ( int i = al.count() - 1; i >= 0; i-- )
    {
        sortedArtists << al.at( i );
        sortedScores << sl.at( i );
    }

    QVariantMap returnedData;
    returnedData["artists"] = sortedArtists;
    returnedData["score"] = sortedScores;

    Tomahawk::InfoSystem::InfoRequestData requestData = reply->property( "requestData" ).value< Tomahawk::InfoSystem::InfoRequestData >();

    emit info( requestData, returnedData );

    Tomahawk::InfoSystem::InfoStringHash origData = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash>();
    Tomahawk::InfoSystem::InfoStringHash criteria;
    criteria["artist"] = origData["artist"];
    emit updateCache( criteria, 2419200000, requestData.type, returnedData );
}


void
LastFmPlugin::chartReturned()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>( sender() );

    QVariantMap returnedData;
    const QRegExp tracks_rx( "chart\\.\\S+tracks\\S*", Qt::CaseInsensitive );
    const QRegExp artists_rx( "chart\\.\\S+artists\\S*", Qt::CaseInsensitive );
    const QString url = reply->url().toString();

    if ( url.contains( tracks_rx ) )
    {
        QList<lastfm::Track> tracks = parseTrackList( reply );
        QList<InfoStringHash> top_tracks;
        foreach( const lastfm::Track &t, tracks ) {
            InfoStringHash pair;
            pair[ "artist" ] = t.artist().toString();
            pair[ "track" ] = t.title();
            top_tracks << pair;
        }
        returnedData["tracks"] = QVariant::fromValue( top_tracks );
        returnedData["type"] = "tracks";

    }
    else if ( url.contains( artists_rx ) )
    {
        QList<lastfm::Artist> list = lastfm::Artist::list( reply );
        QStringList al;
        foreach ( const lastfm::Artist& a, list )
            al << a.toString();
        returnedData["artists"] = al;
        returnedData["type"] = "artists";
    }
    else
    {
        tDebug() << Q_FUNC_INFO << "got non tracks and non artists";
    }

    Tomahawk::InfoSystem::InfoRequestData requestData = reply->property( "requestData" ).value< Tomahawk::InfoSystem::InfoRequestData >();

    emit info( requestData, returnedData );
    // TODO update cache
}


void
LastFmPlugin::topTracksReturned()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>( sender() );

    QStringList topTracks = lastfm::Artist::getTopTracks( reply );
    QVariantMap returnedData;
    returnedData["tracks"] = topTracks;

    Tomahawk::InfoSystem::InfoRequestData requestData = reply->property( "requestData" ).value< Tomahawk::InfoSystem::InfoRequestData >();

    emit info( requestData, returnedData );

    Tomahawk::InfoSystem::InfoStringHash origData = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash>();
    Tomahawk::InfoSystem::InfoStringHash criteria;
    criteria["artist"] = origData["artist"];
    emit updateCache( criteria, 0, requestData.type, returnedData );
}


void
LastFmPlugin::coverArtReturned()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>( sender() );
    QUrl redir = reply->attribute( QNetworkRequest::RedirectionTargetAttribute ).toUrl();
    if ( redir.isEmpty() )
    {
        QByteArray ba = reply->readAll();
        if ( ba.isNull() || !ba.length() )
        {
            tLog() << Q_FUNC_INFO << "Uh oh, null byte array";
            emit info( reply->property( "requestData" ).value< Tomahawk::InfoSystem::InfoRequestData >(), QVariant() );
            return;
        }
        foreach ( const QUrl& url, m_badUrls )
        {
            if ( reply->url().toString().startsWith( url.toString() ) )
                ba = QByteArray();
        }

        QVariantMap returnedData;
        returnedData["imgbytes"] = ba;
        returnedData["url"] = reply->url().toString();

        Tomahawk::InfoSystem::InfoRequestData requestData = reply->property( "requestData" ).value< Tomahawk::InfoSystem::InfoRequestData >();

        emit info( requestData, returnedData );

        Tomahawk::InfoSystem::InfoStringHash origData = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash>();
        Tomahawk::InfoSystem::InfoStringHash criteria;
        criteria["artist"] = origData["artist"];
        criteria["album"] = origData["album"];
        emit updateCache( criteria, 2419200000, requestData.type, returnedData );
    }
    else
    {
        if ( !TomahawkUtils::nam() )
        {
            tLog() << Q_FUNC_INFO << "Uh oh, nam is null";
            emit info( reply->property( "requestData" ).value< Tomahawk::InfoSystem::InfoRequestData >(), QVariant() );
            return;
        }
        // Follow HTTP redirect
        QNetworkRequest req( redir );
        QNetworkReply* newReply = TomahawkUtils::nam()->get( req );
        newReply->setProperty( "requestData", reply->property( "requestData" ) );
        connect( newReply, SIGNAL( finished() ), SLOT( coverArtReturned() ) );
    }

    reply->deleteLater();
}


void
LastFmPlugin::artistImagesReturned()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>( sender() );
    QUrl redir = reply->attribute( QNetworkRequest::RedirectionTargetAttribute ).toUrl();
    if ( redir.isEmpty() )
    {
        QByteArray ba = reply->readAll();
        if ( ba.isNull() || !ba.length() )
        {
            tLog() << Q_FUNC_INFO << "Uh oh, null byte array";
            emit info( reply->property( "requestData" ).value< Tomahawk::InfoSystem::InfoRequestData >(), QVariant() );
            return;
        }
        foreach ( const QUrl& url, m_badUrls )
        {
            if ( reply->url().toString().startsWith( url.toString() ) )
                ba = QByteArray();
        }
        QVariantMap returnedData;
        returnedData["imgbytes"] = ba;
        returnedData["url"] = reply->url().toString();

        Tomahawk::InfoSystem::InfoRequestData requestData = reply->property( "requestData" ).value< Tomahawk::InfoSystem::InfoRequestData >();

        emit info( requestData, returnedData );

        Tomahawk::InfoSystem::InfoStringHash origData = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash>();
        Tomahawk::InfoSystem::InfoStringHash criteria;
        criteria["artist"] = origData["artist"];
        emit updateCache( criteria, 2419200000, requestData.type, returnedData );
    }
    else
    {
        if ( !TomahawkUtils::nam() )
        {
            tLog() << Q_FUNC_INFO << "Uh oh, nam is null";
            emit info( reply->property( "requestData" ).value< Tomahawk::InfoSystem::InfoRequestData >(), QVariant() );
            return;
        }
        // Follow HTTP redirect
        QNetworkRequest req( redir );
        QNetworkReply* newReply = TomahawkUtils::nam()->get( req );
        newReply->setProperty( "requestData", reply->property( "requestData" ) );
        connect( newReply, SIGNAL( finished() ), SLOT( artistImagesReturned() ) );
    }

    reply->deleteLater();
}


void
LastFmPlugin::settingsChanged()
{
    if ( !m_scrobbler && TomahawkSettings::instance()->scrobblingEnabled() )
    { // can simply create the scrobbler
        lastfm::ws::Username = TomahawkSettings::instance()->lastFmUsername();
        m_pw = TomahawkSettings::instance()->lastFmPassword();

        createScrobbler();
    }
    else if ( m_scrobbler && !TomahawkSettings::instance()->scrobblingEnabled() )
    {
        delete m_scrobbler;
        m_scrobbler = 0;
    }
    else if ( TomahawkSettings::instance()->lastFmUsername() != lastfm::ws::Username ||
               TomahawkSettings::instance()->lastFmPassword() != m_pw )
    {
        lastfm::ws::Username = TomahawkSettings::instance()->lastFmUsername();
        m_pw = TomahawkSettings::instance()->lastFmPassword();
        // credentials have changed, have to re-create scrobbler for them to take effect
        if ( m_scrobbler )
        {
            delete m_scrobbler;
            m_scrobbler = 0;
        }

        createScrobbler();
    }
}


void
LastFmPlugin::onAuthenticated()
{
    QNetworkReply* authJob = dynamic_cast<QNetworkReply*>( sender() );
    if ( !authJob )
    {
        tLog() << Q_FUNC_INFO << "Help! No longer got a last.fm auth job!";
        return;
    }

    if ( authJob->error() == QNetworkReply::NoError )
    {
        lastfm::XmlQuery lfm = lastfm::XmlQuery( authJob->readAll() );

        if ( lfm.children( "error" ).size() > 0 )
        {
            tLog() << "Error from authenticating with Last.fm service:" << lfm.text();
            TomahawkSettings::instance()->setLastFmSessionKey( QByteArray() );
        }
        else
        {
            lastfm::ws::SessionKey = lfm[ "session" ][ "key" ].text();

            TomahawkSettings::instance()->setLastFmSessionKey( lastfm::ws::SessionKey.toLatin1() );

//            qDebug() << "Got session key from last.fm";
            if ( TomahawkSettings::instance()->scrobblingEnabled() )
                m_scrobbler = new lastfm::Audioscrobbler( "thk" );
        }
    }
    else
    {
        tLog() << "Got error in Last.fm authentication job:" << authJob->errorString();
    }

    authJob->deleteLater();
}


void
LastFmPlugin::createScrobbler()
{
    if ( TomahawkSettings::instance()->lastFmSessionKey().isEmpty() ) // no session key, so get one
    {
        qDebug() << "LastFmPlugin::createScrobbler Session key is empty";
        QString authToken = TomahawkUtils::md5( ( lastfm::ws::Username.toLower() + TomahawkUtils::md5( m_pw.toUtf8() ) ).toUtf8() );

        QMap<QString, QString> query;
        query[ "method" ] = "auth.getMobileSession";
        query[ "username" ] = lastfm::ws::Username;
        query[ "authToken" ] = authToken;
        QNetworkReply* authJob = lastfm::ws::post( query );

        connect( authJob, SIGNAL( finished() ), SLOT( onAuthenticated() ) );
    }
    else
    {
        qDebug() << "LastFmPlugin::createScrobbler Already have session key";
        lastfm::ws::SessionKey = TomahawkSettings::instance()->lastFmSessionKey();

        m_scrobbler = new lastfm::Audioscrobbler( "thk" );
    }
}


QList<lastfm::Track>
LastFmPlugin::parseTrackList( QNetworkReply* reply )
{
    QList<lastfm::Track> tracks;
    try
    {
        lastfm::XmlQuery lfm = reply->readAll();
        foreach ( lastfm::XmlQuery xq, lfm.children( "track" ) )
        {
            tracks.append( lastfm::Track( xq ) );
        }
    }
    catch ( lastfm::ws::ParseError& e )
    {
        qWarning() << e.what();
    }

    return tracks;
}
