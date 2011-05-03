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


LastFmPlugin::LastFmPlugin( InfoSystemWorker* parent )
    : InfoPlugin(parent)
    , m_scrobbler( 0 )
    , m_authJob( 0 )
    , m_infoSystemWorker( parent )
{
    QSet< InfoType > supportedGetTypes, supportedPushTypes;
    supportedGetTypes << InfoAlbumCoverArt << InfoArtistImages;
    supportedPushTypes << InfoSubmitScrobble << InfoSubmitNowPlaying; 
    parent->registerInfoTypes( this, supportedGetTypes, supportedPushTypes );

    connect( parent, SIGNAL( namChanged() ), SLOT( namChangedSlot() ) );

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
LastFmPlugin::namChangedSlot()
{
    qDebug() << Q_FUNC_INFO;
    lastfm::setNetworkAccessManager( m_infoSystemWorker->nam() );
    settingsChanged(); // to get the scrobbler set up
}


void
LastFmPlugin::dataError( const QString &caller, const Tomahawk::InfoSystem::InfoType type, const QVariant &input, const Tomahawk::InfoSystem::InfoCustomData &customData )
{
    emit info( caller, type, input, QVariant(), customData );
    return;
}


void
LastFmPlugin::getInfo( const QString caller, const Tomahawk::InfoSystem::InfoType type, const QVariant input, const Tomahawk::InfoSystem::InfoCustomData customData )
{
    qDebug() << Q_FUNC_INFO;

    switch ( type )
    {
        case InfoArtistImages:
            fetchArtistImages( caller, type, input, customData );
            break;

        case InfoAlbumCoverArt:
            fetchCoverArt( caller, type, input, customData );
            break;

        default:
            dataError( caller, type, input, customData );
    }
}


void
LastFmPlugin::pushInfo( const QString caller, const Tomahawk::InfoSystem::InfoType type, const QVariant input )
{
    qDebug() << Q_FUNC_INFO;

    switch ( type )
    {
        case InfoSubmitNowPlaying:
            nowPlaying( caller, type, input );
            break;

        case InfoSubmitScrobble:
            scrobble( caller, type, input );
            break;

        default:
            return;
    }
}

void
LastFmPlugin::nowPlaying( const QString &caller, const InfoType type, const QVariant &input )
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

    qDebug() << "LastFmPlugin::nowPlaying valid criteria hash";
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
LastFmPlugin::scrobble( const QString &caller, const InfoType type, const QVariant &input )
{
    qDebug() << Q_FUNC_INFO;
    //Q_ASSERT( QThread::currentThread() == thread() );

    if ( !m_scrobbler || m_track.isNull() )
        return;

    qDebug() << Q_FUNC_INFO << m_track.toString();
    m_scrobbler->cache( m_track );
    m_scrobbler->submit();
}


void
LastFmPlugin::fetchCoverArt( const QString &caller, const InfoType type, const QVariant &input, const Tomahawk::InfoSystem::InfoCustomData &customData )
{
    qDebug() << Q_FUNC_INFO;
    if ( !input.canConvert< Tomahawk::InfoSystem::InfoCriteriaHash >() )
    {
        dataError( caller, type, input, customData );
        return;
    }
    InfoCriteriaHash hash = input.value< Tomahawk::InfoSystem::InfoCriteriaHash >();
    if ( !hash.contains( "artist" ) || !hash.contains( "album" ) )
    {
        dataError( caller, type, input, customData );
        return;
    }

    Tomahawk::InfoSystem::InfoCriteriaHash criteria;
    criteria["artist"] = hash["artist"];
    criteria["album"] = hash["album"];

    emit getCachedInfo( criteria, 2419200000, caller, type, input, customData );
}


void
LastFmPlugin::fetchArtistImages( const QString &caller, const InfoType type, const QVariant &input, const Tomahawk::InfoSystem::InfoCustomData &customData )
{
    qDebug() << Q_FUNC_INFO;
    if ( !input.canConvert< Tomahawk::InfoSystem::InfoCriteriaHash >() )
    {
        dataError( caller, type, input, customData );
        return;
    }
    InfoCriteriaHash hash = input.value< Tomahawk::InfoSystem::InfoCriteriaHash >();
    if ( !hash.contains( "artist" ) )
    {
        dataError( caller, type, input, customData );
        return;
    }

    Tomahawk::InfoSystem::InfoCriteriaHash criteria;
    criteria["artist"] = hash["artist"];

    emit getCachedInfo( criteria, 2419200000, caller, type, input, customData );
}


void
LastFmPlugin::notInCacheSlot( const QHash<QString, QString> criteria, const QString caller, const Tomahawk::InfoSystem::InfoType type, const QVariant input, const Tomahawk::InfoSystem::InfoCustomData customData )
{
    qDebug() << Q_FUNC_INFO;

    switch ( type )
    {
        case InfoAlbumCoverArt:
        {
            QString artistName = criteria["artist"];
            QString albumName = criteria["album"];

            QString imgurl = "http://ws.audioscrobbler.com/2.0/?method=album.imageredirect&artist=%1&album=%2&autocorrect=1&size=medium&api_key=7a90f6672a04b809ee309af169f34b8b";
            QNetworkRequest req( imgurl.arg( artistName ).arg( albumName ) );
            QNetworkReply* reply = m_infoSystemWorker->nam()->get( req );
            reply->setProperty( "customData", QVariant::fromValue<Tomahawk::InfoSystem::InfoCustomData>( customData ) );
            reply->setProperty( "origData", input );
            reply->setProperty( "caller", caller );
            reply->setProperty( "type", (uint)(type) );

            connect( reply, SIGNAL( finished() ), SLOT( coverArtReturned() ) );
            return;
        }

        case InfoArtistImages:
        {
            QString artistName = criteria["artist"];

            QString imgurl = "http://ws.audioscrobbler.com/2.0/?method=artist.imageredirect&artist=%1&autocorrect=1&size=medium&api_key=7a90f6672a04b809ee309af169f34b8b";
            QNetworkRequest req( imgurl.arg( artistName ) );
            QNetworkReply* reply = m_infoSystemWorker->nam()->get( req );
            reply->setProperty( "customData", QVariant::fromValue<Tomahawk::InfoSystem::InfoCustomData>( customData ) );
            reply->setProperty( "origData", input );
            reply->setProperty( "caller", caller );
            reply->setProperty( "type", (uint)(type) );

            connect( reply, SIGNAL( finished() ), SLOT( artistImagesReturned() ) );
            return;
        }

        default:
            qDebug() << "Couldn't figure out what to do with this type of request after cache miss";
    }
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
        foreach ( const QUrl& url, m_badUrls )
        {
            if ( reply->url().toString().startsWith( url.toString() ) )
                ba = QByteArray();
        }

        InfoCustomData returnedData;
        returnedData["imgbytes"] = ba;
        returnedData["url"] = reply->url().toString();

        InfoCustomData customData = reply->property( "customData" ).value< Tomahawk::InfoSystem::InfoCustomData >();
        InfoType type = (Tomahawk::InfoSystem::InfoType)(reply->property( "type" ).toUInt());
        emit info(
            reply->property( "caller" ).toString(),
            type,
            reply->property( "origData" ),
            returnedData,
            customData
        );

        InfoCriteriaHash origData = reply->property( "origData" ).value< Tomahawk::InfoSystem::InfoCriteriaHash >();
        Tomahawk::InfoSystem::InfoCriteriaHash criteria;
        criteria["artist"] = origData["artist"];
        criteria["album"] = origData["album"];
        emit updateCache( criteria, 2419200000, type, returnedData );
    }
    else
    {
        // Follow HTTP redirect
        QNetworkRequest req( redir );
        QNetworkReply* newReply = m_infoSystemWorker->nam()->get( req );
        newReply->setProperty( "origData", reply->property( "origData" ) );
        newReply->setProperty( "customData", reply->property( "customData" ) );
        newReply->setProperty( "caller", reply->property( "caller" ) );
        newReply->setProperty( "type", reply->property( "type" ) );
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
        foreach ( const QUrl& url, m_badUrls )
        {
            if ( reply->url().toString().startsWith( url.toString() ) )
                ba = QByteArray();
        }

        InfoCustomData returnedData;
        returnedData["imgbytes"] = ba;
        returnedData["url"] = reply->url().toString();

        InfoCustomData customData = reply->property( "customData" ).value< Tomahawk::InfoSystem::InfoCustomData >();
        InfoType type = (Tomahawk::InfoSystem::InfoType)(reply->property( "type" ).toUInt());
        emit info(
            reply->property( "caller" ).toString(),
                  type,
                  reply->property( "origData" ),
                  returnedData,
                  customData
                  );

                  InfoCriteriaHash origData = reply->property( "origData" ).value< Tomahawk::InfoSystem::InfoCriteriaHash >();
                  Tomahawk::InfoSystem::InfoCriteriaHash criteria;
                  criteria["artist"] = origData["artist"];
                  emit updateCache( criteria, 2419200000, type, returnedData );
    }
    else
    {
        // Follow HTTP redirect
        QNetworkRequest req( redir );
        QNetworkReply* newReply = m_infoSystemWorker->nam()->get( req );
        newReply->setProperty( "origData", reply->property( "origData" ) );
        newReply->setProperty( "customData", reply->property( "customData" ) );
        newReply->setProperty( "caller", reply->property( "caller" ) );
        newReply->setProperty( "type", reply->property( "type" ) );
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
    if( !m_authJob )
    {
        qDebug() << Q_FUNC_INFO << "Help! No longer got a last.fm auth job!";
        return;
    }

    if( m_authJob->error() == QNetworkReply::NoError )
    {
        lastfm::XmlQuery lfm = lastfm::XmlQuery( m_authJob->readAll() );

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
        qDebug() << "Got error in Last.fm authentication job:" << m_authJob->errorString();
    }

    m_authJob->deleteLater();
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
        m_authJob = lastfm::ws::post( query );

        connect( m_authJob, SIGNAL( finished() ), SLOT( onAuthenticated() ) );
    }
    else
    {
        qDebug() << "LastFmPlugin::createScrobbler Already have session key";
        lastfm::ws::SessionKey = TomahawkSettings::instance()->lastFmSessionKey();

        m_scrobbler = new lastfm::Audioscrobbler( "thk" );
    }
}
