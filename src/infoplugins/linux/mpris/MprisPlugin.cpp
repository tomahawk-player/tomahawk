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
#include <QtPlugin>

#include "audio/AudioEngine.h"
#include "infosystem/InfoSystemWorker.h"
#include "Album.h"
#include "Artist.h"
#include "Result.h"
#include "TomahawkSettings.h"
#include "GlobalActionManager.h"
#include "utils/Logger.h"
#include "utils/TomahawkUtils.h"
#include "audio/AudioEngine.h"
#include "Source.h"

#include "MprisPlugin.h"
#include "MprisPluginRootAdaptor.h"
#include "MprisPluginPlayerAdaptor.h"

namespace Tomahawk
{

namespace InfoSystem
{

static QString s_mpInfoIdentifier = QString( "MPRISPLUGIN" );

MprisPlugin::MprisPlugin()
    : InfoPlugin()
{
    // init
    m_playbackStatus = "Stopped";

    // Types of pushInfo we care about
    m_supportedPushTypes << InfoNowPlaying << InfoNowPaused << InfoNowResumed << InfoNowStopped;
}


MprisPlugin::~MprisPlugin()
{
}


void
MprisPlugin::init()
{
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
        connect( playlist.data(), SIGNAL( itemCountChanged( unsigned int ) ),
                                    SLOT( onTrackCountChanged( unsigned int ) ) );
    }

    // Connect to AudioEngine's seeked signal
    connect( AudioEngine::instance(), SIGNAL( seeked( qint64 ) ),
                                        SLOT( onSeeked( qint64 ) ) );
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
    return p->seekRestrictions() != PlaylistModes::NoSeek;

}


QString
MprisPlugin::loopStatus() const
{
    Tomahawk::playlistinterface_ptr p = AudioEngine::instance()->playlist();
    if ( p.isNull() )
        return "None";
    PlaylistModes::RepeatMode mode = p->repeatMode();
    switch( mode )
    {
        case PlaylistModes::RepeatOne:
            return "Track";
            break;
        case PlaylistModes::RepeatAll:
            return "Playlist";
            break;
        case PlaylistModes::NoRepeat:
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
        p->setRepeatMode( PlaylistModes::RepeatOne );
    else if ( value == "Playlist" )
        p->setRepeatMode( PlaylistModes::RepeatAll );
    else if ( value == "None" )
        p->setRepeatMode( PlaylistModes::NoRepeat );
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
        metadataMap.insert( "mpris:trackid", QVariant::fromValue(QDBusObjectPath(QString( "/track/" ) + track->id().replace( "-", "" ))) );
        metadataMap.insert( "mpris:length", static_cast<qlonglong>(track->duration()) * 1000000 );
        metadataMap.insert( "xesam:album", track->album()->name() );
        metadataMap.insert( "xesam:artist", QStringList( track->artist()->name() ) );
        metadataMap.insert( "xesam:title", track->track() );

        // Only return art if tempfile exists, and if its name contains the same "artist_album_tomahawk_cover.png"
        if ( !m_coverTempFile.isEmpty() )
        {
            QFile coverFile( m_coverTempFile );
            if ( coverFile.exists() && coverFile.fileName().contains( track->artist()->name() + "_" + track->album()->name() + "_tomahawk_cover.png" ) )
                metadataMap.insert( "mpris:artUrl", QString( QUrl::fromLocalFile( m_coverTempFile ).toEncoded() ) );
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
    return static_cast<double>(AudioEngine::instance()->volume()) / 100.0;
}


void
MprisPlugin::setVolume( double value )
{
    AudioEngine::instance()->setVolume( value * 100 );
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

    if ( !map.contains( "trackinfo" ) || !map[ "trackinfo" ].canConvert< Tomahawk::InfoSystem::InfoStringHash >() )
        return;

    InfoStringHash hash = map[ "trackinfo" ].value< Tomahawk::InfoSystem::InfoStringHash >();
    if ( !hash.contains( "title" ) || !hash.contains( "artist" ) || !hash.contains( "album" ) )
        return;

    m_playbackStatus = "Playing";

    if ( map.contains( "coveruri" ) )
        m_coverTempFile = map[ "coveruri" ].toString();

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
        connect( playlist.data(), SIGNAL( itemCountChanged( unsigned int ) ),
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

} //ns InfoSystem

} //ns Tomahawk

Q_EXPORT_PLUGIN2( Tomahawk::InfoSystem::InfoPlugin, Tomahawk::InfoSystem::MprisPlugin )
