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

#include "alsaaudio.h"
#include "alsaplayback.h"

#include <QDebug>
#include <QStringList>


AlsaPlayback::AlsaPlayback()
    : m_audio( 0 )
    , m_paused( false )
    , m_playing( false )
    , m_volume( 0.75 )
    , m_deviceNum( 0 )
{
    setBufferCapacity( 32768 * 4 ); //FIXME: const value
}


AlsaPlayback::~AlsaPlayback()
{
    delete m_audio;
}


bool
AlsaPlayback::haveData()
{
    return ( m_audio->hasData() > 0 );
}


bool
AlsaPlayback::needData()
{
    return ( m_audio->get_thread_buffer_filled() < m_bufferCapacity );
}


void
AlsaPlayback::setBufferCapacity( int size )
{
    m_bufferCapacity = size;
}


int
AlsaPlayback::bufferSize()
{
    return m_audio->get_thread_buffer_filled();
}


float
AlsaPlayback::volume()
{
    return m_volume;
}


void
AlsaPlayback::setVolume( int volume )
{
    m_volume = (float)volume / 100.0;
    m_audio->setVolume( m_volume );
}


void
AlsaPlayback::triggerTimers()
{
    if ( m_audio )
        emit timeElapsed( m_audio->timeElapsed() );
}


QStringList
AlsaPlayback::soundSystems()
{
    return QStringList() << "Alsa";
}


QStringList
AlsaPlayback::devices()
{
//    Q_DEBUG_BLOCK << "Querying audio devices";

    QStringList devices;
    for (int i = 0, n = m_audio->getCards(); i < n; i++)
        devices << m_audio->getDeviceInfo( i ).name;

    return devices;
}


bool
AlsaPlayback::startPlayback()
{
    if ( !m_audio )
    {
        goto _error;
    }

    if ( m_audio->startPlayback() )
    {
        goto _error;
    }

    m_playing = true;
    return true;

_error:
    return false;
}


void
AlsaPlayback::stopPlayback()
{
    m_audio->stopPlayback();
    m_paused = false;
    m_playing = false;
}


void
AlsaPlayback::initAudio( long sampleRate, int channels )
{
    int periodSize = 1024;  // According to mplayer, these two are good defaults.
    int periodCount = 16;   // They create a buffer size of 16384 frames.
    QString cardDevice;

    delete m_audio;
    m_audio = new AlsaAudio;
    m_audio->clearBuffer();

    cardDevice = internalSoundCardID( m_deviceNum );

    // We assume host byte order
#ifdef WORDS_BIGENDIAN
    if ( !m_audio->alsaOpen( cardDevice, FMT_S16_BE, sampleRate, channels, periodSize, periodCount, m_bufferCapacity ) )
#else
    if ( !m_audio->alsaOpen( cardDevice, FMT_S16_LE, sampleRate, channels, periodSize, periodCount, m_bufferCapacity ) )
#endif
    {
    }
}


void
AlsaPlayback::processData( const QByteArray &buffer )
{
    m_audio->alsaWrite( buffer );
}


void
AlsaPlayback::clearBuffers()
{
    m_audio->clearBuffer();
}


QString
AlsaPlayback::internalSoundCardID( int settingsID )
{
    int cards = m_audio->getCards();

    if ( settingsID < cards )
        return m_audio->getDeviceInfo( settingsID ).device;
    else
        return "default";
}


void
AlsaPlayback::pause()
{
    m_paused = true;

    if ( m_audio )
    {
        m_audio->setPaused( true );
    }
}


void
AlsaPlayback::resume()
{
    m_paused = false;

    if ( m_audio )
        m_audio->setPaused( false );
}
