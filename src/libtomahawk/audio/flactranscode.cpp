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

#include "flactranscode.h"


FLACTranscode::FLACTranscode()
    : m_FLACRunning( false )
    , m_finished( false )
{
    qDebug() << Q_FUNC_INFO;

    init();
    set_metadata_respond_all();
}


FLACTranscode::~FLACTranscode()
{
    qDebug() << Q_FUNC_INFO;
}


void
FLACTranscode::onSeek( int seconds )
{
    QMutexLocker locker( &m_mutex );

    m_buffer.clear();
    m_outBuffer.clear();
}


void
FLACTranscode::clearBuffers()
{
    QMutexLocker locker( &m_mutex );

    m_FLACRunning = false;
    m_finished = false;

    m_buffer.clear();
    m_outBuffer.clear();

    flush();
    reset();
}


void
FLACTranscode::processData( const QByteArray& data, bool finish )
{
    m_mutex.lock();
    m_buffer.append( data );
    m_mutex.unlock();

    while ( m_buffer.size() >= FLAC_BUFFER )
    {
        process_single();
    }

    m_finished = finish;
}


::FLAC__StreamDecoderReadStatus
FLACTranscode::read_callback( FLAC__byte buffer[], size_t *bytes )
{
    QMutexLocker locker( &m_mutex );

    if ( *bytes > (unsigned int)m_buffer.size() )
        *bytes = m_buffer.size();

    memcpy( buffer, (char*)m_buffer.data(), *bytes );
    m_buffer.remove( 0, *bytes );

    return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
}


::FLAC__StreamDecoderWriteStatus
FLACTranscode::write_callback( const ::FLAC__Frame *frame, const FLAC__int32 *const buffer[] )
{
    union PCMDATA
    {
        FLAC__int32 i;
        unsigned char b[2];
    } pcmDataLeft, pcmDataRight;

    for ( unsigned int sample = 0; sample < frame->header.blocksize; sample++ )
    {
        pcmDataLeft.i  = buffer[0][sample];
        pcmDataRight.i = buffer[1][sample];

        m_outBuffer.append( pcmDataLeft.b[0] );
        m_outBuffer.append( pcmDataLeft.b[1] );
        m_outBuffer.append( pcmDataRight.b[0] );
        m_outBuffer.append( pcmDataRight.b[1] );
    }

    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}


::FLAC__StreamDecoderSeekStatus
FLACTranscode::seek_callback(FLAC__uint64 absolute_byte_offset)
{
    return FLAC__STREAM_DECODER_SEEK_STATUS_UNSUPPORTED;
}


void
FLACTranscode::metadata_callback( const ::FLAC__StreamMetadata *metadata )
{
    qDebug() << Q_FUNC_INFO << metadata->is_last;

    switch ( metadata->type )
    {
        case FLAC__METADATA_TYPE_STREAMINFO:
        {
            FLAC::Metadata::StreamInfo stream_info( (::FLAC__StreamMetadata *)metadata, true );

            // Try to determine samplerate
            qDebug() << "FLACTranscode( BitsPerSample:" << stream_info.get_bits_per_sample() << "Samplerate:" << stream_info.get_sample_rate() << "Channels:" << stream_info.get_channels() << ")";
            emit streamInitialized( stream_info.get_sample_rate(), stream_info.get_channels() );

            m_FLACRunning = true;
            break;
        }

        default:
            qDebug() << "Not handling type:" << metadata->type;
            break;
    }
}


void
FLACTranscode::error_callback( ::FLAC__StreamDecoderErrorStatus status )
{
    qDebug() << Q_FUNC_INFO << status;
}


bool
FLACTranscode::eof_callback()
{
    return ( m_buffer.isEmpty() && m_finished );
}
