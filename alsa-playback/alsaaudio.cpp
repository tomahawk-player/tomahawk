/***************************************************************************
 *   Copyright (C) 2007 by John Stamp, <jstamp@users.sourceforge.net>      *
 *   Copyright (C) 2007 by Max Howell, Last.fm Ltd.                        *
 *   Copyright (C) 2010 by Christian Muehlhaeuser <muesli@gmail.com>       *
 *                                                                         *
 *   Large portions of this code are shamelessly copied from audio.c:      *
 *   The XMMS ALSA output plugin                                           *
 *   Copyright (C) 2001-2003 Matthieu Sozeau <mattam@altern.org>           *
 *   Copyright (C) 1998-2003  Peter Alm, Mikael Alm, Olle Hallnas,         *
 *                            Thomas Nilsson and 4Front Technologies       *
 *   Copyright (C) 1999-2007  Haavard Kvaalen                              *
 *   Copyright (C) 2005       Takashi Iwai                                 *
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
 *   59 Temple Place - Suite 330, Boston, MA  02110-1301, USA.             *
 ***************************************************************************/

#include "alsaaudio.h"

#include <qendian.h>
#include <QDebug>

//no debug
#define snd_pcm_hw_params_dump( hwparams, logs )
#define snd_pcm_sw_params_dump( x, y )
#define snd_pcm_dump( x, y )

pthread_t AlsaAudio::audio_thread;

char* AlsaAudio::thread_buffer = NULL;
int AlsaAudio::thread_buffer_size = 0;
int AlsaAudio::rd_index = 0;
int AlsaAudio::wr_index = 0;
unsigned int AlsaAudio::pcmCounter = 0;

snd_output_t* AlsaAudio::logs = NULL;
bool AlsaAudio::going = false;
snd_pcm_t *AlsaAudio::alsa_pcm = NULL;

ssize_t AlsaAudio::hw_period_size_in = 0;
snd_format* AlsaAudio::inputf = NULL;
snd_format* AlsaAudio::outputf = NULL;
float AlsaAudio::volume = 1.0;
bool AlsaAudio::paused = false;

convert_func_t AlsaAudio::alsa_convert_func = NULL;
convert_channel_func_t AlsaAudio::alsa_stereo_convert_func = NULL;
convert_freq_func_t AlsaAudio::alsa_frequency_convert_func = NULL;
xmms_convert_buffers* AlsaAudio::convertb = NULL;


AlsaAudio::AlsaAudio()
{
}


AlsaAudio::~AlsaAudio()
{
    // Close here just to be sure
    // These are safe to call more than once
    stopPlayback();
    alsaClose();
}


/******************************************************************************
 * Device Detection
 ******************************************************************************/

int
AlsaAudio::getCards( void )
{
    int card = -1;
    int err = 0;
    m_devices.clear();

    // First add the default PCM device
    AlsaDeviceInfo dev;
    dev.name = "Default PCM device (default)";
    dev.device = "default";
    m_devices.push_back( dev );

    if ( (err = snd_card_next( &card )) != 0 )
        goto getCardsFailed;

    while ( card > -1 )
    {
        getDevicesForCard( card );
        if ( (err = snd_card_next( &card )) != 0 )
            goto getCardsFailed;
    }

    return m_devices.size();

getCardsFailed:
    qDebug() << __PRETTY_FUNCTION__ << "failed:" << snd_strerror( -err );
    return -1;
}


void
AlsaAudio::getDevicesForCard( int card )
{
    int pcm_device = -1, err;
    snd_pcm_info_t *pcm_info;
    snd_ctl_t *ctl;
    char *alsa_name;
    QString cardName = "Unknown soundcard";
    QString device_name = QString( "hw:%1" ).arg( card );

    if ((err = snd_ctl_open( &ctl, device_name.toAscii(), 0 )) < 0) {
        qDebug() << "Failed:" << snd_strerror( -err );
        return;
    }

    if ((err = snd_card_get_name( card, &alsa_name )) != 0)
    {
        qDebug() << "Failed:" << snd_strerror( -err );
    }
    else
        cardName = alsa_name;

    snd_pcm_info_alloca( &pcm_info );

    for (;;)
    {
        if ((err = snd_ctl_pcm_next_device( ctl, &pcm_device )) < 0)
        {
            qDebug() << "Failed:" << snd_strerror( -err );
            pcm_device = -1;
        }
        if (pcm_device < 0)
            break;

        snd_pcm_info_set_device( pcm_info, pcm_device );
        snd_pcm_info_set_subdevice( pcm_info, 0 );
        snd_pcm_info_set_stream( pcm_info, SND_PCM_STREAM_PLAYBACK );

        if ((err = snd_ctl_pcm_info( ctl, pcm_info )) < 0)
        {
            if ( err != -ENOENT )
                qDebug() << "Failed: snd_ctl_pcm_info() failed"
                         "(" << card << ":" << pcm_device << "): "
                         << snd_strerror( -err );
            continue;
        }

        AlsaDeviceInfo dev;
        dev.device = QString( "hw:%1,%2" )
                .arg( card )
                .arg( pcm_device );
        dev.name = QString( "%1: %2 (%3)" )
                .arg( cardName )
                .arg( snd_pcm_info_get_name( pcm_info ) )
                .arg( dev.device );

        m_devices.push_back( dev );
    }

    snd_ctl_close( ctl );
}


AlsaDeviceInfo
AlsaAudio::getDeviceInfo( int device )
{
    return m_devices[device];
}


/******************************************************************************
    Device Setup
******************************************************************************/

bool
AlsaAudio::alsaOpen( QString device, AFormat format, unsigned int rate,
                     unsigned int channels, snd_pcm_uframes_t periodSize,
                     unsigned int periodCount, int minBufferCapacity )
{
    int err, hw_buffer_size;
    ssize_t hw_period_size;
    snd_pcm_hw_params_t *hwparams;
    snd_pcm_sw_params_t *swparams;
    snd_pcm_uframes_t alsa_buffer_size, alsa_period_size;

    inputf = snd_format_from_xmms( format, rate, channels );
    convertb = xmms_convert_buffers_new();
    snd_output_stdio_attach( &logs, stderr, 0 );

    alsa_convert_func = NULL;
    alsa_stereo_convert_func = NULL;
    alsa_frequency_convert_func = NULL;

    free( outputf );
    outputf = snd_format_from_xmms( inputf->xmms_format, inputf->rate, inputf->channels );

    qDebug() << "Opening device:" << device;

    // FIXME: Can snd_pcm_open() return EAGAIN?
    if ((err = snd_pcm_open( &alsa_pcm,
                             device.toAscii(),
                             SND_PCM_STREAM_PLAYBACK,
                             SND_PCM_NONBLOCK )) < 0)
    {
        qDebug() << "Failed to open pcm device (" << device << "):" << snd_strerror( -err );
        alsa_pcm = NULL;
        free( outputf );
        outputf = NULL;
        return false;
    }

    snd_pcm_info_t *info;
    int alsa_card, alsa_device, alsa_subdevice;

    snd_pcm_info_alloca( &info );
    snd_pcm_info( alsa_pcm, info );
    alsa_card = snd_pcm_info_get_card( info );
    alsa_device = snd_pcm_info_get_device( info );
    alsa_subdevice = snd_pcm_info_get_subdevice( info );

//    qDebug() << "Card:" << alsa_card;
//    qDebug() << "Device:" << alsa_device;
//    qDebug() << "Subdevice:" << alsa_subdevice;

    snd_pcm_hw_params_alloca( &hwparams );

    if ( (err = snd_pcm_hw_params_any( alsa_pcm, hwparams ) ) < 0 )
    {
        qDebug() << "No configuration available for playback:"
                 << snd_strerror( -err );
        alsaClose();
        return false;
    }

    if ( ( err = snd_pcm_hw_params_set_access( alsa_pcm, hwparams,
           SND_PCM_ACCESS_RW_INTERLEAVED ) ) < 0 )
    {
        qDebug() << "Cannot set normal write mode:" << snd_strerror( -err );
        alsaClose();
        return false;
    }

    if ( ( err = snd_pcm_hw_params_set_format( alsa_pcm, hwparams, outputf->format ) ) < 0 )
    {
        // Try if one of these format work (one of them should work
        // on almost all soundcards)

        snd_pcm_format_t formats[] = { SND_PCM_FORMAT_S16_LE,
                                       SND_PCM_FORMAT_S16_BE,
                                       SND_PCM_FORMAT_U8 };

        uint i;
        for ( i = 0; i < sizeof( formats ) / sizeof( formats[0] ); i++ )
        {
            if ( snd_pcm_hw_params_set_format( alsa_pcm, hwparams, formats[i] ) == 0 )
            {
                outputf->format = formats[i];
                break;
            }
        }
        if ( outputf->format != inputf->format )
        {
            outputf->xmms_format = (AFormat)format_from_alsa( outputf->format );

            qDebug() << "Converting format from" << inputf->xmms_format << "to" << outputf->xmms_format;

            if ( outputf->xmms_format < 0 )
                return -1;
            alsa_convert_func = xmms_convert_get_func( outputf->xmms_format, inputf->xmms_format );
            if ( alsa_convert_func == NULL )
            {
                qDebug() << "Format translation needed, but not available. Input:" << inputf->xmms_format << "; Output:" << outputf->xmms_format ;
                alsaClose();
                return false;
            }
        }
        else
        {
            qDebug() << "Sample format not available for playback:" << snd_strerror( -err );
            alsaClose();
            return false;
        }
    }

    snd_pcm_hw_params_set_channels_near( alsa_pcm, hwparams, &outputf->channels );
    if ( outputf->channels != inputf->channels )
    {
        qDebug() << "Converting channels from" << inputf->channels << "to" << outputf->channels;

        alsa_stereo_convert_func =
                xmms_convert_get_channel_func( outputf->xmms_format,
                                               outputf->channels,
                                               inputf->channels );
        if ( alsa_stereo_convert_func == NULL )
        {
            qDebug() << "No stereo conversion available. Format:" << outputf->xmms_format << "; Input Channels:" << inputf->channels << "; Output Channels:" << outputf->channels ;
            alsaClose();
            return false;
        }
    }

    snd_pcm_hw_params_set_rate_near( alsa_pcm, hwparams, &outputf->rate, 0 );
    if ( outputf->rate == 0 )
    {
        qDebug() << "No usable samplerate available.";
        alsaClose();
        return false;
    }
    if ( outputf->rate != inputf->rate )
    {
        qDebug() << "Converting samplerate from" << inputf->rate << "to" << outputf->rate ;
        if ( outputf->channels < 1 || outputf->channels > 2 )
        {
            qDebug() << "Unsupported number of channels:" << outputf->channels << "- Resample function not available" ;
            alsa_frequency_convert_func = NULL;
            alsaClose();
            return false;
        }
        alsa_frequency_convert_func =
                xmms_convert_get_frequency_func( outputf->xmms_format,
                                                 outputf->channels );
        if ( alsa_frequency_convert_func == NULL )
        {
            qDebug() << "Resample function not available. Format" << outputf->xmms_format ;
            alsaClose();
            return false;
        }
    }

    outputf->sample_bits = snd_pcm_format_physical_width( outputf->format );
    outputf->bps = ( outputf->rate * outputf->sample_bits * outputf->channels ) >> 3;

    if ( ( err = snd_pcm_hw_params_set_period_size_near( alsa_pcm, hwparams,
                                                         &periodSize, NULL ) ) < 0 )
    {
        qDebug() << "Set period size failed:" << snd_strerror( -err );
        alsaClose();
        return false;
    }

    if ( ( err = snd_pcm_hw_params_set_periods_near( alsa_pcm, hwparams,
         &periodCount, 0 ) ) < 0 )
    {
        qDebug() << "Set period count failed:" << snd_strerror( -err );
        alsaClose();
        return false;
    }

    if ( snd_pcm_hw_params( alsa_pcm, hwparams ) < 0 )
    {
        snd_pcm_hw_params_dump( hwparams, logs );
        qDebug() << "Unable to install hw params";
        alsaClose();
        return false;
    }

    if ( ( err = snd_pcm_hw_params_get_buffer_size( hwparams, &alsa_buffer_size ) ) < 0 )
    {
        qDebug() << "snd_pcm_hw_params_get_buffer_size() failed:" << snd_strerror( -err );
        alsaClose();
        return false;
    }

    if ( ( err = snd_pcm_hw_params_get_period_size( hwparams, &alsa_period_size, 0 ) ) < 0 )
    {
        qDebug() << "snd_pcm_hw_params_get_period_size() failed:" << snd_strerror( -err );
        alsaClose();
        return false;
    }
    snd_pcm_sw_params_alloca( &swparams );
    snd_pcm_sw_params_current( alsa_pcm, swparams );

    if ( ( err = snd_pcm_sw_params_set_start_threshold( alsa_pcm,
         swparams, alsa_buffer_size - alsa_period_size ) < 0 ) )
        qDebug() << "Setting start threshold failed:" << snd_strerror( -err );
    if ( snd_pcm_sw_params( alsa_pcm, swparams ) < 0 )
    {
        qDebug() << "Unable to install sw params";
        alsaClose();
        return false;
    }

  #ifndef QT_NO_DEBUG
    snd_pcm_sw_params_dump( swparams, logs );
    snd_pcm_dump( alsa_pcm, logs );
  #endif

    hw_period_size = snd_pcm_frames_to_bytes( alsa_pcm, alsa_period_size );
    if ( inputf->bps != outputf->bps )
    {
        int align = ( inputf->sample_bits * inputf->channels ) / 8;
        hw_period_size_in = ( (quint64)hw_period_size * inputf->bps +
                            outputf->bps/2 ) / outputf->bps;
        hw_period_size_in -= hw_period_size_in % align;
    }
    else
    {
        hw_period_size_in = hw_period_size;
    }

    hw_buffer_size = snd_pcm_frames_to_bytes( alsa_pcm, alsa_buffer_size );
    thread_buffer_size = minBufferCapacity * 4;
    if ( thread_buffer_size < hw_buffer_size )
        thread_buffer_size = hw_buffer_size * 2;
    if ( thread_buffer_size < 8192 )
        thread_buffer_size = 8192;
    thread_buffer_size += hw_buffer_size;
    thread_buffer_size -= thread_buffer_size % hw_period_size;

    thread_buffer = (char*)calloc(thread_buffer_size, sizeof(char));

//    qDebug() << "Device setup: period size:" << hw_period_size;
//    qDebug() << "Device setup: hw_period_size_in:" << hw_period_size_in;
//    qDebug() << "Device setup: hw_buffer_size:" << hw_buffer_size;
//    qDebug() << "Device setup: thread_buffer_size:" << thread_buffer_size;
//    qDebug() << "bits per sample:" <<  snd_pcm_format_physical_width( outputf->format )
//             << "frame size:" <<  snd_pcm_frames_to_bytes( alsa_pcm, 1 )
//             << "Bps:" << outputf->bps;

    return true;
}


int
AlsaAudio::startPlayback()
{
    int pthreadError = 0;

    // We should double check this here.  AlsaPlayback::initAudio
    // isn't having its emitted error caught.
    // So double check here to avoid a potential assert.
    if ( !alsa_pcm )
        return 1;

    going = true;

    //    qDebug() << "Starting thread";
    AlsaAudio* aaThread = new AlsaAudio();
    pthreadError = pthread_create( &audio_thread, NULL, &alsa_loop, (void*)aaThread );

    return pthreadError;
}


void
AlsaAudio::clearBuffer( void )
{
    wr_index = rd_index = pcmCounter = 0;
    if ( thread_buffer )
        memset( thread_buffer, 0, thread_buffer_size );
}


/******************************************************************************
    Play Interface
******************************************************************************/

void
AlsaAudio::alsaWrite( const QByteArray& input )
{
    int cnt;
    const char *src = input.data();
    int length = input.size();
    //qDebug() << "alsaWrite length:" << length;

    while ( length > 0 )
    {
        int wr;
        cnt = qMin(length, thread_buffer_size - wr_index);
        memcpy(thread_buffer + wr_index, src, cnt);
        wr = (wr_index + cnt) % thread_buffer_size;
        wr_index = wr;
        length -= cnt;
        src += cnt;
    }
}


int
AlsaAudio::get_thread_buffer_filled() const
{
    if ( wr_index >= rd_index )
    {
        return wr_index - rd_index;
    }
    return ( thread_buffer_size - ( rd_index - wr_index ) );
}


// HACK: the buffer may have data, but not enough to send to the card.  In that
// case we tell alsaplayback that we don't have any.  This may chop off some
// data, but only at the natural end of a track.  On my machine, this is at
// most 3759 bytes.  That's less than 0.022 sec.  It beats padding the buffer
// with 0's if the stream fails mid track.  No stutter this way.
int
AlsaAudio::hasData()
{
    int tempSize = get_thread_buffer_filled();
    if ( tempSize < hw_period_size_in )
        return 0;
    else
        return tempSize;
}


int
AlsaAudio::alsa_free() const
{
    //qDebug() << "alsa_free:" << thread_buffer_size - get_thread_buffer_filled() - 1;
    return thread_buffer_size - get_thread_buffer_filled() - 1;
}


void
AlsaAudio::setVolume ( float v )
{
    volume = v;
}


void
AlsaAudio::stopPlayback()
{
    if (going)
    {
//        Q_DEBUG_BLOCK;

        going = false;

        pthread_join( audio_thread, NULL );
    }
}


void
AlsaAudio::alsaClose()
{
//    Q_DEBUG_BLOCK;

    alsa_close_pcm();

    xmms_convert_buffers_destroy( convertb );
    convertb = NULL;

    if ( thread_buffer )
    {
        free(thread_buffer);
        thread_buffer = NULL;
    }
    if ( inputf )
    {
        free( inputf );
        inputf = NULL;
    }
    if (outputf )
    {
        free( outputf );
        outputf = NULL;
    }
    if ( logs )
    {
        snd_output_close( logs );
        logs = NULL;
    }
}


/******************************************************************************
    Play Thread
******************************************************************************/

void*
AlsaAudio::alsa_loop( void* pthis )
{
  AlsaAudio* aaThread = (AlsaAudio*)pthis;
  aaThread->run();
  return NULL;
}


void
AlsaAudio::run()
{
    int npfds = snd_pcm_poll_descriptors_count( alsa_pcm );
    int wr = 0;
    int err;

    if ( npfds <= 0 )
        goto _error;

    err = snd_pcm_prepare( alsa_pcm );
    if ( err < 0 )
        qDebug() << "snd_pcm_prepare error:" << snd_strerror( err );

    while ( going && alsa_pcm )
    {
        if ( !paused && get_thread_buffer_filled() >= hw_period_size_in )
        {
            wr = snd_pcm_wait( alsa_pcm, 10 );

            if ( wr > 0 )
            {
                alsa_write_out_thread_data();
            }
            else if ( wr < 0 )
            {
                alsa_handle_error( wr );
            }
        }
        else
        {
            struct timespec req;
            req.tv_sec = 0;
            req.tv_nsec = 10000000; //0.1 seconds
            nanosleep( &req, NULL );
        }
    }

 _error:
    err = snd_pcm_drop( alsa_pcm );
    if ( err < 0 )
        qDebug() << "snd_pcm_drop error:" << snd_strerror( err );
    wr_index = rd_index = 0;
    memset( thread_buffer, 0, thread_buffer_size );

//    qDebug() << "Exiting thread";

    pthread_exit( NULL );
}


/* transfer audio data from thread buffer to h/w */
void
AlsaAudio::alsa_write_out_thread_data( void )
{
    ssize_t length;
    int cnt;
    length = qMin( hw_period_size_in, ssize_t(get_thread_buffer_filled()) );
    length = qMin( length, snd_pcm_frames_to_bytes( alsa_pcm, alsa_get_avail() ) );

    while (length > 0)
    {
        int rd;
        cnt = qMin(int(length), thread_buffer_size - rd_index);
        alsa_do_write( thread_buffer + rd_index, cnt);
        rd = (rd_index + cnt) % thread_buffer_size;
        rd_index = rd;
        length -= cnt;
    }
}


/* update and get the available space on h/w buffer (in frames) */
snd_pcm_sframes_t
AlsaAudio::alsa_get_avail( void )
{
    snd_pcm_sframes_t ret;

    if ( alsa_pcm == NULL )
        return 0;

    while ( ( ret = snd_pcm_avail_update( alsa_pcm ) ) < 0 )
    {
        ret = alsa_handle_error( ret );
        if ( ret < 0 )
        {
            qDebug() << "alsa_get_avail(): snd_pcm_avail_update() failed:" << snd_strerror( -ret );
            return 0;
        }
    }
    return ret;
}


/* transfer data to audio h/w; length is given in bytes
 *
 * data can be modified via rate conversion or
 * software volume before passed to audio h/w
 */
void
AlsaAudio::alsa_do_write( void* data, ssize_t length )
{
    if ( alsa_convert_func != NULL )
        length = alsa_convert_func( convertb, &data, length );
    if ( alsa_stereo_convert_func != NULL )
        length = alsa_stereo_convert_func( convertb, &data, length );
    if ( alsa_frequency_convert_func != NULL )
    {
        length = alsa_frequency_convert_func( convertb, &data, length,
                                              inputf->rate,
                                              outputf->rate );
    }

    volume_adjust( data, length, outputf->xmms_format );

    alsa_write_audio( (char*)data, length );
}


#define VOLUME_ADJUST( type, endian )                                       \
do {                                                                        \
    type *ptr = (type*)data;                                                \
    for ( i = 0; i < length; i += 2 )                                       \
    {                                                                       \
        *ptr = qTo##endian( (type)( qFrom##endian( *ptr ) * volume  ) );    \
        ptr++;                                                              \
    }                                                                       \
} while ( 0 )

#define VOLUME_ADJUST8( type )           \
do {                                     \
    type *ptr = (type*)data;             \
    for ( i = 0; i < length; i++ )       \
    {                                    \
        *ptr = (type)( *ptr * volume );  \
        ptr++;                           \
    }                                    \
} while ( 0 )

void
AlsaAudio::volume_adjust( void* data, ssize_t length, AFormat fmt )
{
    ssize_t i;
    if ( volume == 1.0 )
        return;

    switch ( fmt )
    {
        case FMT_S16_LE:
            VOLUME_ADJUST( qint16, LittleEndian );
            break;
        case FMT_U16_LE:
            VOLUME_ADJUST( quint16, LittleEndian );
            break;
        case FMT_S16_BE:
            VOLUME_ADJUST( qint16, BigEndian );
            break;
        case FMT_U16_BE:
            VOLUME_ADJUST( quint16, BigEndian );
            break;
        case FMT_S8:
            VOLUME_ADJUST8( qint8 );
            break;
        case FMT_U8:
            VOLUME_ADJUST8( quint8 );
            break;
        default:
            qDebug() << __PRETTY_FUNCTION__ << "unhandled format:" << fmt ;
            break;
    }
}


/* transfer data to audio h/w via normal write */
void
AlsaAudio::alsa_write_audio( char *data, ssize_t length )
{
    snd_pcm_sframes_t written_frames;

    while ( length > 0 )
    {
        snd_pcm_sframes_t frames = snd_pcm_bytes_to_frames( alsa_pcm, length );
        written_frames = snd_pcm_writei( alsa_pcm, data, frames );

        if ( written_frames > 0 )
        {
            ssize_t written = snd_pcm_frames_to_bytes( alsa_pcm, written_frames );
            pcmCounter += written;

            length -= written;
            data += written;
        }
        else
        {
            int err = alsa_handle_error( (int)written_frames );
            if ( err < 0 )
            {
                qDebug() << __PRETTY_FUNCTION__ << "write error:" << snd_strerror( -err );
                break;
            }
        }
    }
}


/* handle generic errors */
int
AlsaAudio::alsa_handle_error( int err )
{
    switch ( err )
    {
        case -EPIPE:
            return xrun_recover();
        case -ESTRPIPE:
            return suspend_recover();
    }

    return err;
}


/* close PCM and release associated resources */
void
AlsaAudio::alsa_close_pcm( void )
{
    if ( alsa_pcm )
    {
        int err;
        snd_pcm_drop( alsa_pcm );
        if ( ( err = snd_pcm_close( alsa_pcm ) ) < 0 )
            qDebug() << "alsa_close_pcm() failed:" << snd_strerror( -err );
        alsa_pcm = NULL;
    }
}


int
AlsaAudio::format_from_alsa( snd_pcm_format_t fmt )
{
    uint i;
    for ( i = 0; i < sizeof( format_table ) / sizeof( format_table[0] ); i++ )
        if ( format_table[i].alsa == fmt )
            return format_table[i].xmms;
    qDebug() << "Unsupported format:" << snd_pcm_format_name( fmt );
    return -1;
}


struct snd_format*
AlsaAudio::snd_format_from_xmms( AFormat fmt, unsigned int rate, unsigned int channels )
{
    struct snd_format *f = (struct snd_format*)malloc( sizeof( struct snd_format ) );
    uint i;

    f->xmms_format = fmt;
    f->format = SND_PCM_FORMAT_UNKNOWN;

    for ( i = 0; i < sizeof( format_table ) / sizeof( format_table[0] ); i++ )
    {
        if ( format_table[i].xmms == fmt )
        {
            f->format = format_table[i].alsa;
            break;
        }
    }

    /* Get rid of _NE */
    for ( i = 0; i < sizeof( format_table ) / sizeof( format_table[0] ); i++ )
    {
        if ( format_table[i].alsa == f->format )
        {
            f->xmms_format = format_table[i].xmms;
            break;
        }
    }

    f->rate = rate;
    f->channels = channels;
    f->sample_bits = snd_pcm_format_physical_width( f->format );
    f->bps = ( rate * f->sample_bits * channels ) >> 3;

    return f;
}


int
AlsaAudio::xrun_recover( void )
{
#ifndef QT_NO_DEBUG
    snd_pcm_status_t *alsa_status;
    snd_pcm_status_alloca( &alsa_status );
    if ( snd_pcm_status( alsa_pcm, alsa_status ) < 0 )
    {
        qDebug() << "AlsaAudio::xrun_recover(): snd_pcm_status() failed";
    }
    else
    {
        snd_pcm_status_dump( alsa_status, logs );
        qDebug() << "Status:\n" << logs;
    }
#endif

    return snd_pcm_prepare( alsa_pcm );
}


int
AlsaAudio::suspend_recover( void )
{
    int err;

    while ( ( err = snd_pcm_resume( alsa_pcm ) ) == -EAGAIN )
        /* wait until suspend flag is released */
        sleep( 1 );
    if ( err < 0 )
    {
        qDebug() << "alsa_handle_error(): snd_pcm_resume() failed." ;
        return snd_pcm_prepare( alsa_pcm );
    }
    return err;
}


unsigned int
AlsaAudio::timeElapsed()
{
    return pcmCounter / outputf->bps;
}
