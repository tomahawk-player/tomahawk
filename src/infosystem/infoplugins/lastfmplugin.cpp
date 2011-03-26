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
#include "tomahawk/tomahawkapp.h"

#include <lastfm/ws.h>
#include <lastfm/XmlQuery>

using namespace Tomahawk::InfoSystem;

static QString
md5( const QByteArray& src )
{
    QByteArray const digest = QCryptographicHash::hash( src, QCryptographicHash::Md5 );
    return QString::fromLatin1( digest.toHex() ).rightJustified( 32, '0' );
}


LastFmPlugin::LastFmPlugin( QObject* parent )
    : InfoPlugin(parent)
    , m_scrobbler( 0 )
    , m_authJob( 0 )
{
    QSet< InfoType > supportedTypes;
    supportedTypes << Tomahawk::InfoSystem::InfoMiscSubmitScrobble << Tomahawk::InfoSystem::InfoMiscSubmitNowPlaying;
    qobject_cast< InfoSystem* >(parent)->registerInfoTypes(this, supportedTypes);
    
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
    
    if( TomahawkSettings::instance()->scrobblingEnabled() && !lastfm::ws::Username.isEmpty() )
    {
        createScrobbler();
    }
        
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

    connect( TomahawkSettings::instance(), SIGNAL( changed() ),
                                             SLOT( settingsChanged() ), Qt::QueuedConnection );
}


LastFmPlugin::~LastFmPlugin()
{
    delete m_scrobbler;
}

void
LastFmPlugin::getInfo( const QString &caller, const InfoType type, const QVariant& data, Tomahawk::InfoSystem::InfoCustomDataHash customData )
{
    qDebug() << Q_FUNC_INFO;
    if ( type == InfoMiscSubmitNowPlaying )
        nowPlaying( caller, type, data, customData );
    else if ( type == InfoMiscSubmitScrobble )
        scrobble( caller, type, data, customData );
    else
        dataError( caller, type, data, customData );
}

void
LastFmPlugin::nowPlaying( const QString &caller, const InfoType type, const QVariant& data, Tomahawk::InfoSystem::InfoCustomDataHash &customData )
{
    if ( !data.canConvert< Tomahawk::InfoSystem::InfoCustomDataHash >() || !m_scrobbler )
    {
        dataError( caller, type, data, customData );
        return;
    }
    InfoCustomDataHash hash = data.value< Tomahawk::InfoSystem::InfoCustomDataHash >();
    if ( !hash.contains( "title" ) || !hash.contains( "artist" ) || !hash.contains( "album" ) || !hash.contains( "duration" ) )
    {
        dataError( caller, type, data, customData );
        return;
    }
    
    m_track = lastfm::MutableTrack();
    m_track.stamp();

    m_track.setTitle( hash["title"].toString() );
    m_track.setArtist( hash["artist"].toString() );
    m_track.setAlbum( hash["album"].toString() );
    m_track.setDuration( hash["duration"].toUInt() );
    m_track.setSource( lastfm::Track::Player );

    m_scrobbler->nowPlaying( m_track );
    emit info( caller, type, data, QVariant(), customData );
    emit finished( caller, type );
}

void
LastFmPlugin::dataError( const QString &caller, const InfoType type, const QVariant& data, Tomahawk::InfoSystem::InfoCustomDataHash &customData )
{
    emit info( caller, type, data, QVariant(), customData );
    emit finished( caller, type );
    return;
}

void
LastFmPlugin::scrobble( const QString &caller, const InfoType type, const QVariant& data, Tomahawk::InfoSystem::InfoCustomDataHash &customData )
{
    Q_ASSERT( QThread::currentThread() == thread() );

    if ( !m_scrobbler || m_track.isNull() )
    {
        dataError( caller, type, data, customData );
        return;
    }

    qDebug() << Q_FUNC_INFO << m_track.toString();
    m_scrobbler->cache( m_track );
    m_scrobbler->submit();
    
    emit info( caller, type, data, QVariant(), customData );
    emit finished( caller, type );
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
    if( TomahawkSettings::instance()->lastFmSessionKey().isEmpty() ) // no session key, so get one
    {
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
        lastfm::ws::SessionKey = TomahawkSettings::instance()->lastFmSessionKey();
        
        m_scrobbler = new lastfm::Audioscrobbler( "thk" );
        m_scrobbler->moveToThread( thread() );
    }
}
