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

#ifndef AUDIOENGINE_H
#define AUDIOENGINE_H

#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtCore/QMutex>

#include <phonon/MediaObject>
#include <phonon/AudioOutput>
#include <phonon/BackendCapabilities>

#include "infosystem/infosystem.h"

#include "result.h"
#include "typedefs.h"

#include "dllmacro.h"

#define AUDIO_VOLUME_STEP 5

namespace Tomahawk
{
    class PlaylistInterface;
}

class DLLEXPORT AudioEngine : public QObject
{
Q_OBJECT

public:
    enum AudioErrorCode { StreamReadError, AudioDeviceError, DecodeError };
    enum AudioState { Stopped, Playing, Paused };

    static AudioEngine* instance();

    explicit AudioEngine();
    ~AudioEngine();

    QStringList supportedMimeTypes() const;
    unsigned int volume() const {  return m_volume; } // in percent

    AudioState state() const { QMutexLocker locker( &m_stateMutex ); return m_state; }
    bool isPlaying() const { QMutexLocker locker( &m_stateMutex ); return m_state == Playing; }
    bool isPaused() const { QMutexLocker locker( &m_stateMutex ); return m_state == Paused; }
    bool isStopped() const { QMutexLocker locker( &m_stateMutex ); return m_state == Stopped; }

    /* Returns the PlaylistInterface of the currently playing track. Note: This might be different to the current playlist! */
    Tomahawk::PlaylistInterface* currentTrackPlaylist() const { QMutexLocker locker( &m_currentTrackPlaylistMutex ); return m_currentTrackPlaylist.data(); }

    /* Returns the PlaylistInterface of the current playlist. Note: The currently playing track might still be from a different playlist! */
    Tomahawk::PlaylistInterface* playlist() const { QMutexLocker locker( &m_playlistMutex ); return m_playlist.data(); }

    Tomahawk::result_ptr currentTrack() const { QMutexLocker locker( &m_currentTrackMutex ); return m_currentTrack; }

    qint64 currentTime() const { QMutexLocker locker( &m_currentTrackMutex ); return m_currentTime; }
    qint64 currentTrackTotalTime() const { QMutexLocker locker( &m_currentTrackMutex ); return m_currentTrackTotalTime; }

public slots:
    void playPause();
    void play();
    void pause();
    void stop();

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
    void onVolumeChanged( qreal volume ) { QMutexLocker locker( &m_volumeMutex ); m_volume = volume * 100; emit volumeChanged( volume * 100 ); }
    void mute();

    void playItem( Tomahawk::PlaylistInterface* playlist, const Tomahawk::result_ptr& result );
    void setPlaylist( Tomahawk::PlaylistInterface* playlist );
    void setQueue( Tomahawk::PlaylistInterface* queue );

    void playlistNextTrackReady();

    void infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData requestData, QVariant output );
    void infoSystemFinished( QString caller );

    void togglePrivateListeningMode();

signals:
    void loading( const Tomahawk::result_ptr& track );
    void started( const Tomahawk::result_ptr& track );
    void finished( const Tomahawk::result_ptr& track );
    void stopped();
    void paused();
    void resumed();

    void seeked( qint64 ms );

    void stateChanged( AudioState newState, AudioState oldState );
    void volumeChanged( int volume /* in percent */ );

    void timerMilliSeconds( qint64 msElapsed );
    void timerSeconds( unsigned int secondsElapsed );
    void timerPercentage( unsigned int percentage );

    void playlistChanged( Tomahawk::PlaylistInterface* playlist );

    void error( AudioErrorCode errorCode );

private slots:
    bool loadTrack( const Tomahawk::result_ptr& result );
    void loadPreviousTrack();
    void loadNextTrack();

    void onAboutToFinish();
    void onStateChanged( Phonon::State newState, Phonon::State oldState );
    void timerTriggered( qint64 time );

    void setCurrentTrack( const Tomahawk::result_ptr& result );

private:
    void setState( AudioState state );

    bool isHttpResult( const QString& ) const;
    bool isLocalResult( const QString& ) const;

    void sendWaitingNotification() const;
    void sendNowPlayingNotification();

    bool m_isPlayingHttp;
    QSharedPointer<QIODevice> m_input;

    Tomahawk::result_ptr m_currentTrack;
    Tomahawk::result_ptr m_lastTrack;
    QWeakPointer< Tomahawk::PlaylistInterface > m_playlist;
    QWeakPointer< Tomahawk::PlaylistInterface > m_currentTrackPlaylist;
    Tomahawk::PlaylistInterface* m_queue;

    Phonon::MediaObject* m_mediaObject;
    Phonon::AudioOutput* m_audioOutput;

    unsigned int m_timeElapsed;
    bool m_expectStop;
    bool m_waitingOnNewTrack;
    bool m_infoSystemConnected;

    mutable QStringList m_supportedMimeTypes;
    AudioState m_state;

    static AudioEngine* s_instance;

    mutable QMutex m_stateMutex;
    mutable QMutex m_mimeTypeMutex;
    mutable QMutex m_currentTrackMutex;
    mutable QMutex m_currentTrackPlaylistMutex;
    mutable QMutex m_playlistMutex;
    mutable QMutex m_queueMutex;
    mutable QMutex m_volumeMutex;

    qint64 m_currentTime;
    qint64 m_currentTrackTotalTime;
    unsigned int m_volume;
};

#endif // AUDIOENGINE_H
