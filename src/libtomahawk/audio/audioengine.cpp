#include "audioengine.h"

#include <QUrl>
#include <QMutexLocker>

#include "playlistinterface.h"

#include "database/database.h"
#include "database/databasecommand_logplayback.h"
#include "network/servent.h"

#include "madtranscode.h"
#ifndef NO_OGG
#include "vorbistranscode.h"
#endif
#ifndef NO_FLAC
#include "flactranscode.h"
#endif

AudioEngine* AudioEngine::s_instance = 0;


AudioEngine*
AudioEngine::instance()
{
    return s_instance;
}


AudioEngine::AudioEngine()
    : QThread()
    , m_playlist( 0 )
    , m_currentTrackPlaylist( 0 )
    , m_queue( 0 )
    , m_timeElapsed( 0 )
    , m_i( 0 )
{
    s_instance = this;
    qDebug() << "Init AudioEngine";

    moveToThread( this );
    qRegisterMetaType< AudioErrorCode >("AudioErrorCode");

#ifdef Q_WS_X11
    m_audio = new AlsaPlayback();
#else
    m_audio = new RTAudioOutput();
#endif
    connect( m_audio, SIGNAL( timeElapsed( unsigned int ) ), SLOT( timerTriggered( unsigned int ) ), Qt::DirectConnection );

    start();
}


AudioEngine::~AudioEngine()
{
    qDebug() << Q_FUNC_INFO << "waiting for event loop to finish...";
    quit();
    wait( 1000 );

    m_input.clear();
    delete m_audio;
}


void
AudioEngine::playPause()
{
    if( m_audio->isPlaying() )
        pause();
    else
        play();

}


void
AudioEngine::play()
{
    qDebug() << Q_FUNC_INFO;

    if ( m_audio->isPaused() )
    {
        QMutexLocker lock( &m_mutex );
        m_audio->resume();
        emit resumed();
    }
    else
        loadNextTrack();
}


void
AudioEngine::pause()
{
    qDebug() << Q_FUNC_INFO;
    QMutexLocker lock( &m_mutex );

    m_audio->pause();
    emit paused();
}


void
AudioEngine::stop()
{
    qDebug() << Q_FUNC_INFO;
    QMutexLocker lock( &m_mutex );

    if ( !m_input.isNull() )
    {
        m_input->close();
        m_input.clear();
    }

    if ( !m_transcode.isNull() )
        m_transcode->clearBuffers();

    m_audio->stopPlayback();

    setCurrentTrack( Tomahawk::result_ptr() );
    emit stopped();
}


void
AudioEngine::previous()
{
    qDebug() << Q_FUNC_INFO;
    clearBuffers();
    loadPreviousTrack();
}


void
AudioEngine::next()
{
    qDebug() << Q_FUNC_INFO;
    clearBuffers();
    loadNextTrack();
}


void
AudioEngine::setVolume( int percentage )
{
    //qDebug() << Q_FUNC_INFO;

    percentage = qBound( 0, percentage, 100 );

    m_audio->setVolume( percentage );
    emit volumeChanged( percentage );
}


void
AudioEngine::mute()
{
    setVolume( 0 );
}


void
AudioEngine::onTrackAboutToClose()
{
    qDebug() << Q_FUNC_INFO;
    // the only way the iodev we are reading from closes itself, is if
    // there was a failure, usually network went away.
    // but we might as well play the remaining data we received
    // stop();
}


bool
AudioEngine::loadTrack( const Tomahawk::result_ptr& result )
{
    qDebug() << Q_FUNC_INFO << thread() << result;
    bool err = false;

    // in a separate scope due to the QMutexLocker!
    {
        QMutexLocker lock( &m_mutex );
        QSharedPointer<QIODevice> io;

        if ( result.isNull() )
            err = true;
        else
        {
            setCurrentTrack( result );
            io = Servent::instance()->getIODeviceForUrl( m_currentTrack );
            if ( m_currentTrack->url().startsWith( "http://" ) )
            {
                m_readReady = false;
                connect( io.data(), SIGNAL( downloadProgress( qint64, qint64 ) ), SLOT( onDownloadProgress( qint64, qint64 ) ) );
            }
            else
                m_readReady = true;

            if ( !io || io.isNull() )
            {
                qDebug() << "Error getting iodevice for item";
                err = true;
            }
            else
                connect( io.data(), SIGNAL( aboutToClose() ), SLOT( onTrackAboutToClose() ), Qt::DirectConnection );
        }

        if ( !err )
        {
            qDebug() << "Starting new song from url:" << m_currentTrack->url();
            emit loading( m_currentTrack );

            if ( !m_input.isNull() )
            {
                m_input->close();
                m_input.clear();
            }

            if ( m_lastTrack.isNull() || ( m_currentTrack->mimetype() != m_lastTrack->mimetype() ) )
            {
                if ( !m_transcode.isNull() )
                {
                    m_transcode.clear();
                }

                if ( m_currentTrack->mimetype() == "audio/mpeg" )
                {
                    m_transcode = QSharedPointer<TranscodeInterface>(new MADTranscode());
                }
#ifndef NO_OGG
                else if ( m_currentTrack->mimetype() == "application/ogg" )
                {
                    m_transcode = QSharedPointer<TranscodeInterface>(new VorbisTranscode());
                }
#endif
#ifndef NO_FLAC
                else if ( m_currentTrack->mimetype() == "audio/flac" )
                {
                    m_transcode = QSharedPointer<TranscodeInterface>(new FLACTranscode());
                }
#endif
                else
                    qDebug() << "Could NOT find suitable transcoder! Stopping audio.";

                if ( !m_transcode.isNull() )
                    connect( m_transcode.data(), SIGNAL( streamInitialized( long, int ) ), SLOT( setStreamData( long, int ) ), Qt::DirectConnection );
            }

            if ( !m_transcode.isNull() )
            {
                m_transcode->clearBuffers();
                m_input = io;

                if ( m_audio->isPaused() )
                    m_audio->resume();
            }
        }
    }

    if ( err )
    {
        stop();
        return false;
    }

    // needs to be out of the mutexlocker scope
    if ( m_transcode.isNull() )
    {
        stop();
        emit error( AudioEngine::DecodeError );
    }

    return !m_transcode.isNull();
}


void
AudioEngine::loadPreviousTrack()
{
    qDebug() << Q_FUNC_INFO;

    if ( !m_playlist )
    {
        stop();
        return;
    }

    Tomahawk::result_ptr result = m_playlist->previousItem();
    if ( !result.isNull() )
        loadTrack( result );
    else
        stop();
}


void
AudioEngine::loadNextTrack()
{
    qDebug() << Q_FUNC_INFO;

    Tomahawk::result_ptr result;

    if ( m_queue && m_queue->trackCount() )
    {
        result = m_queue->nextItem();
    }

    if ( m_playlist && result.isNull() )
    {
        result = m_playlist->nextItem();
    }

    if ( !result.isNull() )
        loadTrack( result );
    else
        stop();
}


void
AudioEngine::playItem( PlaylistInterface* playlist, const Tomahawk::result_ptr& result )
{
    qDebug() << Q_FUNC_INFO;

    clearBuffers();

    setPlaylist( playlist );
    m_currentTrackPlaylist = playlist;

    loadTrack( result );
}


void
AudioEngine::setStreamData( long sampleRate, int channels )
{
    qDebug() << Q_FUNC_INFO << sampleRate << channels << thread();

    if ( sampleRate < 44100 )
        sampleRate = 44100;

    m_audio->initAudio( sampleRate, channels );
    if ( m_audio->startPlayback() )
    {
        emit started( m_currentTrack );

        DatabaseCommand_LogPlayback* cmd = new DatabaseCommand_LogPlayback( m_currentTrack, DatabaseCommand_LogPlayback::Started );
        Database::instance()->enqueue( QSharedPointer<DatabaseCommand>(cmd) );
    }
    else
    {
        qDebug() << "Can't open device for audio output!";
        stop();
        emit error( AudioEngine::AudioDeviceError );
    }

    qDebug() << Q_FUNC_INFO << sampleRate << channels << "done";
}


void
AudioEngine::timerTriggered( unsigned int seconds )
{
    m_timeElapsed = seconds;
    emit timerSeconds( seconds );

    if ( m_currentTrack->duration() == 0 )
    {
        emit timerPercentage( 0 );
    }
    else
    {
        emit timerPercentage( (unsigned int)( seconds / m_currentTrack->duration() ) );
    }
}


void
AudioEngine::clearBuffers()
{
    QMutexLocker lock( &m_mutex );
    m_audio->clearBuffers();
}


void
AudioEngine::setPlaylist( PlaylistInterface* playlist )
{
    m_playlist = playlist;
    emit playlistChanged( playlist );
}


void
AudioEngine::setCurrentTrack( const Tomahawk::result_ptr& result )
{
    m_lastTrack = m_currentTrack;
    if ( !m_lastTrack.isNull() )
    {
        DatabaseCommand_LogPlayback* cmd = new DatabaseCommand_LogPlayback( m_lastTrack, DatabaseCommand_LogPlayback::Finished, m_timeElapsed );
        Database::instance()->enqueue( QSharedPointer<DatabaseCommand>(cmd) );

        emit finished( m_lastTrack );
    }

    m_currentTrack = result;
}


void
AudioEngine::onDownloadProgress( qint64 recv, qint64 total )
{
    if ( ( recv > 1024 * 32 ) || recv > total ) 
        m_readReady = true;
}


void
AudioEngine::run()
{
    QTimer::singleShot( 0, this, SLOT( engineLoop() ) );
    exec();
    qDebug() << "AudioEngine event loop stopped";
}


void
AudioEngine::engineLoop()
{
    qDebug() << "AudioEngine thread:" << this->thread();
    loop();
}


void
AudioEngine::loop()
{
    m_i++;
    //if( m_i % 500 == 0 ) qDebug() << Q_FUNC_INFO << thread();

    {
        QMutexLocker lock( &m_mutex );

/*        if ( m_i % 200 == 0 )
        {
            if ( !m_input.isNull() )
                qDebug() << "Outer audio loop" << m_input->bytesAvailable() << m_audio->needData();
        }*/

        if ( m_i % 10 == 0 && m_audio->isPlaying() )
            m_audio->triggerTimers();

        if( !m_transcode.isNull() &&
            !m_input.isNull() &&
            m_input->bytesAvailable() &&
            m_audio->needData() &&
            !m_audio->isPaused() )
        {
            //if ( m_i % 50 == 0 )
            //    qDebug() << "Inner audio loop";

            if ( m_transcode->needData() > 0 )
            {
                QByteArray encdata = m_input->read( m_transcode->preferredDataSize() );
                m_transcode->processData( encdata, m_input->atEnd() );
            }

            if ( m_transcode->haveData() )
            {
                QByteArray rawdata = m_transcode->data();
                m_audio->processData( rawdata );
            }

            QTimer::singleShot( 0, this, SLOT( loop() ) );
            return;
        }
    }

    unsigned int nextdelay = 50;
    // are we cleanly at the end of a track, and ready for the next one?
    if ( !m_input.isNull() &&
          m_input->atEnd() &&
          m_readReady && 
         !m_input->bytesAvailable() &&
         !m_audio->haveData() &&
         !m_audio->isPaused() )
    {
        qDebug() << "Starting next track then";
        loadNextTrack();
        // will need data immediately:
        nextdelay = 0;
    }
    else if ( !m_input.isNull() && !m_input->isOpen() )
    {
        qDebug() << "AudioEngine IODev closed. errorString:" << m_input->errorString();
        loadNextTrack();
        nextdelay = 0;
    }

    QTimer::singleShot( nextdelay, this, SLOT( loop() ) );
}
