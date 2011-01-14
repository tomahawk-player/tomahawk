#ifndef AUDIOENGINE_H
#define AUDIOENGINE_H

#include <QObject>

#include <Phonon/MediaObject>
#include <Phonon/AudioOutput>

#include "result.h"
#include "typedefs.h"

#include "transcodeinterface.h"

#define AUDIO_VOLUME_STEP 5

class PlaylistInterface;

class AudioEngine : public QObject
{
Q_OBJECT

public:
    enum AudioErrorCode { StreamReadError, AudioDeviceError, DecodeError };

    explicit AudioEngine();
    ~AudioEngine();

    unsigned int volume() const { return m_audioOutput->volume() * 100.0; } // in percent
    bool isPlaying() const { return m_mediaObject->state() == Phonon::PlayingState; }
    bool isPaused() const { return m_mediaObject->state() == Phonon::PausedState; }

    /* Returns the PlaylistInterface of the currently playing track. Note: This might be different to the current playlist! */
    PlaylistInterface* currentTrackPlaylist() const { return m_currentTrackPlaylist; }

    /* Returns the PlaylistInterface of the current playlist. Note: The currently playing track might still be from a different playlist! */
    PlaylistInterface* playlist() const { return m_playlist; }

public slots:
    void play();
    void pause();
    void stop();

    void previous();
    void next();

    void setVolume( int percentage );
    void lowerVolume() { setVolume( volume() - AUDIO_VOLUME_STEP ); }
    void raiseVolume() { setVolume( volume() + AUDIO_VOLUME_STEP ); }
    void onVolumeChanged( float volume ) { emit volumeChanged( volume * 100 ); }

    void playItem( PlaylistInterface* playlist, const Tomahawk::result_ptr& result );
    void setPlaylist( PlaylistInterface* playlist ) { m_playlist = playlist; }
    void setQueue( PlaylistInterface* queue ) { m_queue = queue; }

    void onTrackAboutToClose();

signals:
    void loading( const Tomahawk::result_ptr& track );
    void started( const Tomahawk::result_ptr& track );
    void finished( const Tomahawk::result_ptr& track );
    void stopped();
    void paused();
    void resumed();

    void volumeChanged( int volume /* in percent */ );

    void timerMilliSeconds( qint64 msElapsed );
    void timerSeconds( unsigned int secondsElapsed );
    void timerPercentage( unsigned int percentage );

    void error( AudioErrorCode errorCode );

private slots:
    bool loadTrack( const Tomahawk::result_ptr& result );
    void loadPreviousTrack();
    void loadNextTrack();

    void onStateChanged( Phonon::State newState, Phonon::State oldState );
    void timerTriggered( qint64 time );

    void setCurrentTrack( const Tomahawk::result_ptr& result );

private:
    QSharedPointer<QIODevice> m_input;

    Tomahawk::result_ptr m_currentTrack;
    Tomahawk::result_ptr m_lastTrack;
    PlaylistInterface* m_playlist;
    PlaylistInterface* m_currentTrackPlaylist;
    PlaylistInterface* m_queue;

    Phonon::MediaObject* m_mediaObject;
    Phonon::AudioOutput* m_audioOutput;

    unsigned int m_timeElapsed;
    bool m_expectStop;
};

#endif // AUDIOENGINE_H
