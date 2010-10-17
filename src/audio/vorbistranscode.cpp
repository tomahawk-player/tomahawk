/***************************************************************************
 *   Copyright (C) 2005 - 2006 by                                          *
 *      Christian Muehlhaeuser, Last.fm Ltd <chris@last.fm>                *
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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/

#include "vorbistranscode.h"


size_t
vorbis_read( void* data_ptr, size_t byteSize, size_t sizeToRead, void* data_src )
{
    VorbisTranscode* parent = (VorbisTranscode*)data_src;
    QMutexLocker locker( parent->mutex() );

    int r = byteSize * sizeToRead;
    if ( r > parent->buffer()->size() )
        r = parent->buffer()->size();

    memcpy( data_ptr, (char*)parent->buffer()->data(), r );
    parent->buffer()->remove( 0, r );

    return r;
}


int
vorbis_seek( void* data_src, ogg_int64_t offset, int origin )
{
    return -1;
}


int
vorbis_close( void* data_src )
{
    // done ;-)
    return 0;
}


long
vorbis_tell( void* data_src )
{
    return -1;
}


VorbisTranscode::VorbisTranscode()
    : m_vorbisInit( false )
{
    qDebug() << Q_FUNC_INFO;
}


VorbisTranscode::~VorbisTranscode()
{
    qDebug() << Q_FUNC_INFO;
}


void
VorbisTranscode::onSeek( int seconds )
{
    QMutexLocker locker( &m_mutex );

    m_buffer.clear();
    m_outBuffer.clear();
}


void
VorbisTranscode::clearBuffers()
{
    QMutexLocker locker( &m_mutex );

    m_vorbisInit = false;
    m_buffer.clear();
    m_outBuffer.clear();
}


void
VorbisTranscode::processData( const QByteArray& data, bool )
{
    m_mutex.lock();
    m_buffer.append( data );
    m_mutex.unlock();

    if ( !m_vorbisInit && m_buffer.size() >= OGG_BUFFER )
    {
        ov_callbacks oggCallbacks;

        oggCallbacks.read_func = vorbis_read;
        oggCallbacks.close_func = vorbis_close;
        oggCallbacks.seek_func = vorbis_seek;
        oggCallbacks.tell_func = vorbis_tell;

        ov_open_callbacks( this, &m_vorbisFile, 0, 0, oggCallbacks );
        m_vorbisInit = true;

        // Try to determine samplerate
        vorbis_info* vi = ov_info( &m_vorbisFile, -1 );
        qDebug() << "vorbisTranscode( Samplerate:" << vi->rate << "Channels:" << vi->channels << ")";

        emit streamInitialized( vi->rate, vi->channels );
    }

    long result = 1;
    int currentSection = 0;

    while ( m_buffer.size() >= OGG_BUFFER && result > 0 )
    {
        char tempBuffer[16384];
        result = ov_read( &m_vorbisFile, tempBuffer, sizeof( tempBuffer ), 0, 2, 1, &currentSection );

        if ( result > 0 )
        {
            for ( int i = 0; i < ( result / 2 ); i++ )
            {
                m_outBuffer.append( tempBuffer[i * 2] );
                m_outBuffer.append( tempBuffer[i * 2 + 1] );
            }
        }
    }
}
