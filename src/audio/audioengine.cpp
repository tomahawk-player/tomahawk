#include "audioengine.h"

#include <QUrl>

#include "playlistinterface.h"

#include "database/database.h"
#include "database/databasecommand_logplayback.h"
#include "network/servent.h"


AudioEngine::AudioEngine()
    : QObject()
    , m_playlist( 0 )
    , m_currentTrackPlaylist( 0 )
    , m_queue( 0 )
    , m_timeElapsed( 0 )
{
    qDebug() << "Init AudioEngine";

    qRegisterMetaType< AudioErrorCode >("AudioErrorCode");

    m_mediaObject = new Phonon::MediaObject( this );
    m_audioOutput = new Phonon::AudioOutput( Phonon::MusicCategory, this );
    Phonon::createPath( m_mediaObject, m_audioOutput );

    m_mediaObject->setTickInterval( 150 );
    connect( m_mediaObject, SIGNAL( tick( qint64 ) ), SLOT( timerTriggered( qint64 ) ) );
}


AudioEngine::~AudioEngine()
{
    qDebug() << Q_FUNC_INFO << "waiting for event loop to finish...";

    m_mediaObject->stop();

    delete m_audioOutput;
    delete m_mediaObject;

    m_input.clear();
}


void
AudioEngine::play()
{
    qDebug() << Q_FUNC_INFO;

    if ( isPaused() )
    {
        m_mediaObject->play();
        emit resumed();
    }
    else
        loadNextTrack();
}


void
AudioEngine::pause()
{
    qDebug() << Q_FUNC_INFO;

    m_mediaObject->pause();
    emit paused();
}


void
AudioEngine::stop()
{
    qDebug() << Q_FUNC_INFO;

    m_mediaObject->stop();

    if ( !m_input.isNull() )
    {
        m_input->close();
        m_input.clear();
    }

    setCurrentTrack( Tomahawk::result_ptr() );
    emit stopped();
}


void
AudioEngine::previous()
{
    qDebug() << Q_FUNC_INFO;
    loadPreviousTrack();
}


void
AudioEngine::next()
{
    qDebug() << Q_FUNC_INFO;
    loadNextTrack();
}


void
AudioEngine::setVolume( int percentage )
{
    //qDebug() << Q_FUNC_INFO;

    percentage = qBound( 0, percentage, 100 );

    m_audioOutput->setVolume( (qreal)percentage / 100.0 );
    emit volumeChanged( percentage );
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

    {
        QSharedPointer<QIODevice> io;

        if ( result.isNull() )
            err = true;
        else
        {
            setCurrentTrack( result );
            io = Servent::instance()->getIODeviceForUrl( m_currentTrack );

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

            m_input = io;

            m_mediaObject->setCurrentSource( io.data() ) ;
            m_mediaObject->play();

            emit started( m_currentTrack );

            DatabaseCommand_LogPlayback* cmd = new DatabaseCommand_LogPlayback( m_currentTrack, DatabaseCommand_LogPlayback::Started );
            Database::instance()->enqueue( QSharedPointer<DatabaseCommand>(cmd) );
        }
    }

    if ( err )
    {
        stop();
        return false;
    }

    return true;
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

    m_playlist = playlist;
    m_currentTrackPlaylist = playlist;

    loadTrack( result );
}


void
AudioEngine::timerTriggered( qint64 time )
{
    if ( m_timeElapsed != time / 1000 )
    {
        m_timeElapsed = time / 1000;
        emit timerSeconds( m_timeElapsed );

        if ( m_currentTrack->duration() == 0 )
        {
            emit timerPercentage( 0 );
        }
        else
        {
            emit timerPercentage( ( (double)m_timeElapsed / (double)m_currentTrack->duration() ) * 100.0 );
        }
    }

    emit timerMilliSeconds( time );
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
