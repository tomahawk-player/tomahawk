#include <QMutexLocker>
#include <QStringList>
#include <QMessageBox>
#include <QDebug>

#include "rtaudiooutput.h"

#define BUFFER_SIZE 512

int
audioCallback( void *outputBuffer, void *inputBuffer, unsigned int bufferSize, double streamTime, RtAudioStreamStatus status, void* data_src )
{
    RTAudioOutput* parent = (RTAudioOutput*)data_src;
    QMutexLocker locker( parent->mutex() );

    char* buffer = (char*)outputBuffer;

    if ( !buffer || bufferSize != BUFFER_SIZE )
        return 0;

    int bufs = bufferSize * 2 * parent->sourceChannels();
    memset( buffer, 0, bufs );

    if ( parent->buffer()->size() >= bufs && !parent->isPaused() )
    {
        // Apply volume scaling
        for ( int i = 0; i < bufs / 2; i++ )
        {
            union PCMDATA
            {
                short i;
                unsigned char b[2];
            } pcmData;

            pcmData.b[0] = parent->buffer()->at( i * 2 );
            pcmData.b[1] = parent->buffer()->at( i * 2 + 1 );

            float pcmValue = (float)pcmData.i * parent->volume();
            pcmData.i = (short)pcmValue;

            buffer[i * 2] = pcmData.b[0];
            buffer[i * 2 + 1] = pcmData.b[1];
        }

        parent->m_pcmCounter += bufs;
        parent->buffer()->remove( 0, bufs );
    }

    return 0;
}


RTAudioOutput::RTAudioOutput() :
    m_pcmCounter( 0 ),
    m_audio( new RtAudio() ),
    m_bufferEmpty( true ),
    m_volume( 0.75 ),
    m_paused( false ),
    m_playing( false ),
    m_bps( -1 )
{
    qDebug() << Q_FUNC_INFO << m_audio->getCurrentApi();
    devices();
}


RTAudioOutput::~RTAudioOutput()
{
    qDebug() << Q_FUNC_INFO;
}


QStringList
RTAudioOutput::soundSystems()
{
    QStringList l;

    #ifdef WIN32
        l << "DirectSound";
    #endif

    #ifdef Q_WS_X11
        l << "Alsa";
    #endif

    #ifdef Q_WS_MAC
        l << "CoreAudio";
    #endif

    return l;
}


QStringList
RTAudioOutput::devices()
{
    qDebug() << Q_FUNC_INFO;
    QStringList l;

    try
    {
        qDebug() << "Device nums:" << m_audio->getDeviceCount();

        for ( unsigned int i = 0; i < m_audio->getDeviceCount(); i++ )
        {
            RtAudio::DeviceInfo info;
            info = m_audio->getDeviceInfo( i );
            qDebug() << "Device found:" << i << QString::fromStdString( info.name ) << info.outputChannels << info.duplexChannels << info.isDefaultOutput;

            if ( info.outputChannels > 0 )
                l << QString::fromStdString( info.name ); // FIXME make it utf8 compatible
        }
    }
    catch ( RtError &error )
    {
    }

    return l;
}


bool
RTAudioOutput::startPlayback()
{
    qDebug () << Q_FUNC_INFO;

    if ( m_audio->isStreamOpen() )
    {
        m_audio->startStream();
        m_playing = true;
    }

    return m_audio->isStreamOpen();
}


void
RTAudioOutput::stopPlayback()
{
    qDebug() << Q_FUNC_INFO;
    QMutexLocker locker( &m_mutex );

    delete m_audio; // FIXME
    m_audio = new RtAudio();
    m_buffer.clear();
    m_paused = false;
    m_playing = false;
    m_bps = -1;
    m_pcmCounter = 0;
}


void
RTAudioOutput::initAudio( long sampleRate, int channels )
{
    qDebug() << Q_FUNC_INFO << sampleRate << channels;
    QMutexLocker locker( &m_mutex );
    try
    {
        delete m_audio;
        m_audio = new RtAudio();
        m_bps = sampleRate * channels * 2;
        m_pcmCounter = 0;

        RtAudio::StreamParameters parameters;
        parameters.deviceId = m_audio->getDefaultOutputDevice();
        parameters.nChannels = channels;
        parameters.firstChannel = 0;
        unsigned int bufferFrames = BUFFER_SIZE;

        RtAudio::StreamOptions options;
        options.numberOfBuffers = 32;
        //options.flags = RTAUDIO_SCHEDULE_REALTIME;

        m_sourceChannels = channels;
        m_buffer.clear();

/*        if ( m_audio->isStreamRunning() )
            m_audio->abortStream();

        if ( m_audio->isStreamOpen() )
            m_audio->closeStream();*/

        m_audio->openStream( &parameters, NULL, RTAUDIO_SINT16, sampleRate, &bufferFrames, &audioCallback, this, &options );
    }
    catch ( RtError &error )
    {
         qDebug() << "Starting stream failed. RtAudio error type: " << error.getType();
    }
}


bool
RTAudioOutput::needData()
{
    if ( m_buffer.isEmpty() && !m_bufferEmpty )
    {
        m_bufferEmpty = true;
        emit bufferEmpty();
    }

    return ( m_buffer.size() < 65535 ); // FIXME constant value
}


void
RTAudioOutput::processData( const QByteArray &buffer )
{
    QMutexLocker locker( &m_mutex );

    m_buffer.append( buffer );
    if ( m_bufferEmpty && !buffer.isEmpty() )
    {
        m_bufferEmpty = false;
        emit bufferFull();
    }
}


void
RTAudioOutput::clearBuffers()
{
    qDebug() << Q_FUNC_INFO;
    QMutexLocker locker( &m_mutex );

    m_buffer.clear();
    m_bufferEmpty = true;
    emit bufferEmpty();
}


int
RTAudioOutput::internalSoundCardID( int settingsID )
{
    if ( settingsID < 0 )
        settingsID = 0;

    try
    {
        int card = 0;

        for ( unsigned int i = 1; i <= m_audio->getDeviceCount(); i++ )
        {
            RtAudio::DeviceInfo info;
            info = m_audio->getDeviceInfo( i );
            if ( info.outputChannels > 0 )
            {
                if ( card++ == settingsID )
                    return i;
            }
        }
    }
    catch ( RtError &error )
    {
    }

    #ifdef Q_WS_MAC
        return 3; // FIXME?
    #endif
    return 1;
}

