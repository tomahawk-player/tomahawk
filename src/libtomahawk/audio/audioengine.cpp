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

#include "audioengine.h"

#include <QUrl>

#include "playlistinterface.h"

#include "database/database.h"
#include "database/databasecommand_logplayback.h"
#include "network/servent.h"

AudioEngine* AudioEngine::s_instance = 0;


AudioEngine*
AudioEngine::instance()
{
    return s_instance;
}


AudioEngine::AudioEngine()
    : QObject()
    , m_playlist( 0 )
    , m_currentTrackPlaylist( 0 )
    , m_queue( 0 )
    , m_timeElapsed( 0 )
    , m_expectStop( false )
{
    s_instance = this;
    qDebug() << "Init AudioEngine";

    qRegisterMetaType< AudioErrorCode >("AudioErrorCode");

    m_mediaObject = new Phonon::MediaObject( this );
    m_audioOutput = new Phonon::AudioOutput( Phonon::MusicCategory, this );
    Phonon::createPath( m_mediaObject, m_audioOutput );

    m_mediaObject->setTickInterval( 150 );
    connect( m_mediaObject, SIGNAL( stateChanged( Phonon::State, Phonon::State ) ), SLOT( onStateChanged( Phonon::State, Phonon::State ) ) );
    connect( m_mediaObject, SIGNAL( tick( qint64 ) ), SLOT( timerTriggered( qint64 ) ) );
}


AudioEngine::~AudioEngine()
{
    qDebug() << Q_FUNC_INFO;

    stop();

    delete m_audioOutput;
    delete m_mediaObject;
}


void
AudioEngine::playPause()
{
    if ( isPlaying() )
        pause();
    else
        play();
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

    m_expectStop = true;
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
AudioEngine::mute()
{
    setVolume( 0 );
}


void
AudioEngine::onTrackAboutToFinish()
{
    qDebug() << Q_FUNC_INFO;
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
        }

        if ( !err )
        {
            qDebug() << "Starting new song from url:" << m_currentTrack->url();
            emit loading( m_currentTrack );

            if ( !m_input.isNull() )
            {
                m_expectStop = true;
            }

            m_input = io;

            if ( !m_currentTrack->url().startsWith( "http://" ) )
            {
                m_mediaObject->setCurrentSource( io.data() );
            }
            else
            {
                QUrl furl = m_currentTrack->url();
                if ( m_currentTrack->url().contains( "?" ) )
                {
                    furl = QUrl( m_currentTrack->url().left( m_currentTrack->url().indexOf( '?' ) ) );
                    furl.setEncodedQuery( QString( m_currentTrack->url().mid( m_currentTrack->url().indexOf( '?' ) + 1 ) ).toLocal8Bit() );
                    qDebug() << Q_FUNC_INFO << furl;
                }
                m_mediaObject->setCurrentSource( furl );
            }
            m_mediaObject->currentSource().setAutoDelete( true );
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

    setPlaylist( playlist );
    m_currentTrackPlaylist = playlist;

    loadTrack( result );
}


void
AudioEngine::onStateChanged( Phonon::State newState, Phonon::State oldState )
{
    qDebug() << Q_FUNC_INFO << oldState << newState;
    if ( oldState == Phonon::PlayingState && newState == Phonon::StoppedState )
    {
        if ( !m_expectStop )
        {
            m_expectStop = false;
            loadNextTrack();
        }
    }

    m_expectStop = false;
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
