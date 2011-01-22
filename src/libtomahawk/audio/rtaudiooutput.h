#ifndef RTAUDIOPLAYBACK_H
#define RTAUDIOPLAYBACK_H

#include "RtAudio.h"

#include <QObject>
#include <QMutex>

class RTAudioOutput : public QObject
{
    Q_OBJECT

    public:
        RTAudioOutput();
        ~RTAudioOutput();

        void initAudio( long sampleRate, int channels );

        float volume() { return m_volume; }
        bool isPaused() { return m_paused; }
        virtual bool isPlaying() { return m_playing; }

        bool haveData() { return m_buffer.length() > 2048; }
        bool needData();
        void processData( const QByteArray &buffer );

        QStringList soundSystems();
        QStringList devices();
        int sourceChannels() { return m_sourceChannels; }

        QMutex* mutex() { return &m_mutex; }
        QByteArray* buffer() { return &m_buffer; }

        int m_pcmCounter;

    public slots:
        void clearBuffers();

        bool startPlayback();
        void stopPlayback();

        void pause() { m_paused = true; }
        void resume() { m_paused = false; }

        void setVolume( int volume ) { m_volume = ((float)(volume)) / (float)100.0; emit volumeChanged( m_volume ); }
        virtual void triggerTimers() { if ( m_bps > 0 ) emit timeElapsed( m_pcmCounter / m_bps ); else emit timeElapsed( 0 ); }

    signals:
        void bufferEmpty();
        void bufferFull();

        void volumeChanged( float volume );
        void timeElapsed( unsigned int seconds );

    private:
        RtAudio *m_audio;
        bool m_bufferEmpty;

        float m_volume;
        QByteArray m_buffer;
        QMutex m_mutex;

        int m_sourceChannels;
        bool m_paused;
        bool m_playing;
        int m_bps;
        
        int internalSoundCardID( int settingsID );
};

#endif
