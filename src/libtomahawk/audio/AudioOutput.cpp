/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2014, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright 2013,      Teo Mrnjavac <teo@kde.org>
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

#include "AudioEngine.h"
#include "AudioOutput.h"

#include "utils/Logger.h"

#include <QVarLengthArray>
#include <QFile>
#include <QDir>

#include <boost/function.hpp>
#include <boost/bind.hpp>

#include <vlc/libvlc.h>
#include <vlc/libvlc_media.h>
#include <vlc/libvlc_media_player.h>
#include <vlc/libvlc_events.h>
#include <vlc/libvlc_version.h>

static QString s_aeInfoIdentifier = QString( "AUDIOOUTPUT" );

static const int ABOUT_TO_FINISH_TIME = 2000;

AudioOutput* AudioOutput::s_instance = 0;


AudioOutput*
AudioOutput::instance()
{
    return AudioOutput::s_instance;
}


AudioOutput::AudioOutput( QObject* parent )
    : QObject( parent )
    , currentState( Stopped )
    , muted( false )
    , m_autoDelete ( true )
    , m_volume( 1.0 )
    , m_currentTime( 0 )
    , m_totalTime( 0 )
    , m_aboutToFinish( false )
    , dspPluginCallback( 0 )
    , vlcInstance( 0 )
    , vlcPlayer( 0 )
    , vlcMedia( 0 )
{
    tDebug() << Q_FUNC_INFO;

    AudioOutput::s_instance = this;
    currentStream = 0;

    qRegisterMetaType<AudioOutput::AudioState>("AudioOutput::AudioState");
    
    QList<QByteArray> args;

    args << "--ignore-config";
    args << "--verbose=42";
    args << "--no-plugins-cache";
    args << "--extraintf=logger";
    args << "--no-media-library";
    args << "--no-osd";
    args << "--no-stats";
    args << "--no-video-title-show";
    args << "--no-snapshot-preview";
    args << "--no-xlib";
    args << "--services-discovery=''";
//    args << "--no-one-instance";
    args << "--no-video";
//    args << "--audio-filter=dsp";
//    args << QString("--dsp-callback=%1").arg((quint64)&AudioOutput::s_dspCallback, 0, 16).toAscii();

    QVarLengthArray< const char * , 64 > vlcArgs( args.size() );
    for ( int i = 0 ; i < args.size() ; ++i ) {
        vlcArgs[i] = args.at( i ).constData();
        tDebug() << args.at( i );
    }

    // Create and initialize a libvlc instance (it should be done only once)
    if ( !( vlcInstance = libvlc_new( vlcArgs.size(), vlcArgs.constData() ) ) ) {
        tDebug() << "libVLC: could not initialize";
    }


    vlcPlayer = libvlc_media_player_new( vlcInstance );

    libvlc_event_manager_t* manager = libvlc_media_player_event_manager( vlcPlayer );
    libvlc_event_type_t events[] = {
        libvlc_MediaPlayerMediaChanged,
        libvlc_MediaPlayerNothingSpecial,
        libvlc_MediaPlayerOpening,
        libvlc_MediaPlayerBuffering,
        libvlc_MediaPlayerPlaying,
        libvlc_MediaPlayerPaused,
        libvlc_MediaPlayerStopped,
        libvlc_MediaPlayerForward,
        libvlc_MediaPlayerBackward,
        libvlc_MediaPlayerEndReached,
        libvlc_MediaPlayerEncounteredError,
        libvlc_MediaPlayerTimeChanged,
        libvlc_MediaPlayerPositionChanged,
        libvlc_MediaPlayerSeekableChanged,
        libvlc_MediaPlayerPausableChanged,
        libvlc_MediaPlayerTitleChanged,
        libvlc_MediaPlayerSnapshotTaken,
        //libvlc_MediaPlayerLengthChanged,
        libvlc_MediaPlayerVout
    };
    const int eventCount = sizeof(events) / sizeof( *events );
    for ( int i = 0 ; i < eventCount ; i++ ) {
        libvlc_event_attach( manager, events[ i ], &AudioOutput::vlcEventCallback, this );
    }

    tDebug() << "AudioOutput::AudioOutput OK !\n";
}


AudioOutput::~AudioOutput()
{
    tDebug() << Q_FUNC_INFO;
}


void
AudioOutput::setAutoDelete ( bool ad )
{
    m_autoDelete = ad;
}

void
AudioOutput::setCurrentSource(MediaStream stream)
{
    setCurrentSource( new MediaStream(stream) );
}

void
AudioOutput::setCurrentSource(MediaStream* stream)
{
    tDebug() << Q_FUNC_INFO;

    setState(Loading);

    if ( vlcMedia != 0 ) {
        // Ensure playback is stopped, then release media
        libvlc_media_player_stop( vlcPlayer );
        libvlc_media_release( vlcMedia );
        vlcMedia = 0;
    }
    if ( m_autoDelete && currentStream != 0 ) {
        delete currentStream;
    }
    currentStream = stream;
    m_totalTime = 0;
    m_currentTime = 0;

    QByteArray url;
    switch (stream->type()) {
        case MediaStream::Unknown:
            tDebug() << "MediaStream Type is Invalid:" << stream->type();
            break;

        case MediaStream::Empty:
            tDebug() << "MediaStream is empty.";
            break;

        case MediaStream::Url:
            tDebug() << "MediaStream::Url:" << stream->url();
            if ( stream->url().scheme().isEmpty() ) {
                url = "file:///";
                if ( stream->url().isRelative() ) {
                    url.append( QFile::encodeName( QDir::currentPath() ) + '/' );
                }
            }
            url += stream->url().toEncoded();
            break;

        case MediaStream::Stream:
        case MediaStream::IODevice:
            url = QByteArray( "imem://" );
            break;
    }

    tDebug() << "MediaStream::Final Url:" << url;


    vlcMedia = libvlc_media_new_location( vlcInstance, url.constData() );

    libvlc_event_manager_t* manager = libvlc_media_event_manager( vlcMedia );
    libvlc_event_type_t events[] = {
        libvlc_MediaDurationChanged,
    };
    const int eventCount = sizeof(events) / sizeof( *events );
    for ( int i = 0 ; i < eventCount ; i++ ) {
        libvlc_event_attach( manager, events[ i ], &AudioOutput::vlcEventCallback, this );
    }

    libvlc_media_player_set_media( vlcPlayer, vlcMedia );


    if ( stream->type() == MediaStream::Url ) {
        m_totalTime = libvlc_media_get_duration( vlcMedia );
    }
    else if ( stream->type() == MediaStream::Stream || stream->type() == MediaStream::IODevice ) {
        libvlc_media_add_option_flag(vlcMedia, "imem-cat=4", libvlc_media_option_trusted);
        libvlc_media_add_option_flag(vlcMedia, (QString("imem-data=") + QString::number((quint64)stream)).toUtf8().data(), libvlc_media_option_trusted);
        libvlc_media_add_option_flag(vlcMedia, (QString("imem-get=") + QString::number((quint64)&MediaStream::readCallback)).toUtf8().data(), libvlc_media_option_trusted);
        libvlc_media_add_option_flag(vlcMedia, (QString("imem-release=") + QString::number((quint64)&MediaStream::readDoneCallback)).toUtf8().data(), libvlc_media_option_trusted);
        libvlc_media_add_option_flag(vlcMedia, (QString("imem-seek=") + QString::number((quint64)&MediaStream::seekCallback)).toUtf8().data(), libvlc_media_option_trusted);
    }

    m_aboutToFinish = false;
    setState(Stopped);
}


AudioOutput::AudioState
AudioOutput::state()
{
    tDebug() << Q_FUNC_INFO;
    return currentState;
}


void
AudioOutput::setState( AudioState state )
{
    tDebug() << Q_FUNC_INFO;
    AudioState last = currentState;
    currentState = state;
    emit stateChanged ( state, last );
}


qint64
AudioOutput::currentTime()
{
    return m_currentTime;
}


void
AudioOutput::setCurrentTime( qint64 time )
{
    // TODO : This is a bit hacky, but m_totalTime is only used to determine
    // if we are about to finish
    if ( m_totalTime <= 0 ) {
        m_totalTime = AudioEngine::instance()->currentTrackTotalTime();
    }

    m_currentTime = time;
    emit tick( time );

//    tDebug() << "Current time : " << m_currentTime << " / " << m_totalTime;

    if ( time < m_totalTime - ABOUT_TO_FINISH_TIME ) {
        m_aboutToFinish = false;
    }
    if ( !m_aboutToFinish && m_totalTime > 0 && time >= m_totalTime - ABOUT_TO_FINISH_TIME ) {
        m_aboutToFinish = true;
        emit aboutToFinish();
    }
}


qint64
AudioOutput::totalTime()
{
    return m_totalTime;
}


void
AudioOutput::setTotalTime( qint64 time )
{
    if ( time > 0 ) {
        m_totalTime = time;
        // emit current time to refresh total time
        emit tick( m_currentTime );
    }
}


void
AudioOutput::play()
{
    tDebug() << Q_FUNC_INFO;
    if ( libvlc_media_player_is_playing ( vlcPlayer ) ) {
        libvlc_media_player_set_pause ( vlcPlayer, 0 );
    } else {
        libvlc_media_player_play ( vlcPlayer );
    }

    setState( Playing );
}


void
AudioOutput::pause()
{
    tDebug() << Q_FUNC_INFO;
//    libvlc_media_player_pause( vlcPlayer );
    libvlc_media_player_set_pause ( vlcPlayer, 1 );

    setState( Paused );
}


void
AudioOutput::stop()
{
    tDebug() << Q_FUNC_INFO;
    libvlc_media_player_stop ( vlcPlayer );

    setState( Stopped );
}


void
AudioOutput::seek( qint64 milliseconds )
{
    tDebug() << Q_FUNC_INFO;

    switch ( currentState ) {
        case Playing:
        case Paused:
        case Loading:
        case Buffering:
            break;
        default:
            return;
    }

    tDebug() << "AudioOutput:: seeking" << milliseconds << "msec";

    libvlc_media_player_set_time ( vlcPlayer, milliseconds );
    setCurrentTime( milliseconds );
}


bool
AudioOutput::isMuted()
{
    tDebug() << Q_FUNC_INFO;

    return muted;
}


void
AudioOutput::setMuted(bool m)
{
    tDebug() << Q_FUNC_INFO;

    muted = m;
    if ( muted == true ) {
        libvlc_audio_set_volume( vlcPlayer, 0 );
    } else {
        libvlc_audio_set_volume( vlcPlayer, m_volume * 100.0 );
    }
}


qreal
AudioOutput::volume()
{
    tDebug() << Q_FUNC_INFO;

    return muted ? 0 : m_volume;
}


void 
AudioOutput::setVolume(qreal vol)
{
    tDebug() << Q_FUNC_INFO;

    m_volume = vol;
    if ( !muted ) {
        libvlc_audio_set_volume( vlcPlayer, m_volume * 100.0 );
    }
}


void
AudioOutput::vlcEventCallback( const libvlc_event_t* event, void* opaque )
{
//    tDebug() << Q_FUNC_INFO;

    AudioOutput* that = reinterpret_cast < AudioOutput * > ( opaque );
    Q_ASSERT( that );

    switch (event->type) {
        case libvlc_MediaPlayerTimeChanged:
            that->setCurrentTime( event->u.media_player_time_changed.new_time );
            break;
        case libvlc_MediaPlayerSeekableChanged:
            //TODO, bool event->u.media_player_seekable_changed.new_seekable
            break;
        case libvlc_MediaDurationChanged:
            that->setTotalTime( event->u.media_duration_changed.new_duration );
            break;
        /*
        case libvlc_MediaPlayerLengthChanged:
            that->setTotalTime( event->u.media_player_length_changed.new_length );
            break;
        */
        case libvlc_MediaPlayerNothingSpecial:
        case libvlc_MediaPlayerOpening:
        case libvlc_MediaPlayerBuffering:
        case libvlc_MediaPlayerPlaying:
        case libvlc_MediaPlayerPaused:
        case libvlc_MediaPlayerStopped:
            break;
        case libvlc_MediaPlayerEndReached:
            that->setState(Stopped);
            break;
        case libvlc_MediaPlayerEncounteredError:
            // TODO emit Error
            break;
        case libvlc_MediaPlayerVout:
        case libvlc_MediaPlayerMediaChanged:
        case libvlc_MediaPlayerForward:
        case libvlc_MediaPlayerBackward:
        case libvlc_MediaPlayerPositionChanged:
        case libvlc_MediaPlayerPausableChanged:
        case libvlc_MediaPlayerTitleChanged:
        case libvlc_MediaPlayerSnapshotTaken:
        default:
            break;
    }
}


void
AudioOutput::s_dspCallback( signed short* samples, int nb_channels, int nb_samples )
{
    tDebug() << Q_FUNC_INFO;

    if ( AudioOutput::instance()->dspPluginCallback ) {
        AudioOutput::instance()->dspPluginCallback( samples, nb_channels, nb_samples );
    }
}


void
AudioOutput::setDspCallback( void ( *cb ) ( signed short*, int, int ) )
{
    dspPluginCallback = cb;
}
