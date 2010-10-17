/***************************************************************************
 *   Copyright (C) 2007 by John Stamp, <jstamp@users.sourceforge.net>      *
 *   Copyright (C) 2007 by Max Howell, Last.fm Ltd.                        *
 *   Copyright (C) 2010 by Christian Muehlhaeuser <muesli@gmail.com>       *
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

#ifndef ALSA_AUDIO_H
#define ALSA_AUDIO_H

#include <QByteArray>
#include <QList>
#include <QString>

#include <alsa/asoundlib.h>
#include "xconvert.h"

struct AlsaDeviceInfo
{
    QString name;
    QString device;
};

struct snd_format
{
    unsigned int rate;
    unsigned int channels;
    snd_pcm_format_t format;
    AFormat xmms_format;
    int sample_bits;
    int bps;
};

static const struct 
{
    AFormat xmms;
    snd_pcm_format_t alsa;
}

format_table[] =   { { FMT_S16_LE, SND_PCM_FORMAT_S16_LE },
                     { FMT_S16_BE, SND_PCM_FORMAT_S16_BE },
                     { FMT_S16_NE, SND_PCM_FORMAT_S16    },
                     { FMT_U16_LE, SND_PCM_FORMAT_U16_LE },
                     { FMT_U16_BE, SND_PCM_FORMAT_U16_BE },
                     { FMT_U16_NE, SND_PCM_FORMAT_U16    },
                     { FMT_U8,     SND_PCM_FORMAT_U8     },
                     { FMT_S8,     SND_PCM_FORMAT_S8     }, };

class AlsaAudio 
{
public:
    AlsaAudio();
    ~AlsaAudio();

    int getCards();
    AlsaDeviceInfo getDeviceInfo( int device );

    bool alsaOpen( QString device, AFormat format, unsigned int rate, 
                   unsigned int channels, snd_pcm_uframes_t periodSize,
                   unsigned int periodCount, int minBufferCapacity );

    int startPlayback();
    void stopPlayback();

    void alsaWrite( const QByteArray& inputData );
    void alsaClose();

    void setVolume( float vol );
    void setPaused( bool enabled ) { paused = enabled; }

    unsigned int timeElapsed();

    int hasData();
    int get_thread_buffer_filled() const;
    int alsa_free() const;
    void clearBuffer();

private:
    QList<AlsaDeviceInfo> m_devices;

    // The following static variables are configured in either
    // alsaOpen or alsaSetup and used later in the audio thread
    static ssize_t hw_period_size_in;
    static snd_output_t *logs;
    static bool going;
    static snd_pcm_t *alsa_pcm;
    static snd_format* inputf;
    static snd_format* outputf;
    static float volume;
    static bool paused;
    static convert_func_t alsa_convert_func;
    static convert_channel_func_t alsa_stereo_convert_func;
    static convert_freq_func_t alsa_frequency_convert_func;
    static xmms_convert_buffers *convertb;
    static pthread_t audio_thread;
    static unsigned int pcmCounter;

    void getDevicesForCard( int card );

    static void* alsa_loop( void* );
    void run();
    void alsa_write_out_thread_data();
    void alsa_do_write( void* data, ssize_t length );
    void volume_adjust( void* data, ssize_t length, AFormat fmt );
    void alsa_write_audio( char *data, ssize_t length );
    //int get_thread_buffer_filled() const;

    static char* thread_buffer;
    static int thread_buffer_size;
    static int rd_index, wr_index;

    snd_pcm_sframes_t alsa_get_avail( void );
    int alsa_handle_error( int err );
    int xrun_recover();
    int suspend_recover();
    int format_from_alsa( snd_pcm_format_t fmt );
    snd_format* snd_format_from_xmms( AFormat fmt, unsigned int rate, unsigned int channels );

    void alsa_close_pcm( void );
};

#endif
