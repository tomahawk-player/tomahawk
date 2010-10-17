/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Christian Muehlhaeuser, Last.fm Ltd <chris@last.fm>                *
 *      Erik Jaelevik, Last.fm Ltd <erik@last.fm>                          *
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

#include "madtranscode.h"

#include <QDebug>

typedef struct audio_dither
{
    mad_fixed_t error[3];
    mad_fixed_t random;
} audio_dither;


/* fast 32-bit pseudo-random number generator */
/* code from madplay */
static inline unsigned long prng( unsigned long state )
{
    return (state * 0x0019660dL + 0x3c6ef35fL) & 0xffffffffL;
}


/* dithers 24-bit output to 16 bits instead of simple rounding */
/* code from madplay */
static inline signed int dither( mad_fixed_t sample, audio_dither *dither )
{
    unsigned int scalebits;
    mad_fixed_t output, mask, random;

    enum
    {
        MIN = -MAD_F_ONE,
        MAX =  MAD_F_ONE - 1
    };

    /* noise shape */
    sample += dither->error[0] - dither->error[1] + dither->error[2];

    dither->error[2] = dither->error[1];
    dither->error[1] = dither->error[0] / 2;

    /* bias */
    output = sample + (1L << (MAD_F_FRACBITS + 1 - 16 - 1));

    scalebits = MAD_F_FRACBITS + 1 - 16;
    mask = (1L << scalebits) - 1;

    /* dither */
    random  = prng(dither->random);
    output += (random & mask) - (dither->random & mask);

    dither->random = random;

    /* clip */
    /* TODO: better clipping function */
    if (sample >= MAD_F_ONE)
        sample = MAD_F_ONE - 1;
    else if (sample < -MAD_F_ONE)
        sample = -MAD_F_ONE;
    if (output >= MAD_F_ONE)
        output = MAD_F_ONE - 1;
    else if (output < -MAD_F_ONE)
        output = -MAD_F_ONE;

    /* quantize */
    output &= ~mask;

    /* error feedback */
    dither->error[0] = sample - output;

    /* scale */
    return output >> scalebits;
}


MADTranscode::MADTranscode() :
        m_decodedBufferCapacity( 32 * 1024 ),
        m_mpegInitialised( false )
{
    qDebug() << "Initialising MAD Transcoding";

    mad_stream_init( &stream );
    mad_frame_init( &frame );
    mad_synth_init( &synth );
    timer = mad_timer_zero;
    last_timer = mad_timer_zero;
}


MADTranscode::~MADTranscode()
{
    qDebug() << Q_FUNC_INFO;

    mad_synth_finish( &synth );
    mad_frame_finish( &frame );
    mad_stream_finish( &stream );
}


void
MADTranscode::processData( const QByteArray &buffer, bool finish )
{
    static audio_dither left_dither, right_dither;

    int err = 0;
    m_encodedBuffer.append( buffer );

    while ( err == 0 && ( m_encodedBuffer.count() >= MP3_BUFFER || finish ) )
    {
        mad_stream_buffer( &stream, (const unsigned char*)m_encodedBuffer.data(), m_encodedBuffer.count() );
        err = mad_frame_decode( &frame, &stream );

        if ( stream.next_frame != 0 )
        {
            size_t r = stream.next_frame - stream.buffer;
            m_encodedBuffer.remove( 0, r );
        }

        if ( err )
        {
//             if ( stream.error != MAD_ERROR_LOSTSYNC )
//                 qDebug() << "libmad error:" << mad_stream_errorstr( &stream );

            if ( !MAD_RECOVERABLE( stream.error ) )
                return;

            err = 0;
        }
        else
        {
            mad_timer_add( &timer, frame.header.duration );
            mad_synth_frame( &synth, &frame );

            if ( !m_mpegInitialised )
            {
                long sampleRate = synth.pcm.samplerate;
                int channels = synth.pcm.channels;

                qDebug() << "madTranscode( Samplerate:" << sampleRate << "- Channels:" << channels << ")";

                m_mpegInitialised = true;
                emit streamInitialized( sampleRate, channels > 0 ? channels : 2 );
            }

            for ( int i = 0; i < synth.pcm.length; i++ )
            {
                union PCMDATA
                {
                    short i;
                    unsigned char b[2];
                } pcmData;

                pcmData.i = dither( synth.pcm.samples[0][i], &left_dither );
                m_decodedBuffer.append( pcmData.b[0] );
                m_decodedBuffer.append( pcmData.b[1] );

                if ( synth.pcm.channels == 2 )
                {
                    pcmData.i = dither( synth.pcm.samples[1][i], &right_dither );
                    m_decodedBuffer.append( pcmData.b[0] );
                    m_decodedBuffer.append( pcmData.b[1] );
                }
            }

            if ( timer.seconds != last_timer.seconds )
                emit timeChanged( timer.seconds );

            last_timer = timer;
        }
    }
}


void
MADTranscode::onSeek( int seconds )
{
    mad_timer_t t;
    t.seconds = seconds;
    t.fraction = 0;

    timer = mad_timer_zero;
    mad_timer_add( &timer, t );

    m_encodedBuffer.clear();
    m_decodedBuffer.clear();
}


void
MADTranscode::clearBuffers()
{
    mad_synth_finish( &synth );
    mad_frame_finish( &frame );
    mad_stream_finish( &stream );

    m_mpegInitialised = false;
    timer = mad_timer_zero;
    last_timer = mad_timer_zero;

    m_encodedBuffer.clear();
    m_decodedBuffer.clear();

    mad_stream_init( &stream );
    mad_frame_init( &frame );
    mad_synth_init( &synth );
}

