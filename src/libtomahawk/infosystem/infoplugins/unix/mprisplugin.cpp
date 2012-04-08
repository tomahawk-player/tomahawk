/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include <QApplication>
#include <QImage>
#include <QtDBus/QtDBus>

#include "audio/audioengine.h"
#include "infosystem/infosystemworker.h"
#include "album.h"
#include "artist.h"
#include "result.h"
#include "tomahawksettings.h"
#include "globalactionmanager.h"
#include "utils/logger.h"
#include "utils/tomahawkutils.h"

#include "mprisplugin.h"
#include "mprispluginrootadaptor.h"
#include "mprispluginplayeradaptor.h"

using namespace Tomahawk::InfoSystem;

static QString s_mpInfoIdentifier = QString( "MPRISPLUGIN" );

MprisPlugin::MprisPlugin()
    : InfoPlugin()
    , m_coverTempFile( 0 )
{
    // init
    m_playbackStatus = "Stopped";

    // Types of pushInfo we care about
    m_supportedPushTypes << InfoNowPlaying << InfoNowPaused << InfoNowResumed << InfoNowStopped;

    // DBus connection
    new MprisPluginRootAdaptor( this );
    new MprisPluginPlayerAdaptor( this );
    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.registerObject( "/org/mpris/MediaPlayer2", this );
    dbus.registerService( "org.mpris.MediaPlayer2.tomahawk" );

    // Listen to volume changes
    connect( AudioEngine::instance(), SIGNAL( volumeChanged( int ) ),
                                        SLOT( onVolumeChanged( int ) ) );

    // When the playlist changes, signals for several properties are sent
    connect( AudioEngine::instance(), SIGNAL( playlistChanged( Tomahawk::playlistinterface_ptr ) ),
                                        SLOT( onPlaylistChanged( Tomahawk::playlistinterface_ptr ) ) );

    // When a track is added or removed, CanGoNext updated signal is sent
    Tomahawk::playlistinterface_ptr playlist = AudioEngine::instance()->playlist();
    if ( !playlist.isNull() )
    {
        connect( playlist.data(), SIGNAL( trackCountChanged( unsigned int ) ),
                                    SLOT( onTrackCountChanged( unsigned int ) ) );
    }

    // Connect to AudioEngine's seeked signal
    connect( AudioEngine::instance(), SIGNAL( seeked( qint64 ) ),
                                        SLOT( onSeeked( qint64 ) ) );

    // Connect to the InfoSystem (we need to get album covers via getInfo)

    connect( Tomahawk::InfoSystem::InfoSystem::instance(),
             SIGNAL( info( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ),
             SLOT( infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ) );

    connect( Tomahawk::InfoSystem::InfoSystem::instance(),
             SIGNAL( finished( QString ) ),
             SLOT( infoSystemFinished( QString ) ) );
}


MprisPlugin::~MprisPlugin()
{
    delete m_coverTempFile;
}


// org.mpris.MediaPlayer2

bool
MprisPlugin::canQuit() const
{
    return true;
}


bool
MprisPlugin::canRaise() const
{
    return false;
}


bool
MprisPlugin::hasTrackList() const
{
    return false;
}


QString
MprisPlugin::identity() const
{
    return QString( "Tomahawk" );
}


QString
MprisPlugin::desktopEntry() const
{
    return QString( "tomahawk" );
}


QStringList
MprisPlugin::supportedUriSchemes() const
{
    QStringList uriSchemes;
    uriSchemes << "tomahawk" << "spotify";
    return uriSchemes;
}


QStringList
MprisPlugin::supportedMimeTypes() const
{
    return QStringList();
}


void
MprisPlugin::Raise()
{
}


void
MprisPlugin::Quit()
{
    QApplication::quit();
}


// org.mpris.MediaPlayer2.Player

bool
MprisPlugin::canControl() const
{
    return true;
}


bool
MprisPlugin::canGoNext() const
{
    return AudioEngine::instance()->canGoNext();
}


bool
MprisPlugin::canGoPrevious() const
{
    return AudioEngine::instance()->canGoPrevious();
}


bool
MprisPlugin::canPause() const
{
    return AudioEngine::instance()->currentTrack();
}


bool
MprisPlugin::canPlay() const
{
    // If there is a currently playing track, or if there is a playlist with at least 1 track, you can hit play
    Tomahawk::playlistinterface_ptr p = AudioEngine::instance()->playlist();
    return AudioEngine::instance()->currentTrack() || ( !p.isNull() && p->trackCount() );
}


bool
MprisPlugin::canSeek() const
{
    Tomahawk::playlistinterface_ptr p = AudioEngine::instance()->playlist();
    if ( p.isNull() )
        return false;
    return p->seekRestrictions() != PlaylistInterface::NoSeek;

}


QString
MprisPlugin::loopStatus() const
{
    Tomahawk::playlistinterface_ptr p = AudioEngine::instance()->playlist();
    if ( p.isNull() )
        return "None";
    PlaylistInterface::RepeatMode mode = p->repeatMode();
    switch( mode )
    {
        case PlaylistInterface::RepeatOne:
            return "Track";
            break;
        case PlaylistInterface::RepeatAll:
            return "Playlist";
            break;
        case PlaylistInterface::NoRepeat:
            return "None";
            break;
        default:
            return "None";
            break;
    }

    return QString( "None" );
}


void
MprisPlugin::setLoopStatus( const QString& value )
{
    Tomahawk::playlistinterface_ptr p = AudioEngine::instance()->playlist();
    if ( p.isNull() )
        return;
    if ( value == "Track" )
        p->setRepeatMode( PlaylistInterface::RepeatOne );
    else if ( value == "Playlist" )
        p->setRepeatMode( PlaylistInterface::RepeatAll );
    else if ( value == "None" )
        p->setRepeatMode( PlaylistInterface::NoRepeat );
}


double
MprisPlugin::maximumRate() const
{
    return 1.0;
}


QVariantMap
MprisPlugin::metadata() const
{
    QVariantMap metadataMap;
    Tomahawk::result_ptr track = AudioEngine::instance()->currentTrack();
    if ( track )
    {
        metadataMap.insert( "mpris:trackid", QString( "/track/" ) + track->id().replace( "-", "" ) );
        metadataMap.insert( "mpris:length", track->duration() );
        metadataMap.insert( "xesam:album", track->album()->name() );
        metadataMap.insert( "xesam:artist", track->artist()->name() );
        metadataMap.insert( "xesam:title", track->track() );

        // Only return art if tempfile exists, and if its name contains the same "artist_album_tomahawk_cover.png"
        if ( m_coverTempFile && m_coverTempFile->exists() &&
             m_coverTempFile->fileName().contains( track->artist()->name() + "_" + track->album()->name() + "_tomahawk_cover.png" ) )
        {
            metadataMap.insert( "mpris:artUrl", QString( QUrl::fromLocalFile( QFileInfo( *m_coverTempFile ).absoluteFilePath() ).toEncoded() ) );
        }
        else
        {
            // Need to fetch the album cover

            Tomahawk::InfoSystem::InfoStringHash trackInfo;
            trackInfo["artist"] = track->artist()->name();
            trackInfo["album"] = track->album()->name();

            Tomahawk::InfoSystem::InfoRequestData requestData;
            requestData.caller = s_mpInfoIdentifier;
            requestData.type = Tomahawk::InfoSystem::InfoAlbumCoverArt;
            requestData.input = QVariant::fromValue< Tomahawk::InfoSystem::InfoStringHash >( trackInfo );
            requestData.customData = QVariantMap();

            Tomahawk::InfoSystem::InfoSystem::instance()->getInfo( requestData );
        }
    }

    return metadataMap;
}


double
MprisPlugin::minimumRate() const
{
    return 1.0;
}


QString
MprisPlugin::playbackStatus() const
{
    return m_playbackStatus;
}


qlonglong
MprisPlugin::position() const
{
    // Convert Tomahawk's milliseconds to microseconds
    return (qlonglong) ( AudioEngine::instance()->currentTime() * 1000 );
}


double
MprisPlugin::rate() const
{
    return 1.0;
}


void
MprisPlugin::setRate( double value )
{
    Q_UNUSED( value );
}


bool
MprisPlugin::shuffle() const
{
    Tomahawk::playlistinterface_ptr p = AudioEngine::instance()->playlist();
    if ( p.isNull() )
        return false;
    return p->shuffled();
}


void
MprisPlugin::setShuffle( bool value )
{
    Tomahawk::playlistinterface_ptr p = AudioEngine::instance()->playlist();
    if ( p.isNull() )
        return;
    return p->setShuffled( value );
}


double
MprisPlugin::volume() const
{
    return AudioEngine::instance()->volume();
}


void
MprisPlugin::setVolume( double value )
{
    AudioEngine::instance()->setVolume( value );
}


void
MprisPlugin::Next()
{
    AudioEngine::instance()->next();
}


void
MprisPlugin::OpenUri( const QString& Uri )
{
    if ( Uri.contains( "tomahawk://" ) )
        GlobalActionManager::instance()->parseTomahawkLink( Uri );
    else if ( Uri.contains( "spotify:" ) )
        GlobalActionManager::instance()->openSpotifyLink( Uri );
}


void
MprisPlugin::Pause()
{
    AudioEngine::instance()->pause();
}


void
MprisPlugin::Play()
{
    AudioEngine::instance()->play();
}


void
MprisPlugin::PlayPause()
{
    AudioEngine::instance()->playPause();
}


void
MprisPlugin::Previous()
{
    AudioEngine::instance()->previous();
}


void
MprisPlugin::Seek( qlonglong Offset )
{
    if ( !canSeek() )
        return;

    qlonglong seekTime = position() + Offset;
    if ( seekTime < 0 )
        AudioEngine::instance()->seek( 0 );
    else if ( seekTime > AudioEngine::instance()->currentTrackTotalTime()*1000 )
        Next();
    // seekTime is in microseconds, but we work internally in milliseconds
    else
        AudioEngine::instance()->seek( (qint64) ( seekTime / 1000 ) );

}


void
MprisPlugin::SetPosition( const QDBusObjectPath& TrackId, qlonglong Position )
{
    if ( !canSeek() )
        return;

    if ( TrackId.path() != QString( "/track/" ) + AudioEngine::instance()->currentTrack()->id().replace( "-", "" ) )
        return;

    if ( ( Position < 0) || ( Position > AudioEngine::instance()->currentTrackTotalTime()*1000 )  )
        return;

    AudioEngine::instance()->seek( (qint64) (Position / 1000 ) );

}


void
MprisPlugin::Stop()
{
    AudioEngine::instance()->stop();
}


// InfoPlugin Methods

void
MprisPlugin::getInfo( Tomahawk::InfoSystem::InfoRequestData requestData )
{
  Q_UNUSED( requestData );

  return;
}


void
MprisPlugin::pushInfo( Tomahawk::InfoSystem::InfoPushData pushData )
{
    bool isPlayingInfo = false;

    switch ( pushData.type )
    {
        case InfoNowPlaying:
          isPlayingInfo = true;
          audioStarted( pushData.infoPair.second );
          break;
        case InfoNowPaused:
          isPlayingInfo = true;
          audioPaused();
          break;
        case InfoNowResumed:
          isPlayingInfo = true;
          audioResumed( pushData.infoPair.second );
          break;
        case InfoNowStopped:
          isPlayingInfo = true;
          audioStopped();
          break;

        default:
          break;
    }

    if ( isPlayingInfo )
        notifyPropertyChanged( "org.mpris.MediaPlayer2.Player", "PlaybackStatus" );
}


void
MprisPlugin::stateChanged( AudioState newState, AudioState oldState )
{
    Q_UNUSED( newState );
    Q_UNUSED( oldState );
}


/** Audio state slots */
void
MprisPlugin::audioStarted( const QVariant& input )
{
    if ( !input.canConvert< QVariantMap >() )
        return;

    QVariantMap map = input.toMap();

    if ( map.contains( "private" ) && map[ "private" ].value< TomahawkSettings::PrivateListeningMode >() == TomahawkSettings::FullyPrivate )
        return;
    
    if ( !map.contains( "trackinfo" ) || !map[ "trackinfo" ].canConvert< Tomahawk::InfoSystem::InfoStringHash >() )
        return;

    InfoStringHash hash = map[ "trackinfo" ].value< Tomahawk::InfoSystem::InfoStringHash >();
    if ( !hash.contains( "title" ) || !hash.contains( "artist" ) || !hash.contains( "album" ) )
        return;

    m_playbackStatus = "Playing";
    notifyPropertyChanged( "org.mpris.MediaPlayer2.Player", "Metadata" );
}


void
MprisPlugin::audioFinished( const QVariant& input )
{
    Q_UNUSED( input );
}


void
MprisPlugin::audioStopped()
{
    m_playbackStatus = "Stopped";
}


void
MprisPlugin::audioPaused()
{
    m_playbackStatus = "Paused";
}


void
MprisPlugin::audioResumed( const QVariant& input )
{
    audioStarted( input );
}


void
MprisPlugin::onVolumeChanged( int volume )
{
    Q_UNUSED( volume );
    notifyPropertyChanged( "org.mpris.MediaPlayer2.Player", "Volume" );
}


void
MprisPlugin::onPlaylistChanged( Tomahawk::playlistinterface_ptr playlist )
{
    disconnect( this, SLOT( onTrackCountChanged( unsigned int ) ) );

    if ( !playlist.isNull() )
        connect( playlist.data(), SIGNAL( trackCountChanged( unsigned int ) ),
            SLOT( onTrackCountChanged( unsigned int ) ) );

    // Notify relevant changes
    notifyPropertyChanged( "org.mpris.MediaPlayer2.Player", "LoopStatus" );
    notifyPropertyChanged( "org.mpris.MediaPlayer2.Player", "Shuffle" );
    notifyPropertyChanged( "org.mpris.MediaPlayer2.Player", "CanSeek" );
    onTrackCountChanged( 0 );
}


void
MprisPlugin::onTrackCountChanged( unsigned int tracks )
{
    Q_UNUSED( tracks );
    notifyPropertyChanged( "org.mpris.MediaPlayer2.Player", "CanGoNext" );
    notifyPropertyChanged( "org.mpris.MediaPlayer2.Player", "CanGoPrevious" );

}


void
MprisPlugin::onSeeked( qint64 ms )
{
    qlonglong us = (qlonglong) ( ms*1000 );
    emit Seeked( us );
}


void
MprisPlugin::infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData requestData, QVariant output )
{
    // If the caller for the request was not us, or not the type of info we are seeking, ignore it
    if ( requestData.caller != s_mpInfoIdentifier || requestData.type != Tomahawk::InfoSystem::InfoAlbumCoverArt )
    {
        //notifyPropertyChanged( "org.mpris.MediaPlayer2.Player", "Metadata" );
        return;
    }

    if ( !output.canConvert< QVariantMap >() )
    {
        //notifyPropertyChanged( "org.mpris.MediaPlayer2.Player", "Metadata" );
        tDebug( LOGINFO ) << "Cannot convert fetched art from a QByteArray";
        return;
    }

    // Pull image data into byte array
    QVariantMap returnedData = output.value< QVariantMap >();
    const QByteArray ba = returnedData["imgbytes"].toByteArray();
    if ( ba.length() )
    {
        // Load from byte array to image
        QImage image;
        image.loadFromData( ba );

        // Pull out request data for album+artist
        if ( !requestData.input.canConvert< Tomahawk::InfoSystem::InfoStringHash >() )
        {
            qDebug() << "Cannot convert metadata input to album cover retrieval";
            return;
        }

        Tomahawk::InfoSystem::InfoStringHash hash = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash>();

        // delete the old tempfile and make new one, to avoid caching of filename by mpris clients
        if ( m_coverTempFile )
        {
            delete m_coverTempFile;
            m_coverTempFile = 0;
        }

        if ( image.isNull() )
            return;

        m_coverTempFile = new QTemporaryFile( QDir::toNativeSeparators( QDir::tempPath() + "/" + hash["artist"] + "_" + hash["album"] + "_tomahawk_cover.png" ) );
        if ( !m_coverTempFile->open() )
        {
            qDebug() << "WARNING: could not write temporary file for cover art!";
        }

        // Finally, save the image to the new temp file
        if ( image.save( m_coverTempFile, "PNG" ) )
        {
            qDebug() << "Saving cover image to:" << QFileInfo( *m_coverTempFile ).absoluteFilePath();
            m_coverTempFile->close();
            notifyPropertyChanged( "org.mpris.MediaPlayer2.Player", "Metadata" );
        }
        else
        {
            tDebug() << Q_FUNC_INFO << "failed to save cover image!";
            m_coverTempFile->close();
        }
    }
}


void
MprisPlugin::infoSystemFinished( QString target )
{
    Q_UNUSED( target );
}


void
MprisPlugin::notifyPropertyChanged( const QString& interface, const QString& propertyName )
{
    QDBusMessage signal = QDBusMessage::createSignal(
        "/org/mpris/MediaPlayer2",
        "org.freedesktop.DBus.Properties",
        "PropertiesChanged" );
    signal << interface;
    QVariantMap changedProps;
    changedProps.insert(propertyName, property(propertyName.toAscii()));
    signal << changedProps;
    signal << QStringList();
    QDBusConnection::sessionBus().send(signal);
}

