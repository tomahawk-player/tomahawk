/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2015, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright 2013,      Teo Mrnjavac <teo@kde.org>
 *   Copyright 2014,      Adrien Aubry <dridri85@gmail.com>
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
#include "TomahawkVersion.h"

#include "audio/MediaStream.h"
#include "utils/Logger.h"

#include <QApplication>
#include <QVarLengthArray>
#include <QFile>
#include <QDir>

#include <vlc/libvlc.h>
#include <vlc/libvlc_media.h>
#include <vlc/libvlc_media_player.h>
#include <vlc/libvlc_events.h>
#include <vlc/libvlc_version.h>

AudioOutput* AudioOutput::s_instance = 0;


AudioOutput*
AudioOutput::instance()
{
    return AudioOutput::s_instance;
}


AudioOutput::AudioOutput( QObject* parent )
    : QObject( parent )
    , m_currentState( Stopped )
    , m_currentStream( nullptr )
    , m_muted( false )
    , m_seekable( true )
    , m_autoDelete ( true )
    , m_volume( 1.0 )
    , m_currentTime( 0 )
    , m_totalTime( 0 )
    , m_justSeeked( false )
    , dspPluginCallback( nullptr )
    , m_vlcInstance( nullptr )
    , m_vlcPlayer( nullptr )
    , m_vlcMedia( nullptr )
{
    tDebug() << Q_FUNC_INFO;

    AudioOutput::s_instance = this;

    qRegisterMetaType<AudioOutput::AudioState>("AudioOutput::AudioState");

    const char* vlcArgs[] = {
        "--ignore-config",
        "--extraintf=logger",
        qApp->arguments().contains( "--verbose" ) ? "--verbose=3" : "",
        // "--no-plugins-cache",
        // "--no-media-library",
        // "--no-osd",
        // "--no-stats",
        // "--no-video-title-show",
        // "--no-snapshot-preview",
        // "--services-discovery=''",
        "--no-video",
        "--no-xlib"
    };

    // Create and initialize a libvlc instance (it should be done only once)
    m_vlcInstance = libvlc_new( sizeof(vlcArgs) / sizeof(*vlcArgs), vlcArgs );
    if ( !m_vlcInstance )
    {
        tDebug() << Q_FUNC_INFO << "libVLC: could not initialize";
        //FIXME PANIC, abort
    }
    libvlc_set_user_agent( m_vlcInstance, TOMAHAWK_APPLICATION_NAME,
                           TOMAHAWK_APPLICATION_NAME "/" TOMAHAWK_VERSION );
    // FIXME: icon is named tomahawk, so we need the lowercase application name
#if (LIBVLC_VERSION_INT >= LIBVLC_VERSION(2, 1, 0, 0))
    libvlc_set_app_id( m_vlcInstance, "org.tomahawk-player.desktop",
                       TOMAHAWK_VERSION, "tomahawk" );
#endif

    m_vlcPlayer = libvlc_media_player_new( m_vlcInstance );
    libvlc_event_manager_t* manager = libvlc_media_player_event_manager( m_vlcPlayer );
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
    for ( int i = 0; i < eventCount; i++ )
    {
        libvlc_event_attach( manager, events[ i ], &AudioOutput::vlcEventCallback, this );
    }

    tDebug() << Q_FUNC_INFO << "Init OK";
}


AudioOutput::~AudioOutput()
{
    tDebug() << Q_FUNC_INFO;

    if ( m_vlcPlayer != nullptr )
    {
        libvlc_media_player_stop( m_vlcPlayer );
        libvlc_media_player_release( m_vlcPlayer );
        m_vlcPlayer = nullptr;
    }
    if ( m_vlcMedia != nullptr )
    {
        libvlc_media_release( m_vlcMedia );
        m_vlcMedia = nullptr;
    }
    if ( m_vlcInstance != nullptr )
    {
        libvlc_release( m_vlcInstance );
    }
}


void
AudioOutput::setAutoDelete( bool ad )
{
    m_autoDelete = ad;
}


void
AudioOutput::setCurrentSource( const QUrl& stream )
{
    setCurrentSource( new MediaStream( stream ) );
}


void
AudioOutput::setCurrentSource( QIODevice* stream )
{
    setCurrentSource( new MediaStream( stream ) );
}


int readCallback( void* data, const char* cookie, int64_t* dts, int64_t* pts, unsigned* flags, size_t* bufferSize, void** buffer )
{
    MediaStream* mediaStream = static_cast< MediaStream * >( data );
    return mediaStream->readCallback( cookie, dts, pts, flags, bufferSize, buffer );
}


int
readDoneCallback( void* data, const char* cookie, size_t bufferSize, void* buffer )
{
    MediaStream* mediaStream = static_cast< MediaStream * >( data );
    return mediaStream->readDoneCallback( cookie, bufferSize, buffer );
}


void
AudioOutput::setCurrentSource( MediaStream* stream )
{
    tDebug() << Q_FUNC_INFO;

    setState( Loading );

    if ( m_vlcMedia != nullptr )
    {
        // Ensure playback is stopped, then release media
        libvlc_media_player_stop( m_vlcPlayer );
        libvlc_media_release( m_vlcMedia );
        m_vlcMedia = nullptr;
    }
    if ( m_autoDelete && m_currentStream != nullptr )
    {
        delete m_currentStream;
    }

    m_currentStream = stream;
    m_totalTime = 0;
    m_currentTime = 0;
    m_justSeeked = false;
    m_seekable = true;

    QByteArray url;
    switch ( stream->type() )
    {
        case MediaStream::Unknown:
            tDebug() << Q_FUNC_INFO << "MediaStream Type is Invalid:" << stream->type();
            break;

        case MediaStream::Empty:
            tDebug() << Q_FUNC_INFO << "MediaStream is empty.";
            break;

        case MediaStream::Url:
            tDebug() << Q_FUNC_INFO << "MediaStream::Url:" << stream->url();
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

    tDebug() << Q_FUNC_INFO << "MediaStream::Final Url:" << url;

    m_vlcMedia = libvlc_media_new_location( m_vlcInstance, url.constData() );
    libvlc_event_manager_t* manager = libvlc_media_event_manager( m_vlcMedia );
    libvlc_event_type_t events[] = {
        libvlc_MediaDurationChanged,
    };
    const int eventCount = sizeof(events) / sizeof( *events );
    for ( int i = 0; i < eventCount; i++ )
    {
        libvlc_event_attach( manager, events[ i ], &AudioOutput::vlcEventCallback, this );
    }

    libvlc_media_player_set_media( m_vlcPlayer, m_vlcMedia );

    if ( stream->type() == MediaStream::Url )
    {
        m_totalTime = libvlc_media_get_duration( m_vlcMedia );
    }
    else if ( stream->type() == MediaStream::Stream || stream->type() == MediaStream::IODevice )
    {
        libvlc_media_add_option_flag(m_vlcMedia, "imem-cat=4", libvlc_media_option_trusted);
        const char* imemData = QString( "imem-data=%1" ).arg( (uintptr_t)stream ).toLatin1().constData();
        libvlc_media_add_option_flag(m_vlcMedia, imemData, libvlc_media_option_trusted);
        const char* imemGet = QString( "imem-get=%1" ).arg( (uintptr_t)&readCallback ).toLatin1().constData();
        libvlc_media_add_option_flag(m_vlcMedia, imemGet, libvlc_media_option_trusted);
        const char* imemRelease = QString( "imem-release=%1" ).arg( (uintptr_t)&readDoneCallback ).toLatin1().constData();
        libvlc_media_add_option_flag(m_vlcMedia, imemRelease, libvlc_media_option_trusted);
        const char* imemSeek = QString( "imem-seek=%1" ).arg( (uintptr_t)&MediaStream::seekCallback ).toLatin1().constData();
        libvlc_media_add_option_flag(m_vlcMedia, imemSeek, libvlc_media_option_trusted);
    }

    setState( Stopped );
}


AudioOutput::AudioState
AudioOutput::state() const
{
    return m_currentState;
}


void
AudioOutput::setState( AudioState state )
{
    tDebug() << Q_FUNC_INFO;
    AudioState last = m_currentState;
    m_currentState = state;
    emit stateChanged( state, last );
}


qint64
AudioOutput::currentTime() const
{
    return m_currentTime;
}


void
AudioOutput::setCurrentPosition( float position )
{
    //tDebug() << Q_FUNC_INFO << position;
    emit positionChanged( position );
    m_havePosition = position > 0.0;
}


void
AudioOutput::setCurrentTime( qint64 time )
{
    // FIXME : This is a bit hacky, but m_totalTime is only used to determine
    // if we are about to finish
    if ( m_totalTime == 0 )
    {
        m_totalTime = AudioEngine::instance()->currentTrackTotalTime();
    }

    m_currentTime = time;
    emit tick( time );

    //    tDebug() << Q_FUNC_INFO << "Current time:" << m_currentTime << "/" << m_totalTime;

    // FIXME pt 2 : we use temporary variable to avoid overriding m_totalTime
    // in the case it is < 0 (which means that the media is not seekable)
    qint64 total = m_totalTime;
    if ( total <= 0 )
    {
        total = AudioEngine::instance()->currentTrackTotalTime();
    }

    if ( time <= 0 )
    {
        m_seekable = false;
    } else {
        m_seekable = true;
    }
}


qint64
AudioOutput::totalTime() const
{
    return m_totalTime;
}


void
AudioOutput::setTotalTime( qint64 time )
{
//    tDebug() << Q_FUNC_INFO << time;

    if ( time > 0 )
    {
        m_totalTime = time;
        // emit current time to refresh total time
        emit tick( time );
    }
}


void
AudioOutput::play()
{
    tDebug() << Q_FUNC_INFO;

    if ( libvlc_media_player_is_playing( m_vlcPlayer ) )
    {
        libvlc_media_player_set_pause( m_vlcPlayer, 0 );
    }
    else
    {
        libvlc_media_player_play( m_vlcPlayer );
    }

    setState( Playing );
}


void
AudioOutput::pause()
{
    tDebug() << Q_FUNC_INFO;

    libvlc_media_player_set_pause( m_vlcPlayer, 1 );
    setState( Paused );
}


void
AudioOutput::stop()
{
    tDebug() << Q_FUNC_INFO;

    libvlc_media_player_stop( m_vlcPlayer );
    setState( Stopped );
}


void
AudioOutput::seek( qint64 milliseconds )
{
    tDebug() << Q_FUNC_INFO;

    switch ( m_currentState )
    {
        case Playing:
        case Paused:
        case Loading:
        case Buffering:
            break;
        default:
            return;
    }

    if ( m_seekable )
    {

        //    tDebug() << Q_FUNC_INFO << "AudioOutput:: seeking" << milliseconds << "msec";
        libvlc_media_player_set_time( m_vlcPlayer, milliseconds );
        setCurrentTime( milliseconds );
    }
    else
    {
        qint64 duration = AudioEngine::instance()->currentTrackTotalTime();
        float position = float(float(milliseconds) / duration);
        libvlc_media_player_set_position(m_vlcPlayer, position);
        tDebug() << Q_FUNC_INFO << "AudioOutput:: seeking via position" << position << "pos";
    }
    m_justSeeked = true;
}


bool
AudioOutput::isSeekable() const
{
    tDebug() << Q_FUNC_INFO << m_seekable << m_havePosition << m_totalTime << libvlc_media_player_is_seekable( m_vlcPlayer );
    return !m_havePosition || (libvlc_media_player_is_seekable( m_vlcPlayer ) && m_totalTime > 0);
}


bool
AudioOutput::isMuted() const
{
    return m_muted;
}


void
AudioOutput::setMuted( bool m )
{
    tDebug() << Q_FUNC_INFO;

    m_muted = m;
    if ( m_muted )
    {
        libvlc_audio_set_volume( m_vlcPlayer, 0 );
    }
    else
    {
        libvlc_audio_set_volume( m_vlcPlayer, m_volume * 100.0 );
    }
}


qreal
AudioOutput::volume() const
{
    return m_muted ? 0 : m_volume;
}


void
AudioOutput::setVolume( qreal vol )
{
    tDebug() << Q_FUNC_INFO;

    m_volume = vol;
    if ( !m_muted )
    {
        libvlc_audio_set_volume( m_vlcPlayer, m_volume * 100.0 );
    }
}


void
AudioOutput::onVlcEvent( const libvlc_event_t* event )
{
    switch ( event->type )
    {
        case libvlc_MediaPlayerTimeChanged:
            setCurrentTime( event->u.media_player_time_changed.new_time );
            break;
        case libvlc_MediaPlayerPositionChanged:
            setCurrentPosition(event->u.media_player_position_changed.new_position); 
            break;
        case libvlc_MediaPlayerSeekableChanged:
         //   tDebug() << Q_FUNC_INFO << " : seekable changed : " << event->u.media_player_seekable_changed.new_seekable;
            break;
        case libvlc_MediaDurationChanged:
            setTotalTime( event->u.media_duration_changed.new_duration );
            break;
        case libvlc_MediaPlayerLengthChanged:
        //    tDebug() << Q_FUNC_INFO << " : length changed : " << event->u.media_player_length_changed.new_length;
            break;
        case libvlc_MediaPlayerNothingSpecial:
        case libvlc_MediaPlayerOpening:
        case libvlc_MediaPlayerBuffering:
        case libvlc_MediaPlayerPlaying:
        case libvlc_MediaPlayerPaused:
        case libvlc_MediaPlayerStopped:
            break;
        case libvlc_MediaPlayerEndReached:
            setState( Stopped );
            break;
        case libvlc_MediaPlayerEncounteredError:
            tDebug() << Q_FUNC_INFO << "LibVLC error : MediaPlayerEncounteredError. Stopping";
            if ( m_vlcPlayer != nullptr )
            {
                stop();
            }
            setState( Error );
            break;
        case libvlc_MediaPlayerVout:
        case libvlc_MediaPlayerMediaChanged:
        case libvlc_MediaPlayerForward:
        case libvlc_MediaPlayerBackward:
        case libvlc_MediaPlayerPausableChanged:
        case libvlc_MediaPlayerTitleChanged:
        case libvlc_MediaPlayerSnapshotTaken:
        default:
            break;
    }
}


void
AudioOutput::vlcEventCallback( const libvlc_event_t* event, void* opaque )
{
//    tDebug() << Q_FUNC_INFO;

    AudioOutput* that = reinterpret_cast < AudioOutput * > ( opaque );
    Q_ASSERT( that );

    that->onVlcEvent( event );
}


void
AudioOutput::s_dspCallback( int frameNumber, float* samples, int nb_channels, int nb_samples )
{
//    tDebug() << Q_FUNC_INFO;

    int state = AudioOutput::instance()->m_justSeeked ? 1 : 0;
    AudioOutput::instance()->m_justSeeked = false;
    if ( AudioOutput::instance()->dspPluginCallback )
    {
        AudioOutput::instance()->dspPluginCallback( state, frameNumber, samples, nb_channels, nb_samples );
    }
}


void
AudioOutput::setDspCallback( std::function< void( int, int, float*, int, int ) > cb )
{
    dspPluginCallback = cb;
}


libvlc_instance_t*
AudioOutput::vlcInstance() const
{
    return m_vlcInstance;
}
