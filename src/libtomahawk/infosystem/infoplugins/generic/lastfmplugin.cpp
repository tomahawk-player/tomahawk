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
#include <QCryptographicHash>
#include <QNetworkConfiguration>

#include "album.h"
#include "typedefs.h"
#include "audio/audioengine.h"
#include "tomahawksettings.h"
#include "utils/tomahawkutils.h"

#include <lastfm/ws.h>
#include <lastfm/XmlQuery>

using namespace Tomahawk::InfoSystem;

static QString
md5( const QByteArray& src )
{
    QByteArray const digest = QCryptographicHash::hash( src, QCryptographicHash::Md5 );
    return QString::fromLatin1( digest.toHex() ).rightJustified( 32, '0' );
}


LastFmPlugin::LastFmPlugin()
    : InfoPlugin()
    , m_scrobbler( 0 )
{
    m_supportedGetTypes << InfoAlbumCoverArt << InfoArtistImages << InfoArtistSimilars << InfoArtistSongs;
    m_supportedPushTypes << InfoSubmitScrobble << InfoSubmitNowPlaying << InfoLove << InfoUnLove;

/*
      Your API Key is 7194b85b6d1f424fe1668173a78c0c4a
      Your secret is ba80f1df6d27ae63e9cb1d33ccf2052f
*/

    // Flush session key cache
    TomahawkSettings::instance()->setLastFmSessionKey( QByteArray() );

    lastfm::ws::ApiKey = "7194b85b6d1f424fe1668173a78c0c4a";
    lastfm::ws::SharedSecret = "ba80f1df6d27ae63e9cb1d33ccf2052f";
    lastfm::ws::Username = TomahawkSettings::instance()->lastFmUsername();

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
}


LastFmPlugin::~LastFmPlugin()
{
    qDebug() << Q_FUNC_INFO << " beginning";
    delete m_scrobbler;
    qDebug() << Q_FUNC_INFO << " exiting";
}


void
LastFmPlugin::namChangedSlot( QNetworkAccessManager *nam )
{
    qDebug() << Q_FUNC_INFO;

    if ( !nam )
        return;

    QNetworkAccessManager* currNam = lastfm::nam();
    TomahawkUtils::NetworkProxyFactory* oldProxyFactory = dynamic_cast< TomahawkUtils::NetworkProxyFactory* >( nam->proxyFactory() );

    if ( !oldProxyFactory )
    {
        qDebug() << "Could not get old proxyFactory!";
        return;
    }

    currNam->setConfiguration( nam->configuration() );
    currNam->setNetworkAccessible( nam->networkAccessible() );
    TomahawkUtils::NetworkProxyFactory* newProxyFactory = new TomahawkUtils::NetworkProxyFactory();
    newProxyFactory->setNoProxyHosts( oldProxyFactory->noProxyHosts() );
    QNetworkProxy newProxy( oldProxyFactory->proxy() );
    newProxyFactory->setProxy( newProxy );
    currNam->setProxyFactory( newProxyFactory );
    //FIXME: on Mac/Win as liblastfm's network access manager also sets its overriding application proxy
    //may have to do a QNetworkProxy::setApplicationProxy and clobber our own factory to override it
    settingsChanged(); // to get the scrobbler set up
}


void
LastFmPlugin::dataError( uint requestId, Tomahawk::InfoSystem::InfoRequestData requestData )
{
    emit info( requestId, requestData, QVariant() );
    return;
}


void
LastFmPlugin::getInfo( uint requestId, Tomahawk::InfoSystem::InfoRequestData requestData )
{
    qDebug() << Q_FUNC_INFO;

    switch ( requestData.type )
    {
        case InfoArtistImages:
            fetchArtistImages( requestId, requestData );
            break;

        case InfoAlbumCoverArt:
            fetchCoverArt( requestId, requestData );
            break;

        case InfoArtistSimilars:
            fetchSimilarArtists( requestId, requestData );
            break;

        case InfoArtistSongs:
            fetchTopTracks( requestId, requestData );
            break;

        default:
            dataError( requestId, requestData );
    }
}


void
LastFmPlugin::pushInfo( const QString caller, const Tomahawk::InfoSystem::InfoType type, const QVariant input )
{
    qDebug() << Q_FUNC_INFO;
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
    qDebug() << Q_FUNC_INFO;
    if ( !input.canConvert< Tomahawk::InfoSystem::InfoCriteriaHash >() || !m_scrobbler )
    {
        qDebug() << "LastFmPlugin::nowPlaying no m_scrobbler, or cannot convert input!";
        if ( ! m_scrobbler )
            qDebug() << "no scrobbler!";
        return;
    }

    InfoCriteriaHash hash = input.value< Tomahawk::InfoSystem::InfoCriteriaHash >();
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
    qDebug() << Q_FUNC_INFO;

    if ( !m_scrobbler || m_track.isNull() )
        return;

    qDebug() << Q_FUNC_INFO << m_track.toString();
    m_scrobbler->cache( m_track );
    m_scrobbler->submit();
}


void
LastFmPlugin::sendLoveSong( const InfoType type, QVariant input )
{
    qDebug() << Q_FUNC_INFO;

    if ( !input.canConvert< Tomahawk::InfoSystem::InfoCriteriaHash >() )
    {
        qDebug() << "LastFmPlugin::nowPlaying cannot convert input!";
        return;
    }

    InfoCriteriaHash hash = input.value< Tomahawk::InfoSystem::InfoCriteriaHash >();
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
LastFmPlugin::fetchSimilarArtists( uint requestId, Tomahawk::InfoSystem::InfoRequestData requestData )
{
    qDebug() << Q_FUNC_INFO;
    if ( !requestData.input.canConvert< Tomahawk::InfoSystem::InfoCriteriaHash >() )
    {
        dataError( requestId, requestData );
        return;
    }
    InfoCriteriaHash hash = requestData.input.value< Tomahawk::InfoSystem::InfoCriteriaHash >();
    if ( !hash.contains( "artist" ) )
    {
        dataError( requestId, requestData );
        return;
    }

    Tomahawk::InfoSystem::InfoCriteriaHash criteria;
    criteria["artist"] = hash["artist"];

    emit getCachedInfo( requestId, criteria, 2419200000, requestData );
}


void
LastFmPlugin::fetchTopTracks( uint requestId, Tomahawk::InfoSystem::InfoRequestData requestData )
{
    qDebug() << Q_FUNC_INFO;
    if ( !requestData.input.canConvert< Tomahawk::InfoSystem::InfoCriteriaHash >() )
    {
        dataError( requestId, requestData );
        return;
    }
    InfoCriteriaHash hash = requestData.input.value< Tomahawk::InfoSystem::InfoCriteriaHash >();
    if ( !hash.contains( "artist" ) )
    {
        dataError( requestId, requestData );
        return;
    }

    Tomahawk::InfoSystem::InfoCriteriaHash criteria;
    criteria["artist"] = hash["artist"];

    emit getCachedInfo( requestId, criteria, 2419200000, requestData );
}


void
LastFmPlugin::fetchCoverArt( uint requestId, Tomahawk::InfoSystem::InfoRequestData requestData )
{
    qDebug() << Q_FUNC_INFO;
    if ( !requestData.input.canConvert< Tomahawk::InfoSystem::InfoCriteriaHash >() )
    {
        dataError( requestId, requestData );
        return;
    }
    InfoCriteriaHash hash = requestData.input.value< Tomahawk::InfoSystem::InfoCriteriaHash >();
    if ( !hash.contains( "artist" ) || !hash.contains( "album" ) )
    {
        dataError( requestId, requestData );
        return;
    }

    Tomahawk::InfoSystem::InfoCriteriaHash criteria;
    criteria["artist"] = hash["artist"];
    criteria["album"] = hash["album"];

    emit getCachedInfo( requestId, criteria, 2419200000, requestData );
}


void
LastFmPlugin::fetchArtistImages( uint requestId, Tomahawk::InfoSystem::InfoRequestData requestData )
{
    qDebug() << Q_FUNC_INFO;
    if ( !requestData.input.canConvert< Tomahawk::InfoSystem::InfoCriteriaHash >() )
    {
        dataError( requestId, requestData );
        return;
    }
    InfoCriteriaHash hash = requestData.input.value< Tomahawk::InfoSystem::InfoCriteriaHash >();
    if ( !hash.contains( "artist" ) )
    {
        dataError( requestId, requestData );
        return;
    }

    Tomahawk::InfoSystem::InfoCriteriaHash criteria;
    criteria["artist"] = hash["artist"];

    emit getCachedInfo( requestId, criteria, 2419200000, requestData );
}


void
LastFmPlugin::notInCacheSlot( uint requestId, QHash<QString, QString> criteria, Tomahawk::InfoSystem::InfoRequestData requestData )
{
    qDebug() << Q_FUNC_INFO << " for requestId " << requestId;

    if ( !lastfm::nam() )
    {
        qDebug() << "Have a null QNAM, uh oh";
        emit info( requestId, requestData, QVariant() );
        return;
    }

    switch ( requestData.type )
    {
        case InfoArtistSimilars:
        {
            lastfm::Artist a( criteria["artist"] );
            QNetworkReply* reply = a.getSimilar();
            reply->setProperty( "requestId", requestId );
            reply->setProperty( "requestData", QVariant::fromValue< Tomahawk::InfoSystem::InfoRequestData >( requestData ) );

            connect( reply, SIGNAL( finished() ), SLOT( similarArtistsReturned() ) );
            return;
        }

        case InfoArtistSongs:
        {
            lastfm::Artist a( criteria["artist"] );
            QNetworkReply* reply = a.getTopTracks();
            reply->setProperty( "requestId", requestId );
            reply->setProperty( "requestData", QVariant::fromValue< Tomahawk::InfoSystem::InfoRequestData >( requestData ) );

            connect( reply, SIGNAL( finished() ), SLOT( topTracksReturned() ) );
            return;
        }

        case InfoAlbumCoverArt:
        {
            QString artistName = criteria["artist"];
            QString albumName = criteria["album"];

            QString imgurl = "http://ws.audioscrobbler.com/2.0/?method=album.imageredirect&artist=%1&album=%2&autocorrect=1&size=large&api_key=7a90f6672a04b809ee309af169f34b8b";
            QNetworkRequest req( imgurl.arg( artistName ).arg( albumName ) );
            QNetworkReply* reply = lastfm::nam()->get( req );
            reply->setProperty( "requestId", requestId );
            reply->setProperty( "requestData", QVariant::fromValue< Tomahawk::InfoSystem::InfoRequestData >( requestData ) );

            connect( reply, SIGNAL( finished() ), SLOT( coverArtReturned() ) );
            return;
        }

        case InfoArtistImages:
        {
            QString artistName = criteria["artist"];

            QString imgurl = "http://ws.audioscrobbler.com/2.0/?method=artist.imageredirect&artist=%1&autocorrect=1&size=large&api_key=7a90f6672a04b809ee309af169f34b8b";
            QNetworkRequest req( imgurl.arg( artistName ) );
            QNetworkReply* reply = lastfm::nam()->get( req );
            reply->setProperty( "requestId", requestId );
            reply->setProperty( "requestData", QVariant::fromValue< Tomahawk::InfoSystem::InfoRequestData >( requestData ) );

            connect( reply, SIGNAL( finished() ), SLOT( artistImagesReturned() ) );
            return;
        }

        default:
        {
            qDebug() << "Couldn't figure out what to do with this type of request after cache miss";
            emit info( requestId, requestData, QVariant() );
            return;
        }
    }
}


void
LastFmPlugin::similarArtistsReturned()
{
    qDebug() << Q_FUNC_INFO;
    QNetworkReply* reply = qobject_cast<QNetworkReply*>( sender() );

    QMap< int, QString > similarArtists = lastfm::Artist::getSimilar( reply );
    QStringList al;
    QStringList sl;

    foreach ( const QString& a, similarArtists.values() )
    {
        qDebug() << "Got sim-artist:" << a;
        al << a;
    }

    QVariantMap returnedData;
    returnedData["artists"] = al;
    returnedData["score"] = sl;

    Tomahawk::InfoSystem::InfoRequestData requestData = reply->property( "requestData" ).value< Tomahawk::InfoSystem::InfoRequestData >();

    emit info(
        reply->property( "requestId" ).toUInt(),
        requestData,
        returnedData
    );

    Tomahawk::InfoSystem::InfoCriteriaHash origData = requestData.input.value< Tomahawk::InfoSystem::InfoCriteriaHash>();
    Tomahawk::InfoSystem::InfoCriteriaHash criteria;
    criteria["artist"] = origData["artist"];
    emit updateCache( criteria, 2419200000, requestData.type, returnedData );
}


void
LastFmPlugin::topTracksReturned()
{
    qDebug() << Q_FUNC_INFO;
    QNetworkReply* reply = qobject_cast<QNetworkReply*>( sender() );

    QStringList topTracks = lastfm::Artist::getTopTracks( reply );
    foreach ( const QString& t, topTracks )
    {
        qDebug() << "Got top-track:" << t;
    }

    QVariantMap returnedData;
    returnedData["tracks"] = topTracks;

    Tomahawk::InfoSystem::InfoRequestData requestData = reply->property( "requestData" ).value< Tomahawk::InfoSystem::InfoRequestData >();
    
    emit info(
        reply->property( "requestId" ).toUInt(),
        requestData,
        returnedData
    );

    Tomahawk::InfoSystem::InfoCriteriaHash origData = requestData.input.value< Tomahawk::InfoSystem::InfoCriteriaHash>();
    Tomahawk::InfoSystem::InfoCriteriaHash criteria;
    criteria["artist"] = origData["artist"];
    emit updateCache( criteria, 2419200000, requestData.type, returnedData );
}


void
LastFmPlugin::coverArtReturned()
{
    qDebug() << Q_FUNC_INFO;
    QNetworkReply* reply = qobject_cast<QNetworkReply*>( sender() );
    QUrl redir = reply->attribute( QNetworkRequest::RedirectionTargetAttribute ).toUrl();
    if ( redir.isEmpty() )
    {
        QByteArray ba = reply->readAll();
        if ( ba.isNull() || !ba.length() )
        {
            qDebug() << "Uh oh, null byte array";
            emit info( reply->property( "requestId" ).toUInt(), reply->property( "requestData" ).value< Tomahawk::InfoSystem::InfoRequestData >(), QVariant() );
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
        
        emit info(
            reply->property( "requestId" ).toUInt(),
            requestData,
            returnedData
        );

        Tomahawk::InfoSystem::InfoCriteriaHash origData = requestData.input.value< Tomahawk::InfoSystem::InfoCriteriaHash>();
        Tomahawk::InfoSystem::InfoCriteriaHash criteria;
        criteria["artist"] = origData["artist"];
        criteria["album"] = origData["album"];
        emit updateCache( criteria, 2419200000, requestData.type, returnedData );
    }
    else
    {
        if ( !lastfm::nam() )
        {
            qDebug() << "Uh oh, nam is null";
            emit info( reply->property( "requestId" ).toUInt(), reply->property( "requestData" ).value< Tomahawk::InfoSystem::InfoRequestData >(), QVariant() );
            return;
        }
        // Follow HTTP redirect
        QNetworkRequest req( redir );
        QNetworkReply* newReply = lastfm::nam()->get( req );
        newReply->setProperty( "requestId", reply->property( "requestId" ) );
        newReply->setProperty( "requestData", reply->property( "requestData" ) );
        connect( newReply, SIGNAL( finished() ), SLOT( coverArtReturned() ) );
    }

    reply->deleteLater();
}


void
LastFmPlugin::artistImagesReturned()
{
    qDebug() << Q_FUNC_INFO;
    QNetworkReply* reply = qobject_cast<QNetworkReply*>( sender() );
    QUrl redir = reply->attribute( QNetworkRequest::RedirectionTargetAttribute ).toUrl();
    if ( redir.isEmpty() )
    {
        QByteArray ba = reply->readAll();
        if ( ba.isNull() || !ba.length() )
        {
            qDebug() << "Uh oh, null byte array";
            emit info( reply->property( "requestId" ).toUInt(), reply->property( "requestData" ).value< Tomahawk::InfoSystem::InfoRequestData >(), QVariant() );
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
        
        emit info( reply->property( "requestId" ).toUInt(), requestData, returnedData );

        Tomahawk::InfoSystem::InfoCriteriaHash origData = requestData.input.value< Tomahawk::InfoSystem::InfoCriteriaHash>();
        Tomahawk::InfoSystem::InfoCriteriaHash criteria;
        criteria["artist"] = origData["artist"];
        emit updateCache( criteria, 2419200000, requestData.type, returnedData );
    }
    else
    {
        if ( !lastfm::nam() )
        {
            qDebug() << "Uh oh, nam is null";
            emit info( reply->property( "requestId" ).toUInt(), reply->property( "requestData" ).value< Tomahawk::InfoSystem::InfoRequestData >(), QVariant() );
            return;
        }
        // Follow HTTP redirect
        QNetworkRequest req( redir );
        QNetworkReply* newReply = lastfm::nam()->get( req );
        newReply->setProperty( "requestId", reply->property( "requestId" ) );
        newReply->setProperty( "requestData", reply->property( "requestData" ) );
        connect( newReply, SIGNAL( finished() ), SLOT( artistImagesReturned() ) );
    }

    reply->deleteLater();
}


void
LastFmPlugin::settingsChanged()
{
    if( !m_scrobbler && TomahawkSettings::instance()->scrobblingEnabled() )
    { // can simply create the scrobbler
        lastfm::ws::Username = TomahawkSettings::instance()->lastFmUsername();
        m_pw = TomahawkSettings::instance()->lastFmPassword();

        createScrobbler();
    }
    else if( m_scrobbler && !TomahawkSettings::instance()->scrobblingEnabled() )
    {
        delete m_scrobbler;
        m_scrobbler = 0;
    }
    else if( TomahawkSettings::instance()->lastFmUsername() != lastfm::ws::Username ||
               TomahawkSettings::instance()->lastFmPassword() != m_pw )
    {
        lastfm::ws::Username = TomahawkSettings::instance()->lastFmUsername();
        m_pw = TomahawkSettings::instance()->lastFmPassword();
        // credentials have changed, have to re-create scrobbler for them to take effect
        if( m_scrobbler )
            delete m_scrobbler;

        createScrobbler();
    }
}


void
LastFmPlugin::onAuthenticated()
{
    qDebug() << Q_FUNC_INFO;
    QNetworkReply* authJob = dynamic_cast<QNetworkReply*>( sender() );
    if( !authJob )
    {
        qDebug() << Q_FUNC_INFO << "Help! No longer got a last.fm auth job!";
        return;
    }

    if( authJob->error() == QNetworkReply::NoError )
    {
        lastfm::XmlQuery lfm = lastfm::XmlQuery( authJob->readAll() );

        if( lfm.children( "error" ).size() > 0 )
        {
            qDebug() << "Error from authenticating with Last.fm service:" << lfm.text();
            TomahawkSettings::instance()->setLastFmSessionKey( QByteArray() );
        }
        else
        {
            lastfm::ws::SessionKey = lfm[ "session" ][ "key" ].text();

            TomahawkSettings::instance()->setLastFmSessionKey( lastfm::ws::SessionKey.toLatin1() );

            qDebug() << "Got session key from last.fm";
            if( TomahawkSettings::instance()->scrobblingEnabled() )
                m_scrobbler = new lastfm::Audioscrobbler( "thk" );
        }
    }
    else
    {
        qDebug() << "Got error in Last.fm authentication job:" << authJob->errorString();
    }

    authJob->deleteLater();
}


void
LastFmPlugin::createScrobbler()
{
    qDebug() << Q_FUNC_INFO;
    if( TomahawkSettings::instance()->lastFmSessionKey().isEmpty() ) // no session key, so get one
    {
        qDebug() << "LastFmPlugin::createScrobbler Session key is empty";
        QString authToken = md5( ( lastfm::ws::Username.toLower() + md5( m_pw.toUtf8() ) ).toUtf8() );

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

