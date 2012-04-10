/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include "config.h"

#include <QtCore/QUrl>
#include <QtNetwork/QNetworkReply>
#include <QTemporaryFile>

#include "playlistinterface.h"
#include "sourceplaylistinterface.h"
#include "tomahawksettings.h"
#include "database/database.h"
#include "database/databasecommand_logplayback.h"
#include "network/servent.h"
#include "utils/qnr_iodevicestream.h"
#include "utils/closure.h"
#include "headlesscheck.h"
#include "infosystem/infosystem.h"
#include "album.h"

#include "utils/logger.h"


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
    , m_queue( 0 )
    , m_timeElapsed( 0 )
    , m_expectStop( false )
    , m_waitingOnNewTrack( false )
    , m_state( Stopped )
{
    s_instance = this;
    tDebug() << "Init AudioEngine";

    qRegisterMetaType< AudioErrorCode >("AudioErrorCode");
    qRegisterMetaType< AudioState >("AudioState");

    m_mediaObject = new Phonon::MediaObject( this );
    m_audioOutput = new Phonon::AudioOutput( Phonon::MusicCategory, this );
    Phonon::createPath( m_mediaObject, m_audioOutput );

    m_mediaObject->setTickInterval( 150 );
    connect( m_mediaObject, SIGNAL( stateChanged( Phonon::State, Phonon::State ) ), SLOT( onStateChanged( Phonon::State, Phonon::State ) ) );
    connect( m_mediaObject, SIGNAL( tick( qint64 ) ), SLOT( timerTriggered( qint64 ) ) );
    connect( m_mediaObject, SIGNAL( aboutToFinish() ), SLOT( onAboutToFinish() ) );

    connect( m_audioOutput, SIGNAL( volumeChanged( qreal ) ), SLOT( onVolumeChanged( qreal ) ) );

    onVolumeChanged( m_audioOutput->volume() );

#ifndef Q_WS_X11
    // On mac & win, phonon volume is independent from system volume, so the onVolumeChanged call above just sets our volume to 100%.
    // Since it's indendent, we'll set it to 75% since that's nicer.
    setVolume( 75 );
#endif
}


AudioEngine::~AudioEngine()
{
    tDebug() << Q_FUNC_INFO;
    m_mediaObject->stop();
//    stop();

    delete m_audioOutput;
    delete m_mediaObject;
}


QStringList
AudioEngine::supportedMimeTypes() const
{
    if ( m_supportedMimeTypes.isEmpty() )
    {
        m_supportedMimeTypes = Phonon::BackendCapabilities::availableMimeTypes();
        m_supportedMimeTypes << "audio/basic";

        return m_supportedMimeTypes;
    }
    else
        return m_supportedMimeTypes;
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
    tDebug( LOGEXTRA ) << Q_FUNC_INFO;

    if ( isPaused() )
    {
        setVolume( m_volume );
        m_mediaObject->play();
        setVolume( m_volume );
        emit resumed();

        sendNowPlayingNotification( Tomahawk::InfoSystem::InfoNowResumed );
    }
    else
        next();
}


void
AudioEngine::pause()
{
    tDebug( LOGEXTRA ) << Q_FUNC_INFO;

    m_volume = volume();
    m_mediaObject->pause();
    emit paused();

    Tomahawk::InfoSystem::InfoSystem::instance()->pushInfo( Tomahawk::InfoSystem::InfoPushData( s_aeInfoIdentifier, Tomahawk::InfoSystem::InfoNowPaused, QVariant(), Tomahawk::InfoSystem::PushNoFlag ) );
}


void
AudioEngine::stop()
{
    tDebug( LOGEXTRA ) << Q_FUNC_INFO;

    emit stopped();
    if ( isStopped() )
        return;

    setState( Stopped );
    m_mediaObject->stop();

    if ( !m_playlist.isNull() )
        m_playlist.data()->reset();
    if ( !m_currentTrack.isNull() )
        emit timerPercentage( ( (double)m_timeElapsed / (double)m_currentTrack->duration() ) * 100.0 );

    setCurrentTrack( Tomahawk::result_ptr() );

    if ( m_waitingOnNewTrack )
        sendWaitingNotification();

    Tomahawk::InfoSystem::InfoPushData pushData( s_aeInfoIdentifier, Tomahawk::InfoSystem::InfoNowStopped, QVariant(), Tomahawk::InfoSystem::PushNoFlag );
    
    Tomahawk::InfoSystem::InfoSystem::instance()->pushInfo( pushData );
}


void
AudioEngine::previous()
{
    tDebug( LOGEXTRA ) << Q_FUNC_INFO;

    if ( canGoPrevious() )
        loadPreviousTrack();
}


void
AudioEngine::next()
{
    tDebug( LOGEXTRA ) << Q_FUNC_INFO;

    if ( canGoNext() )
        loadNextTrack();
}


bool
AudioEngine::canGoNext()
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO;

    if ( m_queue && m_queue->trackCount() )
        return true;

    if ( m_playlist.isNull() )
        return false;

    if ( m_playlist.data()->skipRestrictions() == PlaylistInterface::NoSkip ||
         m_playlist.data()->skipRestrictions() == PlaylistInterface::NoSkipForwards )
        return false;

    if ( !m_currentTrack.isNull() && !m_playlist->hasNextItem() &&
         ( m_playlist->currentItem().isNull() || ( m_currentTrack->id() == m_playlist->currentItem()->id() ) ) )
    {
        //For instance, when doing a catch-up while listening along, but the person
        //you're following hasn't started a new track yet...don't do anything
        tDebug( LOGEXTRA ) << Q_FUNC_INFO << "catch up, but same track or can't move on because don't have next track or it wasn't resolved";
        return false;
    }

    return m_playlist.data()->hasNextItem();
}


bool
AudioEngine::canGoPrevious()
{
    if ( m_playlist.isNull() )
        return false;

    if ( m_playlist.data()->skipRestrictions() == PlaylistInterface::NoSkip ||
         m_playlist.data()->skipRestrictions() == PlaylistInterface::NoSkipBackwards )
        return false;

    return true;
}


bool
AudioEngine::canSeek()
{
    bool phononCanSeek = true;
    /* TODO: When phonon properly reports this, re-enable it
    if ( m_mediaObject && m_mediaObject->isValid() )
        phononCanSeek = m_mediaObject->isSeekable();
    */
    return !m_playlist.isNull() && ( m_playlist.data()->seekRestrictions() != PlaylistInterface::NoSeek ) && phononCanSeek;
}


void
AudioEngine::seek( qint64 ms )
{
    if ( !canSeek() )
    {
        tDebug( LOGEXTRA ) << "Could not seek!";
        return;
    }

    if ( isPlaying() || isPaused() )
    {
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << ms;
        m_mediaObject->seek( ms );
        emit seeked( ms );
    }
}


void
AudioEngine::seek( int ms )
{
    seek( (qint64) ms );
}


void
AudioEngine::setVolume( int percentage )
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << percentage;

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
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO;
    //since it's async, after this is triggered our result could come in, so don't show the popup in that case
    if ( !m_playlist.isNull() && m_playlist->hasNextItem() )
        return;

    Tomahawk::InfoSystem::InfoPushData pushData (
        s_aeInfoIdentifier, Tomahawk::InfoSystem::InfoTrackUnresolved,
        QVariant(),
        Tomahawk::InfoSystem::PushNoFlag );

    Tomahawk::InfoSystem::InfoSystem::instance()->pushInfo( pushData );
}


void
AudioEngine::sendNowPlayingNotification( const Tomahawk::InfoSystem::InfoType type )
{
#ifndef ENABLE_HEADLESS
    if ( m_currentTrack->album().isNull() || m_currentTrack->album()->infoLoaded() )
        onNowPlayingInfoReady( type );
    else
    {
        _detail::Closure* closure = NewClosure( m_currentTrack->album().data(), SIGNAL( updated() ), const_cast< AudioEngine* >( this ), SLOT( onNowPlayingInfoReady( const Tomahawk::InfoSystem::InfoType ) ), type );
        Q_UNUSED( closure );
        m_currentTrack->album()->cover( QSize( 0, 0 ) );
    }
#endif
}


void
AudioEngine::onNowPlayingInfoReady( const Tomahawk::InfoSystem::InfoType type )
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO;
    if ( m_currentTrack.isNull() ||
         m_currentTrack->track().isNull() ||
         m_currentTrack->artist().isNull() )
        return;

    if ( !m_currentTrack->album().isNull() && sender() && m_currentTrack->album().data() != sender() )
        return;
    
    QVariantMap playInfo;

    if ( !m_currentTrack->album().isNull() )
    {
#ifndef ENABLE_HEADLESS
        QImage cover;
        cover = m_currentTrack->album()->cover( QSize( 0, 0 ) ).toImage();
        playInfo["cover"] = cover;

        QTemporaryFile coverTempFile( QDir::toNativeSeparators( QDir::tempPath() + "/" + m_currentTrack->artist()->name() + "_" + m_currentTrack->album()->name() + "_tomahawk_cover.png" ) );
        if ( !coverTempFile.open() )
        {
            tDebug() << "WARNING: could not write temporary file for cover art!";
        }

        // Finally, save the image to the new temp file
        if ( cover.save( &coverTempFile, "PNG" ) )
        {
            tDebug( LOGVERBOSE ) << "Saving cover image to:" << QFileInfo( coverTempFile ).absoluteFilePath();
            coverTempFile.close();
            playInfo["coveruri"] = QFileInfo( coverTempFile ).absoluteFilePath();
        }
        else
        {
            tDebug() << Q_FUNC_INFO << "failed to save cover image!";
            coverTempFile.close();
        }
#endif
    }

    Tomahawk::InfoSystem::InfoStringHash trackInfo;
    trackInfo["title"] = m_currentTrack->track();
    trackInfo["artist"] = m_currentTrack->artist()->name();
    trackInfo["album"] = m_currentTrack->album()->name();
    trackInfo["duration"] = QString::number( m_currentTrack->duration() );
    trackInfo["albumpos"] = QString::number( m_currentTrack->albumpos() );

    playInfo["trackinfo"] = QVariant::fromValue< Tomahawk::InfoSystem::InfoStringHash >( trackInfo );
    playInfo["private"] = TomahawkSettings::instance()->privateListeningMode();
    
    Tomahawk::InfoSystem::InfoPushData pushData ( s_aeInfoIdentifier, type, playInfo, Tomahawk::InfoSystem::PushShortUrlFlag );

    Tomahawk::InfoSystem::InfoSystem::instance()->pushInfo( pushData );
}


bool
AudioEngine::loadTrack( const Tomahawk::result_ptr& result )
{
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
                    tLog() << "Error getting iodevice for" << result->url();
                    err = true;
                }
            }
        }

        if ( !err )
        {
            tLog() << "Starting new song:" << m_currentTrack->url();
            emit loading( m_currentTrack );

            if ( !isHttpResult( m_currentTrack->url() ) && !isLocalResult( m_currentTrack->url() ) )
            {
                if ( QNetworkReply* qnr_io = qobject_cast< QNetworkReply* >( io.data() ) )
                    m_mediaObject->setCurrentSource( new QNR_IODeviceStream( qnr_io, this ) );
                else
                    m_mediaObject->setCurrentSource( io.data() );
                m_mediaObject->currentSource().setAutoDelete( false );
            }
            else
            {
                if ( !isLocalResult( m_currentTrack->url() ) )
                {
                    QUrl furl = m_currentTrack->url();
                    if ( m_currentTrack->url().contains( "?" ) )
                    {
                        furl = QUrl( m_currentTrack->url().left( m_currentTrack->url().indexOf( '?' ) ) );
                        furl.setEncodedQuery( QString( m_currentTrack->url().mid( m_currentTrack->url().indexOf( '?' ) + 1 ) ).toLocal8Bit() );
                    }
                    m_mediaObject->setCurrentSource( furl );
                }
                else
                {
                    QString furl = m_currentTrack->url();
#ifdef Q_WS_WIN
                    if ( furl.startsWith( "file://" ) )
                        furl = furl.right( furl.length() - 7 );
#endif
                    tLog( LOGVERBOSE ) << "Passing to Phonon:" << furl << furl.toLatin1();
                    m_mediaObject->setCurrentSource( furl );
                }

                m_mediaObject->currentSource().setAutoDelete( true );
            }

            if ( !m_input.isNull() )
            {
                m_input->close();
                m_input.clear();
            }
            m_input = io;
            m_mediaObject->play();
            emit started( m_currentTrack );

            if ( TomahawkSettings::instance()->privateListeningMode() != TomahawkSettings::FullyPrivate )
            {
                DatabaseCommand_LogPlayback* cmd = new DatabaseCommand_LogPlayback( m_currentTrack, DatabaseCommand_LogPlayback::Started );
                Database::instance()->enqueue( QSharedPointer<DatabaseCommand>(cmd) );
            }
            
            sendNowPlayingNotification( Tomahawk::InfoSystem::InfoNowPlaying );
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
    tDebug( LOGEXTRA ) << Q_FUNC_INFO;

    if ( m_playlist.isNull() )
    {
        stop();
        return;
    }

    Tomahawk::result_ptr result = m_playlist.data()->previousItem();
    if ( !result.isNull() )
        loadTrack( result );
    else
        stop();
}


void
AudioEngine::loadNextTrack()
{
    tDebug( LOGEXTRA ) << Q_FUNC_INFO;

    Tomahawk::result_ptr result;

    if ( !m_stopAfterTrack.isNull() )
    {
        if ( m_stopAfterTrack->equals( m_currentTrack->toQuery() ) )
        {
            m_stopAfterTrack.clear();
            stop();
            return;
        }
    }

    if ( m_queue && m_queue->trackCount() )
    {
        result = m_queue->nextItem();
    }

    if ( !m_playlist.isNull() && result.isNull() )
    {
        tDebug( LOGEXTRA ) << Q_FUNC_INFO << "Loading playlist's next item";
        result = m_playlist.data()->nextItem();
        m_currentTrackPlaylist = m_playlist;
    }

    if ( !result.isNull() )
    {
        tDebug( LOGEXTRA ) << Q_FUNC_INFO << "Got next item, loading track";
        loadTrack( result );
    }
    else
    {
        if ( !m_playlist.isNull() && m_playlist.data()->retryMode() == Tomahawk::PlaylistInterface::Retry )
            m_waitingOnNewTrack = true;

        stop();
    }
}


void
AudioEngine::playItem( Tomahawk::playlistinterface_ptr playlist, const Tomahawk::result_ptr& result )
{
    tDebug( LOGEXTRA ) << Q_FUNC_INFO << ( result.isNull() ? QString() : result->url() );

    if ( !m_playlist.isNull() )
        m_playlist.data()->reset();

    setPlaylist( playlist );
    m_currentTrackPlaylist = playlist;

    if ( !result.isNull() )
    {
        loadTrack( result );
    }
    else if ( !m_playlist.isNull() && m_playlist.data()->retryMode() == PlaylistInterface::Retry )
    {
        m_waitingOnNewTrack = true;
        if ( isStopped() )
            emit sendWaitingNotification();
        else
            stop();
    }
}


void
AudioEngine::onPlaylistNextTrackReady()
{
    // If in real-time and you have a few seconds left, you're probably lagging -- finish it up
    if ( m_playlist && m_playlist->latchMode() == PlaylistInterface::RealTime && ( m_waitingOnNewTrack || m_currentTrack.isNull() || m_currentTrack->id() == 0 || ( currentTrackTotalTime() - currentTime() > 6000 ) ) )
    {
        m_waitingOnNewTrack = false;
        loadNextTrack();
        return;
    }

    if ( !m_waitingOnNewTrack )
        return;

    m_waitingOnNewTrack = false;
    loadNextTrack();
}


void
AudioEngine::onAboutToFinish()
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO;
    m_expectStop = true;
}


void
AudioEngine::onStateChanged( Phonon::State newState, Phonon::State oldState )
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << oldState << newState << m_expectStop;

    if ( newState == Phonon::ErrorState )
    {
        stop();

        tLog() << "Phonon Error:" << m_mediaObject->errorString() << m_mediaObject->errorType();
        emit error( UnknownError );
        return;
    }
    if ( newState == Phonon::PlayingState )
        setState( Playing );

    if ( oldState == Phonon::PlayingState )
    {
        bool stopped = false;
        switch ( newState )
        {
            case Phonon::PausedState:
            {
                qint64 duration = m_mediaObject->totalTime() > 0 ? m_mediaObject->totalTime() : m_currentTrack->duration() * 1000;
                stopped = ( duration - 1000 < m_mediaObject->currentTime() );
                if ( !stopped )
                    setState( Paused );

                break;
            }
            case Phonon::StoppedState:
            {
                stopped = true;
                break;
            }
            default:
                break;
        }

        if ( stopped && m_expectStop )
        {
            m_expectStop = false;
            tDebug( LOGEXTRA ) << "Finding next track.";
            if ( canGoNext() )
                loadNextTrack();
            else
            {
                if ( !m_playlist.isNull() && m_playlist.data()->retryMode() == Tomahawk::PlaylistInterface::Retry )
                    m_waitingOnNewTrack = true;
                stop();
            }
        }
    }
}


void
AudioEngine::timerTriggered( qint64 time )
{
    emit timerMilliSeconds( time );

    if ( m_timeElapsed != time / 1000 )
    {
        m_timeElapsed = time / 1000;
        emit timerSeconds( m_timeElapsed );

        if ( !m_currentTrack.isNull() )
        {
            if ( m_currentTrack->duration() == 0 )
            {
                emit timerPercentage( 0 );
            }
            else
            {
                emit timerPercentage( ( (double)m_timeElapsed / (double)m_currentTrack->duration() ) * 100.0 );
            }
        }
    }
}


void
AudioEngine::setPlaylist( Tomahawk::playlistinterface_ptr playlist )
{
    if ( m_playlist == playlist )
        return;

    if ( !m_playlist.isNull() )
    {
        if ( m_playlist.data() && m_playlist.data()->retryMode() == PlaylistInterface::Retry )
            disconnect( m_playlist.data(), SIGNAL( nextTrackReady() ) );
        m_playlist.data()->reset();
    }

    if ( playlist.isNull() )
    {
        m_playlist.clear();
        emit playlistChanged( playlist );
        return;
    }
    
    m_playlist = playlist;
    m_stopAfterTrack.clear();

    if ( !m_playlist.isNull() && m_playlist.data() && m_playlist.data()->retryMode() == PlaylistInterface::Retry )
        connect( m_playlist.data(), SIGNAL( nextTrackReady() ), SLOT( onPlaylistNextTrackReady() ) );

    emit playlistChanged( playlist );
}


void
AudioEngine::setCurrentTrack( const Tomahawk::result_ptr& result )
{
    Tomahawk::result_ptr lastTrack = m_currentTrack;
    if ( !lastTrack.isNull() )
    {
        if ( TomahawkSettings::instance()->privateListeningMode() == TomahawkSettings::PublicListening )
        {
            DatabaseCommand_LogPlayback* cmd = new DatabaseCommand_LogPlayback( lastTrack, DatabaseCommand_LogPlayback::Finished, m_timeElapsed );
            Database::instance()->enqueue( QSharedPointer<DatabaseCommand>(cmd) );
        }

        emit finished( lastTrack );
    }

    m_currentTrack = result;
}


bool
AudioEngine::isHttpResult( const QString& url ) const
{
    return url.startsWith( "http://" ) || url.startsWith( "https://" );
}


bool
AudioEngine::isLocalResult( const QString& url ) const
{
    return url.startsWith( "file://" );
}


void
AudioEngine::setState( AudioState state )
{
    AudioState oldState = m_state;
    m_state = state;

    emit stateChanged( state, oldState );
}
