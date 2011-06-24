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
#include "sourceplaylistinterface.h"
#include "tomahawksettings.h"

#include "database/database.h"
#include "database/databasecommand_logplayback.h"
#include "infosystem/infosystem.h"
#include "network/servent.h"

#include "album.h"

using namespace Tomahawk;

AudioEngine* AudioEngine::s_instance = 0;

static QString s_aeInfoIdentifier = QString( "AUDIOENGINE" );

AudioEngine*
AudioEngine::instance()
{
    return s_instance;
}


AudioEngine::AudioEngine()
    : QObject()
    , m_isPlayingHttp( false )
    , m_playlist( 0 )
    , m_currentTrackPlaylist( 0 )
    , m_queue( 0 )
    , m_timeElapsed( 0 )
    , m_expectStop( false )
    , m_waitingOnNewTrack( false )
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
    connect( m_mediaObject, SIGNAL( aboutToFinish() ), SLOT( onAboutToFinish() ) );

    connect( m_audioOutput, SIGNAL( volumeChanged( qreal ) ), this, SLOT( onVolumeChanged( qreal ) ) );

    onVolumeChanged( m_audioOutput->volume() );
#ifdef Q_OS_MAC
    // On mac, phonon volume is independent from system volume, so the onVolumeChanged call above just sets our volume to 100%.
    // Since it's indendent, we'll set it to 75% since that's nicer
    setVolume( 75 );
#endif
}


AudioEngine::~AudioEngine()
{
    qDebug() << Q_FUNC_INFO;
    m_mediaObject->stop();
//    stop();

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
        Tomahawk::InfoSystem::InfoCriteriaHash trackInfo;

        trackInfo["title"] = m_currentTrack->track();
        trackInfo["artist"] = m_currentTrack->artist()->name();
        trackInfo["album"] = m_currentTrack->album()->name();
        Tomahawk::InfoSystem::InfoSystem::instance()->pushInfo(
        s_aeInfoIdentifier, Tomahawk::InfoSystem::InfoNowResumed,
        QVariant::fromValue< Tomahawk::InfoSystem::InfoCriteriaHash >( trackInfo ) );
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
    Tomahawk::InfoSystem::InfoSystem::instance()->pushInfo( s_aeInfoIdentifier, Tomahawk::InfoSystem::InfoNowPaused, QVariant() );
}


void
AudioEngine::stop()
{
    qDebug() << Q_FUNC_INFO;

    m_mediaObject->stop();

    if ( m_playlist )
        m_playlist->reset();

    setCurrentTrack( Tomahawk::result_ptr() );
    emit stopped();

    Tomahawk::InfoSystem::InfoMap map;
    map[ Tomahawk::InfoSystem::InfoNowStopped ] = QVariant();

    if ( m_waitingOnNewTrack )
        sendWaitingNotification();
    else if ( TomahawkSettings::instance()->verboseNotifications() )
    {
        Tomahawk::InfoSystem::InfoCriteriaHash stopInfo;
        stopInfo["message"] = QString( "Tomahawk is stopped." );
        map[ Tomahawk::InfoSystem::InfoNotifyUser ] = QVariant::fromValue< Tomahawk::InfoSystem::InfoCriteriaHash >( stopInfo );
    }

    Tomahawk::InfoSystem::InfoSystem::instance()->pushInfo( s_aeInfoIdentifier, map );
}


void
AudioEngine::previous()
{
    qDebug() << Q_FUNC_INFO;

    if ( !m_playlist )
        return;

    if ( m_playlist->skipRestrictions() == PlaylistInterface::NoSkip ||
         m_playlist->skipRestrictions() == PlaylistInterface::NoSkipBackwards )
        return;

    loadPreviousTrack();
}


void
AudioEngine::next()
{
    qDebug() << Q_FUNC_INFO;

    if ( !m_playlist )
        return;

    if ( m_playlist->skipRestrictions() == PlaylistInterface::NoSkip ||
         m_playlist->skipRestrictions() == PlaylistInterface::NoSkipForwards )
        return;

    if ( !m_currentTrack.isNull() && !m_playlist->hasNextItem() &&
          m_currentTrack->id() == m_playlist->currentItem()->id() )
    {
        //For instance, when doing a catch-up while listening along, but the person
        //you're following hasn't started a new track yet...don't do anything
        return;
    }

    loadNextTrack();
}


void
AudioEngine::seek( int ms )
{
    if ( !m_playlist )
        return;

    if ( m_playlist->seekRestrictions() == PlaylistInterface::NoSeek )
        return;

    if ( isPlaying() || isPaused() )
    {
        qDebug() << Q_FUNC_INFO << ms;
        m_mediaObject->seek( ms );
    }
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
AudioEngine::sendWaitingNotification() const
{
    Tomahawk::InfoSystem::InfoCriteriaHash retryInfo;
    retryInfo["message"] = QString( "The current track could not be resolved. Tomahawk will pick back up with the next resolvable track from this source." );
    Tomahawk::InfoSystem::InfoSystem::instance()->pushInfo(
        s_aeInfoIdentifier, Tomahawk::InfoSystem::InfoNotifyUser,
        QVariant::fromValue< Tomahawk::InfoSystem::InfoCriteriaHash >( retryInfo ) );
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

            if ( !isHttpResult( m_currentTrack->url() ) && !isLocalResult( m_currentTrack->url() ) )
            {
                io = Servent::instance()->getIODeviceForUrl( m_currentTrack );

                if ( !io || io.isNull() )
                {
                    qDebug() << "Error getting iodevice for item";
                    err = true;
                }
            }
        }

        if ( !err )
        {
            qDebug() << "Starting new song from url:" << m_currentTrack->url();
            emit loading( m_currentTrack );

            if ( !isHttpResult( m_currentTrack->url() ) && !isLocalResult( m_currentTrack->url() ) )
            {
                m_mediaObject->setCurrentSource( io.data() );
                m_mediaObject->currentSource().setAutoDelete( false );
                m_isPlayingHttp = false;
            }
            else
            {
                QUrl furl = m_currentTrack->url();
                if ( m_currentTrack->url().contains( "?" ) )
                {
                    furl = QUrl( m_currentTrack->url().left( m_currentTrack->url().indexOf( '?' ) ) );
                    furl.setEncodedQuery( QString( m_currentTrack->url().mid( m_currentTrack->url().indexOf( '?' ) + 1 ) ).toLocal8Bit() );
                }
                m_mediaObject->setCurrentSource( furl );
                m_mediaObject->currentSource().setAutoDelete( true );
                m_isPlayingHttp = true;
            }

            if ( !m_input.isNull() )
            {
                m_input->close();
                m_input.clear();
            }
            m_input = io;
            m_mediaObject->play();
            emit started( m_currentTrack );

            DatabaseCommand_LogPlayback* cmd = new DatabaseCommand_LogPlayback( m_currentTrack, DatabaseCommand_LogPlayback::Started );
            Database::instance()->enqueue( QSharedPointer<DatabaseCommand>(cmd) );

            Tomahawk::InfoSystem::InfoMap map;

            Tomahawk::InfoSystem::InfoCriteriaHash trackInfo;
            trackInfo["title"] = m_currentTrack->track();
            trackInfo["artist"] = m_currentTrack->artist()->name();
            trackInfo["album"] = m_currentTrack->album()->name();
            map[ Tomahawk::InfoSystem::InfoNowPlaying ] = QVariant::fromValue< Tomahawk::InfoSystem::InfoCriteriaHash >( trackInfo );

            if ( TomahawkSettings::instance()->verboseNotifications() )
            {
                Tomahawk::InfoSystem::InfoCriteriaHash playInfo;
                playInfo["message"] = QString( "Tomahawk is playing \"%1\" by %2 on album %3." )
                    .arg( m_currentTrack->track() )
                    .arg( m_currentTrack->artist()->name() )
                    .arg( m_currentTrack->album()->name() );
                map[ Tomahawk::InfoSystem::InfoNotifyUser ] = QVariant::fromValue< Tomahawk::InfoSystem::InfoCriteriaHash >( playInfo );
            }

            Tomahawk::InfoSystem::InfoSystem::instance()->pushInfo( s_aeInfoIdentifier, map );
        }
    }

    if ( err )
    {
        stop();
        return false;
    }

    m_waitingOnNewTrack = false;
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
    {
        if ( m_playlist && m_playlist->retryMode() == Tomahawk::PlaylistInterface::Retry )
            m_waitingOnNewTrack = true;
        stop();
    }
}


void
AudioEngine::playItem( Tomahawk::PlaylistInterface* playlist, const Tomahawk::result_ptr& result )
{
    qDebug() << Q_FUNC_INFO;

    if ( m_playlist )
        m_playlist->reset();

    setPlaylist( playlist );
    m_currentTrackPlaylist = playlist;

    if ( !result.isNull() )
        loadTrack( result );
    else if ( m_playlist && m_playlist->retryMode() == PlaylistInterface::Retry )
    {
        m_waitingOnNewTrack = true;
        stop();
    }
}


void
AudioEngine::playlistNextTrackReady()
{
    if ( !m_waitingOnNewTrack )
        return;

    m_waitingOnNewTrack = false;
    next();
}


void
AudioEngine::onAboutToFinish()
{
    qDebug() << Q_FUNC_INFO;
    m_expectStop = true;
}


void
AudioEngine::onStateChanged( Phonon::State newState, Phonon::State oldState )
{
    qDebug() << Q_FUNC_INFO << oldState << newState << m_expectStop;

    if ( newState == Phonon::ErrorState )
    {
        qDebug() << "Phonon Error:" << m_mediaObject->errorString() << m_mediaObject->errorType();
        return;
    }

    if ( !m_expectStop )
        return;
    m_expectStop = false;

    if ( oldState == Phonon::PlayingState )
    {
        if ( newState == Phonon::PausedState || newState == Phonon::StoppedState )
        {
            qDebug() << "Loading next track.";
            loadNextTrack();
        }
    }
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
    if ( m_playlist )
        m_playlist->reset();

    m_playlist = playlist;

    if ( m_playlist && m_playlist->object() && m_playlist->retryMode() == PlaylistInterface::Retry )
        connect( m_playlist->object(), SIGNAL( nextTrackReady() ), SLOT( playlistNextTrackReady() ) );

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


bool
AudioEngine::isHttpResult( const QString& url ) const
{
    return url.startsWith( "http://" );
}


bool
AudioEngine::isLocalResult( const QString& url ) const
{
    return url.startsWith( "file://" );
}
