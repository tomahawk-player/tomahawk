/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2012, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#ifndef AUDIOENGINE_H
#define AUDIOENGINE_H

#include "../Typedefs.h" // PlaylistModes

#include "playlistinterface_ptr.h"
#include "result_ptr.h"
#include "query_ptr.h"
#include "artist_ptr.h"
#include "album_ptr.h"

// #include <QStringList>

#include "DllMacro.h"

class AudioEnginePrivate;

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
    unsigned int volume() const; // in percent

    AudioState state() const;
    bool isPlaying() const;
    bool isPaused() const;
    bool isStopped() const;

    /* Returns the PlaylistInterface of the currently playing track. Note: This might be different to the current playlist! */
    Tomahawk::playlistinterface_ptr currentTrackPlaylist() const;

    /* Returns the PlaylistInterface of the current playlist. Note: The currently playing track might still be from a different playlist! */
    Tomahawk::playlistinterface_ptr playlist() const;

    Tomahawk::result_ptr currentTrack() const;
    Tomahawk::query_ptr stopAfterTrack() const;

    qint64 currentTime() const;
    qint64 currentTrackTotalTime() const;

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
    void loadTrack( const Tomahawk::result_ptr& result ); //async!
    void performLoadTrack( const Tomahawk::result_ptr& result, QSharedPointer< QIODevice >& io ); //only call from loadTrack kthxbi
    void loadPreviousTrack();
    void loadNextTrack();

    void onAboutToFinish();
    void onVolumeChanged( qreal volume );
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

    Q_DECLARE_PRIVATE( AudioEngine );
    AudioEnginePrivate* d_ptr;
};

#endif // AUDIOENGINE_H
