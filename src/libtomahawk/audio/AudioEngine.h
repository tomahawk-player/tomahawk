/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2012, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
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

#ifndef AUDIOENGINE_H
#define AUDIOENGINE_H

#include "infosystem/InfoSystem.h"
#include "Typedefs.h"
#include "Result.h"
#include "PlaylistInterface.h"

#include "DllMacro.h"

#include <phonon/MediaObject>
#include <phonon/AudioOutput>
#include <phonon/BackendCapabilities>

#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtCore/QQueue>


class DLLEXPORT AudioEngine : public QObject
{
Q_OBJECT

public:
    enum AudioErrorCode { StreamReadError, AudioDeviceError, DecodeError, UnknownError, NoError };
    enum AudioState { Stopped = 0, Playing = 1, Paused = 2, Error = 3, Loading = 4 };

    static AudioEngine* instance();

    explicit AudioEngine();
    ~AudioEngine();

    QStringList supportedMimeTypes() const;
    unsigned int volume() const { return m_audioOutput->volume() * 100.0; } // in percent

    AudioState state() const { return m_state; }
    bool isPlaying() const { return m_state == Playing; }
    bool isPaused() const { return m_state == Paused; }
    bool isStopped() const { return m_state == Stopped; }

    /* Returns the PlaylistInterface of the currently playing track. Note: This might be different to the current playlist! */
    Tomahawk::playlistinterface_ptr currentTrackPlaylist() const { return m_currentTrackPlaylist; }

    /* Returns the PlaylistInterface of the current playlist. Note: The currently playing track might still be from a different playlist! */
    Tomahawk::playlistinterface_ptr playlist() const { return m_playlist; }

    Tomahawk::result_ptr currentTrack() const { return m_currentTrack; }
    Tomahawk::query_ptr stopAfterTrack() const  { return m_stopAfterTrack; }

    qint64 currentTime() const { return m_mediaObject->currentTime(); }
    qint64 currentTrackTotalTime() const { return m_mediaObject->totalTime(); }

public slots:
    void playPause();
    void play();
    void pause();
    void stop( AudioErrorCode errorCode = NoError );

    void previous();
    void next();

    bool canGoPrevious();
    bool canGoNext();
    bool canSeek();

    void seek( qint64 ms );
    void seek( int ms ); // for compatibility with seekbar in audiocontrols
    void setVolume( int percentage );
    void lowerVolume();
    void raiseVolume();
    void mute();

    void playItem( Tomahawk::playlistinterface_ptr playlist, const Tomahawk::result_ptr& result, const Tomahawk::query_ptr& fromQuery = Tomahawk::query_ptr() );
    void playItem( Tomahawk::playlistinterface_ptr playlist, const Tomahawk::query_ptr& query );
    void playItem( const Tomahawk::artist_ptr& artist );
    void playItem( const Tomahawk::album_ptr& album );
    void setPlaylist( Tomahawk::playlistinterface_ptr playlist );
    void setQueue( const Tomahawk::playlistinterface_ptr& queue );

    void setStopAfterTrack( const Tomahawk::query_ptr& query );

    void setRepeatMode( Tomahawk::PlaylistModes::RepeatMode mode );
    void setShuffled( bool enabled );

signals:
    void loading( const Tomahawk::result_ptr& track );
    void started( const Tomahawk::result_ptr& track );
    void finished( const Tomahawk::result_ptr& track );
    void stopped();
    void paused();
    void resumed();

    void stopAfterTrackChanged();

    void seeked( qint64 ms );

    void shuffleModeChanged( bool enabled );
    void repeatModeChanged( Tomahawk::PlaylistModes::RepeatMode mode );
    void controlStateChanged();
    void stateChanged( AudioState newState, AudioState oldState );
    void volumeChanged( int volume /* in percent */ );

    void timerMilliSeconds( qint64 msElapsed );
    void timerSeconds( unsigned int secondsElapsed );
    void timerPercentage( unsigned int percentage );

    void playlistChanged( Tomahawk::playlistinterface_ptr playlist );

    void error( AudioEngine::AudioErrorCode errorCode );

private slots:
    bool loadTrack( const Tomahawk::result_ptr& result );
    void loadPreviousTrack();
    void loadNextTrack();

    void onAboutToFinish();
    void onStateChanged( Phonon::State newState, Phonon::State oldState );
    void onVolumeChanged( qreal volume ) { emit volumeChanged( volume * 100 ); }
    void timerTriggered( qint64 time );

    void setCurrentTrack( const Tomahawk::result_ptr& result );
    void onNowPlayingInfoReady( const Tomahawk::InfoSystem::InfoType type );
    void onPlaylistNextTrackAvailable();

    void sendNowPlayingNotification( const Tomahawk::InfoSystem::InfoType type );
    void sendWaitingNotification() const;

    void queueStateSafety();

private:
    void checkStateQueue();
    void queueState( AudioState state );

    void setState( AudioState state );

    bool isHttpResult( const QString& ) const;
    bool isLocalResult( const QString& ) const;

    QSharedPointer<QIODevice> m_input;

    Tomahawk::query_ptr m_stopAfterTrack;
    Tomahawk::result_ptr m_currentTrack;
    Tomahawk::playlistinterface_ptr m_playlist;
    Tomahawk::playlistinterface_ptr m_currentTrackPlaylist;
    Tomahawk::playlistinterface_ptr m_queue;

    Phonon::MediaObject* m_mediaObject;
    Phonon::AudioOutput* m_audioOutput;

    unsigned int m_timeElapsed;
    bool m_expectStop;
    bool m_waitingOnNewTrack;

    mutable QStringList m_supportedMimeTypes;

    AudioState m_state;
    QQueue< AudioState > m_stateQueue;
    QTimer m_stateQueueTimer;

    static AudioEngine* s_instance;
};

#endif // AUDIOENGINE_H
