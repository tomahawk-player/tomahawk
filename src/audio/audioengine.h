#ifndef AUDIOENGINE_H
#define AUDIOENGINE_H

#include <QThread>
#include <QMutex>
#include <QBuffer>

#include "result.h"
#include "typedefs.h"

#include "rtaudiooutput.h"
#include "alsaplayback.h"
#include "transcodeinterface.h"

#define AUDIO_VOLUME_STEP 5

class PlaylistInterface;

class AudioEngine : public QThread
{
Q_OBJECT

public:
    enum AudioErrorCode { StreamReadError, AudioDeviceError, DecodeError };

    explicit AudioEngine();
    ~AudioEngine();

    unsigned int volume() const { if ( m_audio ) return m_audio->volume() * 100.0; else return 0; }; // in percent
    bool isPaused() const { return m_audio->isPaused(); }

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

    void timerSeconds( unsigned int secondsElapsed );
    void timerPercentage( unsigned int percentage );

    void error( AudioErrorCode errorCode );

private slots:
    bool loadTrack( const Tomahawk::result_ptr& result );
    void loadPreviousTrack();
    void loadNextTrack();

    void setStreamData( long sampleRate, int channels );
    void timerTriggered( unsigned int seconds );

    void engineLoop();
    void loop();

private:
    void run();
    void clearBuffers();

    QSharedPointer<QIODevice> m_input;
    QSharedPointer<TranscodeInterface> m_transcode;

#ifdef Q_WS_X11
    AlsaPlayback* m_audio;
#else
    RTAudioOutput* m_audio;
#endif

    Tomahawk::result_ptr m_currentTrack;
    Tomahawk::result_ptr m_lastTrack;
    PlaylistInterface* m_playlist;
    PlaylistInterface* m_currentTrackPlaylist;
    PlaylistInterface* m_queue;
    QMutex m_mutex;

    int m_i;
};

#endif // AUDIOENGINE_H
