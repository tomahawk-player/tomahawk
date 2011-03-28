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
