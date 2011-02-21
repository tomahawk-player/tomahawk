/***************************************************************************
 *   Copyright (C) 2005 - 2010 by                                          *
 *      Christian Muehlhaeuser <muesli@gmail.com>                          *
 *      Erik Jaelevik, Last.fm Ltd <erik@last.fm>                          *
 *      Max Howell, Last.fm Ltd <max@last.fm>                              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#ifndef ALSAPLAYBACK_H
#define ALSAPLAYBACK_H

#include <QObject>

class AlsaPlayback : public QObject
{
    Q_OBJECT

    public:
        AlsaPlayback();
        ~AlsaPlayback();

        virtual void initAudio( long sampleRate, int channels );

        virtual float volume();
        virtual bool isPaused() { return m_paused; }
        virtual bool isPlaying() { return m_playing; }

        virtual bool haveData();
        virtual bool needData();
        virtual void processData( const QByteArray& );

        virtual void setBufferCapacity( int size );
        virtual int bufferSize();
        
        virtual QStringList soundSystems();
        virtual QStringList devices();

    public slots:
        virtual void clearBuffers();

        virtual bool startPlayback();
        virtual void stopPlayback();

        virtual void pause();
        virtual void resume();

        virtual void setVolume( int volume );

        virtual void triggerTimers();

    signals:
        void timeElapsed( unsigned int seconds );

    private:
        class AlsaAudio *m_audio;
        int m_bufferCapacity;

        bool m_paused;
        bool m_playing;
        float m_volume;
        int m_deviceNum;

        QString internalSoundCardID( int settingsID );
};

#endif
