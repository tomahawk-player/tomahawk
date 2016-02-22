/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2014, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#ifndef AUDIOOUTPUT_H
#define AUDIOOUTPUT_H

#include "DllMacro.h"
#include "Typedefs.h"

#include <QFile>

#include <functional>

struct libvlc_instance_t;
struct libvlc_media_player_t;
struct libvlc_media_t;
struct libvlc_event_t;

class MediaStream;

class DLLEXPORT AudioOutput : public QObject
{
Q_OBJECT

public:
    enum AudioState { Stopped = 0, Playing = 1, Paused = 2, Error = 3, Loading = 4, Buffering = 5 };

    explicit AudioOutput( QObject* parent = nullptr );
    ~AudioOutput();

    bool isInitialized() const;
    AudioState state() const;

    void setCurrentSource( const QUrl& stream );
    void setCurrentSource( QIODevice* stream );
    void setCurrentSource( MediaStream* stream );

    void play();
    void pause();
    void stop();
    void seek( qint64 milliseconds );

    bool isSeekable() const;
    bool isMuted() const;
    void setMuted( bool m );
    void setVolume( qreal vol );
    qreal volume() const;
    qint64 currentTime() const;
    qint64 totalTime() const;
    void setAutoDelete ( bool ad );

    void setDspCallback( std::function< void( int, int, float*, int, int ) > cb );

    static AudioOutput* instance();
    libvlc_instance_t* vlcInstance() const;

public slots:

signals:
    void initialized();
    void stateChanged( AudioOutput::AudioState, AudioOutput::AudioState );
    void tick( qint64 );
    void positionChanged( float );
    void volumeChanged( qreal volume );
    void mutedChanged( bool );

private:
    void onInitVlcEvent( const libvlc_event_t* event );

    void setState( AudioState state );
    void setCurrentTime( qint64 time );
    void setCurrentPosition( float position );
    void setTotalTime( qint64 time );

    void onVlcEvent( const libvlc_event_t* event );
    static void vlcEventCallback( const libvlc_event_t* event, void* opaque );
    static void s_dspCallback( int frameNumber, float* samples, int nb_channels, int nb_samples );

    static AudioOutput* s_instance;
    AudioState m_currentState;
    MediaStream* m_currentStream;
    bool m_seekable;
    bool m_muted;
    bool m_autoDelete;
    bool m_havePosition;
    bool m_haveTiming;
    qreal m_volume;
    qint64 m_currentTime;
    qint64 m_totalTime;
    bool m_justSeeked;

    bool m_initialized;
    QFile m_silenceFile;

    std::function< void( int state, int frameNumber, float* samples, int nb_channels, int nb_samples ) > dspPluginCallback;

    libvlc_instance_t* m_vlcInstance;
    libvlc_media_player_t* m_vlcPlayer;
    libvlc_media_t* m_vlcMedia;
};

#endif // AUDIOOUTPUT_H
