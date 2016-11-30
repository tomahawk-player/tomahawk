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

#ifndef AUDIOENGINE_H
#define AUDIOENGINE_H

#include "../Typedefs.h"

#include <QStringList>
#include <functional>

#include "DllMacro.h"

class NetworkReply;
class AudioEnginePrivate;

class DLLEXPORT AudioEngine : public QObject
{
Q_OBJECT

public:
    enum AudioErrorCode { StreamReadError, AudioDeviceError, DecodeError, UnknownError, NoError };
    enum AudioState { Stopped = 0, Playing = 1, Paused = 2, Error = 3, Loading = 4 };
    enum AudioChannel { LeftChannel, LeftSurroundChannel, RightChannel , RightSurroundChannel, CenterChannel , SubwooferChannel };

    static AudioEngine* instance();

    explicit AudioEngine();
    ~AudioEngine();

    /**
     * List the MIME types we can play.
     *
     * This list might not include all possible MIME types that can be played.
     * If you know that a certain service always returns a specific MIME type
     * not reported here, we still might be able to play this media. Please
     * check the individual backends why they do not report it and if every
     * (recent) version supports this type.
     *
     * @return A list of playable MIME types.
     */
    QStringList supportedMimeTypes() const;

    /**
     * Reports the current set volume in percent (0-100).
     *
     * @return Current volume in percent.
     */
    unsigned int volume() const;

    AudioState state() const;
    bool isPlaying() const;
    bool isPaused() const;
    bool isStopped() const;
    bool isMuted() const;

    /**
     * Returns the PlaylistInterface of the currently playing track.
     *
     * Note: This might be different to the current playlist!
     */
    Tomahawk::playlistinterface_ptr currentTrackPlaylist() const;

    /**
     * Returns the PlaylistInterface of the current playlist.
     *
     * Note: The currently playing track might still be from a different
     * playlist!
     */
    Tomahawk::playlistinterface_ptr playlist() const;

    Tomahawk::result_ptr currentTrack() const;
    Tomahawk::query_ptr stopAfterTrack() const;

    /**
     * Get the current position in the media.
     *
     * As the user might seek forwards and backwards this only returns the
     * location in the media, not the actual time that the media was already
     * playing.
     * @return The current time in milliseconds.
     */
    qint64 currentTime() const;

    /**
     * Returns the total duration of the currently playing track.
     *
     * For some media this time might only be an estimate as metadate might
     * not have reported a duration and thus the duration is estimated. During
     * playback we might get a better estimate so that the return value may
     * differ between multiple calls.
     *
     * @return The total duration in milliseconds.
     */
    qint64 currentTrackTotalTime() const;

    void setDspCallback( std::function< void( int state, int frameNumber, float* samples, int nb_channels, int nb_samples ) > cb );

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
    void toggleMute();

    void play( const QUrl& url );
    void playItem( Tomahawk::playlistinterface_ptr playlist, const Tomahawk::result_ptr& result, const Tomahawk::query_ptr& fromQuery = Tomahawk::query_ptr());
    void playItem( Tomahawk::playlistinterface_ptr playlist, const Tomahawk::query_ptr& query);
    void playItem( const Tomahawk::artist_ptr& artist);
    void playItem( const Tomahawk::album_ptr& album);
    void playPlaylistInterface( const Tomahawk::playlistinterface_ptr& playlist );
    void setPlaylist( Tomahawk::playlistinterface_ptr playlist );
    void setQueue( const Tomahawk::playlistinterface_ptr& queue );

    void setStopAfterTrack( const Tomahawk::query_ptr& query );

    void setRepeatMode( Tomahawk::PlaylistModes::RepeatMode mode );
    void setShuffled( bool enabled );

signals:
    void initialized();

    void loading( const Tomahawk::result_ptr track );
    void started( const Tomahawk::result_ptr track );
    void finished( const Tomahawk::result_ptr track );
    void stopped();
    void paused();
    void resumed();

//    void audioDataReady( QMap< AudioEngine::AudioChannel, QVector<qint16> > data );

    void stopAfterTrackChanged();

    void seeked( qint64 ms );

    void shuffleModeChanged( bool enabled );
    void repeatModeChanged( Tomahawk::PlaylistModes::RepeatMode mode );
    void controlStateChanged();
    void stateChanged( AudioState newState, AudioState oldState );
    void volumeChanged( int volume /* in percent */ );
    void mutedChanged( bool muted );

    void timerMilliSeconds( qint64 msElapsed );
    void timerSeconds( unsigned int secondsElapsed );
    void timerPercentage( unsigned int percentage );
    void trackPosition( float position );

    void playlistChanged( Tomahawk::playlistinterface_ptr playlist );
    void currentTrackPlaylistChanged( Tomahawk::playlistinterface_ptr playlist );

    void error( AudioEngine::AudioErrorCode errorCode );

private slots:
    void loadTrack( const Tomahawk::result_ptr& result, bool preload ); //async!
    void gotStreamUrl( const QVariantMap& data );
    void gotRedirectedStreamUrl( const Tomahawk::result_ptr& result, NetworkReply* reply );


    void performLoadIODevice( const Tomahawk::result_ptr& result, const QString& url, bool preload ); //only call from loadTrack kthxbi
    void performLoadTrack( const Tomahawk::result_ptr result, const QString& url, QSharedPointer< QIODevice > io, bool preload ); //only call from loadTrack or performLoadIODevice kthxbi
    void loadPreviousTrack();
    void loadNextTrack(bool preload = false);

    void onVolumeChanged( qreal volume );
    void timerTriggered( qint64 time );
    void onPositionChanged( float new_position );

    void setCurrentTrack( const Tomahawk::result_ptr& result );
    void setPreloadTrack( const Tomahawk::result_ptr& result );
    void onNowPlayingInfoReady( const Tomahawk::InfoSystem::InfoType type );
    void onPlaylistNextTrackAvailable();

    void sendNowPlayingNotification( const Tomahawk::InfoSystem::InfoType type );
    void sendWaitingNotification() const;

private:
    void setState( AudioState state );
    void setCurrentTrackPlaylist( const Tomahawk::playlistinterface_ptr& playlist );

//    void audioDataArrived( QMap< AudioEngine::AudioChannel, QVector< qint16 > >& data );


    Q_DECLARE_PRIVATE( AudioEngine )
    AudioEnginePrivate* d_ptr;
};

#endif // AUDIOENGINE_H
