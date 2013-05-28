/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2012, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright 2013,      Teo Mrnjavac <teo@kde.org>
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

#include "AudioEngine.h"
#include "AudioEngine_p.h"

#include "config.h"

#include "TomahawkSettings.h"
#include "network/Servent.h"
#include "utils/Qnr_IoDeviceStream.h"
#include "utils/Closure.h"
#include "Artist.h"
#include "Album.h"
#include "Track.h"
#include "Pipeline.h"
#include "jobview/JobStatusView.h"
#include "jobview/JobStatusModel.h"
#include "jobview/ErrorStatusMessage.h"
#include "infosystem/InfoSystem.h"

#include "utils/Logger.h"
#include "playlist/SingleTrackPlaylistInterface.h"

#include <QDir>

#include <boost/bind.hpp>

using namespace Tomahawk;

#define AUDIO_VOLUME_STEP 5

static const uint_fast8_t UNDERRUNTHRESHOLD = 2;

static QString s_aeInfoIdentifier = QString( "AUDIOENGINE" );


void
AudioEnginePrivate::onStateChanged( Phonon::State newState, Phonon::State oldState )
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << oldState << newState << expectStop << q_ptr->state();

    if ( newState == Phonon::LoadingState )
    {
        // We don't emit this state to listeners - yet.
        state = AudioEngine::Loading;
    }
    if ( newState == Phonon::BufferingState )
    {
        if ( underrunCount > UNDERRUNTHRESHOLD && !underrunNotified )
        {
            underrunNotified = true;
            //FIXME: Actually notify
        }
        else
            underrunCount++;
    }
    if ( newState == Phonon::ErrorState )
    {
        q_ptr->stop( AudioEngine::UnknownError );

        tDebug() << "Phonon Error:" << mediaObject->errorString() << mediaObject->errorType();

        emit q_ptr->error( AudioEngine::UnknownError );
        q_ptr->setState( AudioEngine::Error );
    }
    if ( newState == Phonon::PlayingState )
    {
        if ( q_ptr->state() != AudioEngine::Paused && q_ptr->state() != AudioEngine::Playing )
        {
            underrunCount = 0;
            underrunNotified = false;
            emit q_ptr->started( currentTrack );
        }

        q_ptr->setState( AudioEngine::Playing );
    }
    if ( newState == Phonon::StoppedState && oldState == Phonon::PausedState )
    {
        // GStreamer backend hack: instead of going from PlayingState to StoppedState, it traverses PausedState
        q_ptr->setState( AudioEngine::Stopped );
    }

    if ( oldState == Phonon::PlayingState )
    {
        bool stopped = false;
        switch ( newState )
        {
            case Phonon::PausedState:
            {
                if ( mediaObject && currentTrack )
                {
                    qint64 duration = mediaObject->totalTime() > 0 ? mediaObject->totalTime() : currentTrack->track()->duration() * 1000;
                    stopped = ( duration - 1000 < mediaObject->currentTime() );
                }
                else
                    stopped = true;

                if ( !stopped )
                    q_ptr->setState( AudioEngine::Paused );

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

        if ( stopped && expectStop )
        {
            expectStop = false;
            tDebug( LOGVERBOSE ) << "Finding next track.";
            if ( q_ptr->canGoNext() )
            {
                q_ptr->loadNextTrack();
            }
            else
            {
                if ( !playlist.isNull() && playlist.data()->retryMode() == Tomahawk::PlaylistModes::Retry )
                    waitingOnNewTrack = true;

                q_ptr->stop();
            }
        }
    }

    if ( newState == Phonon::PausedState || newState == Phonon::PlayingState || newState == Phonon::ErrorState )
    {
        tDebug( LOGVERBOSE ) << "Phonon state now:" << newState;
        if ( stateQueue.count() )
        {
            /*/ AudioState qState = */ stateQueue.dequeue();
            q_ptr->checkStateQueue();
        }
    }
}




AudioEngine* AudioEnginePrivate::s_instance = 0;


AudioEngine*
AudioEngine::instance()
{
    return AudioEnginePrivate::s_instance;
}


AudioEngine::AudioEngine()
    : QObject()
    , d_ptr( new AudioEnginePrivate( this ) )
{
    Q_D( AudioEngine );

    d->timeElapsed = 0;
    d->expectStop = false;
    d->waitingOnNewTrack = false;
    d->state = Stopped;
    d->coverTempFile = 0;

    d->s_instance = this;
    tDebug() << "Init AudioEngine";

    qRegisterMetaType< AudioErrorCode >("AudioErrorCode");
    qRegisterMetaType< AudioState >("AudioState");

    d->mediaObject = new Phonon::MediaObject( this );
    d->audioOutput = new Phonon::AudioOutput( Phonon::MusicCategory, this );
    Phonon::createPath( d->mediaObject, d->audioOutput );

    d->mediaObject->setTickInterval( 150 );
    connect( d->mediaObject, SIGNAL( stateChanged( Phonon::State, Phonon::State ) ), d_func(), SLOT( onStateChanged( Phonon::State, Phonon::State ) ) );
    connect( d->mediaObject, SIGNAL( tick( qint64 ) ), SLOT( timerTriggered( qint64 ) ) );
    connect( d->mediaObject, SIGNAL( aboutToFinish() ), SLOT( onAboutToFinish() ) );

    connect( d->audioOutput, SIGNAL( volumeChanged( qreal ) ), SLOT( onVolumeChanged( qreal ) ) );

    d->stateQueueTimer.setInterval( 5000 );
    d->stateQueueTimer.setSingleShot( true );
    connect( &d->stateQueueTimer, SIGNAL( timeout() ), SLOT( queueStateSafety() ) );

    onVolumeChanged( d->audioOutput->volume() );

    setVolume( TomahawkSettings::instance()->volume() );
}


AudioEngine::~AudioEngine()
{
    tDebug() << Q_FUNC_INFO;

    d_func()->mediaObject->stop();
    TomahawkSettings::instance()->setVolume( volume() );


    delete d_ptr;
}


QStringList
AudioEngine::supportedMimeTypes() const
{
    if ( d_func()->supportedMimeTypes.isEmpty() )
    {
        d_func()->supportedMimeTypes = Phonon::BackendCapabilities::availableMimeTypes();
        d_func()->supportedMimeTypes << "audio/basic";

        return d_func()->supportedMimeTypes;
    }
    else
        return d_func()->supportedMimeTypes;
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
    Q_D( AudioEngine );

    tDebug( LOGEXTRA ) << Q_FUNC_INFO;

    if ( isPaused() )
    {
        queueState( Playing );
        emit resumed();

        sendNowPlayingNotification( Tomahawk::InfoSystem::InfoNowResumed );
    }
    else
    {
        if ( !d->currentTrack && d->playlist && d->playlist->nextResult() )
        {
            loadNextTrack();
        }
        else
            next();
    }
}


void
AudioEngine::pause()
{
    tDebug( LOGEXTRA ) << Q_FUNC_INFO;

    queueState( Paused );
    emit paused();

    Tomahawk::InfoSystem::InfoSystem::instance()->pushInfo( Tomahawk::InfoSystem::InfoPushData( s_aeInfoIdentifier, Tomahawk::InfoSystem::InfoNowPaused, QVariant(), Tomahawk::InfoSystem::PushNoFlag ) );
}


void
AudioEngine::stop( AudioErrorCode errorCode )
{
    Q_D( AudioEngine );

    tDebug() << Q_FUNC_INFO << errorCode << isStopped();

    if ( isStopped() )
        return;

    if ( errorCode == NoError )
        setState( Stopped );
    else
        setState( Error );

    if ( d->mediaObject->state() != Phonon::StoppedState )
        d->mediaObject->stop();

    emit stopped();

    if ( !d->playlist.isNull() )
        d->playlist.data()->reset();
    if ( !d->currentTrack.isNull() )
        emit timerPercentage( ( (double)d->timeElapsed / (double)d->currentTrack->track()->duration() ) * 100.0 );

    setCurrentTrack( Tomahawk::result_ptr() );

    if ( d->waitingOnNewTrack )
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
    Q_D( AudioEngine );

    tDebug( LOGVERBOSE ) << Q_FUNC_INFO;

    if ( d->queue && d->queue->trackCount() )
        return true;

    if ( d->playlist.isNull() )
        return false;

    if ( d->playlist.data()->skipRestrictions() == PlaylistModes::NoSkip ||
         d->playlist.data()->skipRestrictions() == PlaylistModes::NoSkipForwards )
    {
        return false;
    }

    if ( !d->currentTrack.isNull() && !d->playlist->hasNextResult() &&
       ( d->playlist->currentItem().isNull() || ( d->currentTrack->id() == d->playlist->currentItem()->id() ) ) )
    {
        //For instance, when doing a catch-up while listening along, but the person
        //you're following hasn't started a new track yet...don't do anything
        tDebug( LOGEXTRA ) << Q_FUNC_INFO << "Catch up, but same track or can't move on because don't have next track or it wasn't resolved";
        return false;
    }

    return ( d->currentTrack && d->playlist.data()->hasNextResult() &&
             !d->playlist.data()->nextResult().isNull() &&
             d->playlist.data()->nextResult()->isOnline() );
}


bool
AudioEngine::canGoPrevious()
{
    Q_D( AudioEngine );

    if ( d->playlist.isNull() )
        return false;

    if ( d->playlist.data()->skipRestrictions() == PlaylistModes::NoSkip ||
        d->playlist.data()->skipRestrictions() == PlaylistModes::NoSkipBackwards )
        return false;

    return ( d->currentTrack && d->playlist.data()->hasPreviousResult() && d->playlist.data()->previousResult()->isOnline() );
}


bool
AudioEngine::canSeek()
{
    Q_D( AudioEngine );

    bool phononCanSeek = true;
    /* TODO: When phonon properly reports this, re-enable it
    if ( d->mediaObject && d->mediaObject->isValid() )
        phononCanSeek = d->mediaObject->isSeekable();
    */
    if ( d->playlist.isNull() )
        return phononCanSeek;

    return !d->playlist.isNull() && ( d->playlist.data()->seekRestrictions() != PlaylistModes::NoSeek ) && phononCanSeek;
}


void
AudioEngine::seek( qint64 ms )
{
    Q_D( AudioEngine );

    if ( !canSeek() )
    {
        tDebug( LOGEXTRA ) << "Could not seek!";
        return;
    }

    if ( isPlaying() || isPaused() )
    {
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << ms;
        d->mediaObject->seek( ms );
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
    Q_D( AudioEngine );

    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << percentage;

    percentage = qBound( 0, percentage, 100 );
    d->audioOutput->setVolume( (qreal)percentage / 100.0 );
    emit volumeChanged( percentage );
}


void
AudioEngine::lowerVolume()
{
    setVolume( volume() - AUDIO_VOLUME_STEP );
}


void
AudioEngine::raiseVolume()
{
    setVolume( volume() + AUDIO_VOLUME_STEP );
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
    if ( d_func()->playlist && d_func()->playlist->nextResult() && d_func()->playlist->nextResult()->isOnline() )
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
    Q_D( AudioEngine );

    if ( d->currentTrack.isNull() )
        return;

#ifndef ENABLE_HEADLESS
    if ( d->currentTrack->track()->coverLoaded() )
    {
        onNowPlayingInfoReady( type );
    }
    else
    {
        NewClosure( d->currentTrack->track().data(), SIGNAL( coverChanged() ), const_cast< AudioEngine* >( this ), SLOT( sendNowPlayingNotification( const Tomahawk::InfoSystem::InfoType ) ), type );
        d->currentTrack->track()->cover( QSize( 0, 0 ), true );
    }
#endif
}


void
AudioEngine::onNowPlayingInfoReady( const Tomahawk::InfoSystem::InfoType type )
{
    Q_D( AudioEngine );

    if ( d->currentTrack.isNull() ||
         d->currentTrack->track()->artist().isEmpty() )
        return;

    QVariantMap playInfo;

#ifndef ENABLE_HEADLESS
    QImage cover;
    cover = d->currentTrack->track()->cover( QSize( 0, 0 ) ).toImage();
    if ( !cover.isNull() )
    {
        playInfo["cover"] = cover;

        delete d->coverTempFile;
        d->coverTempFile = new QTemporaryFile( QDir::toNativeSeparators( QDir::tempPath() + "/" + d->currentTrack->track()->artist() + "_" + d->currentTrack->track()->album() + "_tomahawk_cover.png" ) );
        if ( !d->coverTempFile->open() )
        {
            tDebug() << Q_FUNC_INFO << "WARNING: could not write temporary file for cover art!";
        }
        else
        {
            // Finally, save the image to the new temp file
            if ( cover.save( d->coverTempFile, "PNG" ) )
            {
                tDebug() <<  Q_FUNC_INFO << "Saving cover image to:" << QFileInfo( *d->coverTempFile ).absoluteFilePath();
                playInfo["coveruri"] = QFileInfo( *d->coverTempFile ).absoluteFilePath();
            }
            else
                tDebug() << Q_FUNC_INFO << "Failed to save cover image!";
        }
    }
    else
        tDebug() << Q_FUNC_INFO << "Cover from query is null!";
#endif

    Tomahawk::InfoSystem::InfoStringHash trackInfo;
    trackInfo["title"] = d->currentTrack->track()->track();
    trackInfo["artist"] = d->currentTrack->track()->artist();
    trackInfo["album"] = d->currentTrack->track()->album();
    trackInfo["duration"] = QString::number( d->currentTrack->track()->duration() );
    trackInfo["albumpos"] = QString::number( d->currentTrack->track()->albumpos() );

    playInfo["trackinfo"] = QVariant::fromValue< Tomahawk::InfoSystem::InfoStringHash >( trackInfo );
    playInfo["private"] = TomahawkSettings::instance()->privateListeningMode();

    Tomahawk::InfoSystem::InfoPushData pushData ( s_aeInfoIdentifier, type, playInfo, Tomahawk::InfoSystem::PushShortUrlFlag );
    Tomahawk::InfoSystem::InfoSystem::instance()->pushInfo( pushData );
}


void
AudioEngine::loadTrack( const Tomahawk::result_ptr& result )
{
    Q_D( AudioEngine );

    if ( result.isNull() )
    {
        stop();
        return;
    }

    setCurrentTrack( result );

    if ( !TomahawkUtils::isHttpResult( d->currentTrack->url() ) &&
         !TomahawkUtils::isLocalResult( d->currentTrack->url() ) )
    {
        boost::function< void ( QSharedPointer< QIODevice >& ) > callback =
                boost::bind( &AudioEngine::performLoadTrack, this, result, _1 );
        Servent::instance()->getIODeviceForUrl( d->currentTrack, callback );
    }
    else
    {
        QSharedPointer< QIODevice > io;
        performLoadTrack( result, io );
    }
}


void
AudioEngine::performLoadTrack( const Tomahawk::result_ptr& result, QSharedPointer< QIODevice >& io )
{
    Q_D( AudioEngine );

    bool err = false;
    {
        if ( !TomahawkUtils::isHttpResult( d->currentTrack->url() ) &&
             !TomahawkUtils::isLocalResult( d->currentTrack->url() ) &&
             ( !io || io.isNull() ) )
        {
            tLog() << "Error getting iodevice for" << result->url();
            err = true;
        }

        if ( !err )
        {
            tLog() << "Starting new song:" << d->currentTrack->url();
            d->state = Loading;
            emit loading( d->currentTrack );

            if ( !TomahawkUtils::isHttpResult( d->currentTrack->url() ) &&
                 !TomahawkUtils::isLocalResult( d->currentTrack->url() ) )
            {
                if ( QNetworkReply* qnr_io = qobject_cast< QNetworkReply* >( io.data() ) )
                    d->mediaObject->setCurrentSource( new QNR_IODeviceStream( qnr_io, this ) );
                else
                    d->mediaObject->setCurrentSource( io.data() );
                d->mediaObject->currentSource().setAutoDelete( false );
            }
            else
            {
                if ( !TomahawkUtils::isLocalResult( d->currentTrack->url() ) )
                {
                    QUrl furl = d->currentTrack->url();
                    if ( d->currentTrack->url().contains( "?" ) )
                    {
                        furl = QUrl( d->currentTrack->url().left( d->currentTrack->url().indexOf( '?' ) ) );
                        TomahawkUtils::urlSetQuery( furl, QString( d->currentTrack->url().mid( d->currentTrack->url().indexOf( '?' ) + 1 ) ) );
                    }

                    tLog( LOGVERBOSE ) << "Passing to Phonon:" << furl;
                    d->mediaObject->setCurrentSource( furl );
                }
                else
                {
                    QString furl = d->currentTrack->url();
                    if ( furl.startsWith( "file://" ) )
                        furl = furl.right( furl.length() - 7 );

                    tLog( LOGVERBOSE ) << "Passing to Phonon:" << QUrl::fromLocalFile( furl );
                    d->mediaObject->setCurrentSource( QUrl::fromLocalFile( furl ) );
                }

                d->mediaObject->currentSource().setAutoDelete( true );
            }

            if ( !d->input.isNull() )
            {
                d->input->close();
                d->input.clear();
            }
            d->input = io;
            queueState( Playing );

            if ( TomahawkSettings::instance()->privateListeningMode() != TomahawkSettings::FullyPrivate )
            {
                d->currentTrack->track()->startPlaying();
            }

            sendNowPlayingNotification( Tomahawk::InfoSystem::InfoNowPlaying );
        }
    }

    if ( err )
    {
        stop();
        return;
    }

    d->waitingOnNewTrack = false;
    return;
}


void
AudioEngine::loadPreviousTrack()
{
    Q_D( AudioEngine );

    tDebug( LOGEXTRA ) << Q_FUNC_INFO;

    if ( d->playlist.isNull() )
    {
        stop();
        return;
    }

    Tomahawk::result_ptr result;
    if ( d->playlist.data()->previousResult() )
    {
        result = d->playlist.data()->setSiblingResult( -1 );
        d->currentTrackPlaylist = d->playlist;
    }

    if ( !result.isNull() )
        loadTrack( result );
    else
        stop();
}


void
AudioEngine::loadNextTrack()
{
    Q_D( AudioEngine );

    tDebug( LOGEXTRA ) << Q_FUNC_INFO;

    Tomahawk::result_ptr result;

    if ( d->stopAfterTrack && d->currentTrack )
    {
        if ( d->stopAfterTrack->track()->equals( d->currentTrack->track() ) )
        {
            d->stopAfterTrack.clear();
            stop();
            return;
        }
    }

    if ( d->queue && d->queue->trackCount() )
    {
        query_ptr query = d->queue->tracks().first();
        if ( query && query->numResults() )
            result = query->results().first();
    }

    if ( !d->playlist.isNull() && result.isNull() )
    {
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Loading playlist's next item" << d->playlist.data() << d->playlist->shuffled();

        if ( d->playlist.data()->nextResult() )
        {
            result = d->playlist.data()->setSiblingResult( 1 );
            d->currentTrackPlaylist = d->playlist;
        }
    }

    if ( !result.isNull() )
    {
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Got next item, loading track";
        loadTrack( result );
    }
    else
    {
        if ( !d->playlist.isNull() && d->playlist.data()->retryMode() == Tomahawk::PlaylistModes::Retry )
            d->waitingOnNewTrack = true;

        stop();
    }
}


void
AudioEngine::playItem( Tomahawk::playlistinterface_ptr playlist, const Tomahawk::result_ptr& result, const Tomahawk::query_ptr& fromQuery )
{
    Q_D( AudioEngine );

    tDebug( LOGEXTRA ) << Q_FUNC_INFO << ( result.isNull() ? QString() : result->url() );

    if ( !d->playlist.isNull() )
        d->playlist.data()->reset();

    setPlaylist( playlist );

    if ( playlist.isNull() && !fromQuery.isNull() )
        d->currentTrackPlaylist = playlistinterface_ptr( new SingleTrackPlaylistInterface( fromQuery ) );
    else
        d->currentTrackPlaylist = playlist;

    if ( !result.isNull() )
    {
        loadTrack( result );
    }
    else if ( !d->playlist.isNull() && d->playlist.data()->retryMode() == PlaylistModes::Retry )
    {
        d->waitingOnNewTrack = true;
        if ( isStopped() )
            emit sendWaitingNotification();
        else
            stop();
    }
}


void
AudioEngine::playItem( Tomahawk::playlistinterface_ptr playlist, const Tomahawk::query_ptr& query )
{
    if ( query->resolvingFinished() )
    {
        if ( query->numResults() && query->results().first()->isOnline() )
        {
            playItem( playlist, query->results().first(), query );
            return;
        }

        JobStatusView::instance()->model()->addJob(
            new ErrorStatusMessage( tr( "Sorry, Tomahawk couldn't find the track '%1' by %2" ).arg( query->queryTrack()->track() ).arg( query->queryTrack()->artist() ), 15 ) );

        if ( isStopped() )
            emit stopped(); // we do this so the original caller knows we couldn't find this track
    }
    else
    {
        Pipeline::instance()->resolve( query );

        NewClosure( query.data(), SIGNAL( resolvingFinished( bool ) ),
                    const_cast<AudioEngine*>(this), SLOT( playItem( Tomahawk::playlistinterface_ptr, Tomahawk::query_ptr ) ), playlist, query );
    }
}


void
AudioEngine::playItem( const Tomahawk::artist_ptr& artist )
{
    playlistinterface_ptr pli = artist->playlistInterface( Mixed );
    if ( pli->isFinished() )
    {
        if ( !pli->tracks().count() )
        {
            JobStatusView::instance()->model()->addJob(
                new ErrorStatusMessage( tr( "Sorry, Tomahawk couldn't find the artist '%1'" ).arg( artist->name() ), 15 ) );

            if ( isStopped() )
                emit stopped(); // we do this so the original caller knows we couldn't find this track
        }
        else
            playItem( pli, pli->tracks().first() );
    }
    else
    {
        NewClosure( artist.data(), SIGNAL( tracksAdded( QList<Tomahawk::query_ptr>, Tomahawk::ModelMode, Tomahawk::collection_ptr ) ),
                    const_cast<AudioEngine*>(this), SLOT( playItem( Tomahawk::artist_ptr ) ), artist );
        pli->tracks();
    }
}


void
AudioEngine::playItem( const Tomahawk::album_ptr& album )
{
    playlistinterface_ptr pli = album->playlistInterface( Mixed );
    if ( pli->isFinished() )
    {
        if ( !pli->tracks().count() )
        {
            JobStatusView::instance()->model()->addJob(
                new ErrorStatusMessage( tr( "Sorry, Tomahawk couldn't find the album '%1' by %2" ).arg( album->name() ).arg( album->artist()->name() ), 15 ) );

            if ( isStopped() )
                emit stopped(); // we do this so the original caller knows we couldn't find this track
        }
        else
            playItem( pli, pli->tracks().first() );
    }
    else
    {
        NewClosure( album.data(), SIGNAL( tracksAdded( QList<Tomahawk::query_ptr>, Tomahawk::ModelMode, Tomahawk::collection_ptr ) ),
                    const_cast<AudioEngine*>(this), SLOT( playItem( Tomahawk::album_ptr ) ), album );
        pli->tracks();
    }
}


void
AudioEngine::onPlaylistNextTrackAvailable()
{
    Q_D( AudioEngine );

    tDebug() << Q_FUNC_INFO;

    {
        // If in real-time and you have a few seconds left, you're probably lagging -- finish it up
        if ( d->playlist && d->playlist->latchMode() == PlaylistModes::RealTime && ( d->waitingOnNewTrack || d->currentTrack.isNull() || d->currentTrack->id() == 0 || ( currentTrackTotalTime() - currentTime() > 6000 ) ) )
        {
            d->waitingOnNewTrack = false;
            loadNextTrack();
            return;
        }

        if ( !d->waitingOnNewTrack )
            return;

        d->waitingOnNewTrack = false;
        loadNextTrack();
    }
}


void
AudioEngine::onAboutToFinish()
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO;
    d_func()->expectStop = true;
}

void
AudioEngine::timerTriggered( qint64 time )
{
    Q_D( AudioEngine );

    emit timerMilliSeconds( time );

    if ( d->timeElapsed != time / 1000 )
    {
        d->timeElapsed = time / 1000;
        emit timerSeconds( d->timeElapsed );

        if ( !d->currentTrack.isNull() )
        {
            if ( d->currentTrack->track()->duration() == 0 )
            {
                emit timerPercentage( 0 );
            }
            else
            {
                emit timerPercentage( ( (double) d->timeElapsed / (double) d->currentTrack->track()->duration() ) * 100.0 );
            }
        }
    }
}


void
AudioEngine::setQueue( const playlistinterface_ptr& queue )
{
    Q_D( AudioEngine );

    if ( d->queue )
    {
        disconnect( d->queue.data(), SIGNAL( previousTrackAvailable( bool ) ), this, SIGNAL( controlStateChanged() ) );
        disconnect( d->queue.data(), SIGNAL( nextTrackAvailable( bool ) ), this, SIGNAL( controlStateChanged() ) );
    }

    d->queue = queue;

    if ( d->queue )
    {
        connect( d->queue.data(), SIGNAL( previousTrackAvailable( bool ) ), SIGNAL( controlStateChanged() ) );
        connect( d->queue.data(), SIGNAL( nextTrackAvailable( bool ) ), SIGNAL( controlStateChanged() ) );
    }
}


void
AudioEngine::setPlaylist( Tomahawk::playlistinterface_ptr playlist )
{
    Q_D( AudioEngine );

    if ( d->playlist == playlist )
        return;

    if ( !d->playlist.isNull() )
    {
        if ( d->playlist.data() )
        {
            disconnect( d->playlist.data(), SIGNAL( previousTrackAvailable( bool ) ) );
            disconnect( d->playlist.data(), SIGNAL( nextTrackAvailable( bool ) ) );
            disconnect( d->playlist.data(), SIGNAL( shuffleModeChanged( bool ) ) );
            disconnect( d->playlist.data(), SIGNAL( repeatModeChanged( Tomahawk::PlaylistModes::RepeatMode ) ) );
        }

        d->playlist.data()->reset();
    }

    if ( playlist.isNull() )
    {
        d->playlist.clear();
        emit playlistChanged( playlist );
        return;
    }

    d->playlist = playlist;
    d->stopAfterTrack.clear();

    if ( !d->playlist.isNull() )
    {
        connect( d->playlist.data(), SIGNAL( nextTrackAvailable( bool ) ), SLOT( onPlaylistNextTrackAvailable() ) );

        connect( d->playlist.data(), SIGNAL( previousTrackAvailable( bool ) ), SIGNAL( controlStateChanged() ) );
        connect( d->playlist.data(), SIGNAL( nextTrackAvailable( bool ) ), SIGNAL( controlStateChanged() ) );

        connect( d->playlist.data(), SIGNAL( shuffleModeChanged( bool ) ), SIGNAL( shuffleModeChanged( bool ) ) );
        connect( d->playlist.data(), SIGNAL( repeatModeChanged( Tomahawk::PlaylistModes::RepeatMode ) ), SIGNAL( repeatModeChanged( Tomahawk::PlaylistModes::RepeatMode ) ) );

        emit shuffleModeChanged( d->playlist.data()->shuffled() );
        emit repeatModeChanged( d->playlist.data()->repeatMode() );
    }

    emit playlistChanged( playlist );
}


void
AudioEngine::setRepeatMode( Tomahawk::PlaylistModes::RepeatMode mode )
{
    Q_D( AudioEngine );

    if ( !d->playlist.isNull() )
    {
        d->playlist.data()->setRepeatMode( mode );
    }
}


void
AudioEngine::setShuffled( bool enabled )
{
    Q_D( AudioEngine );

    if ( !d->playlist.isNull() )
    {
        d->playlist.data()->setShuffled( enabled );
    }
}


void
AudioEngine::setStopAfterTrack( const query_ptr& query )
{
    Q_D( AudioEngine );

    if ( d->stopAfterTrack != query )
    {
        d->stopAfterTrack = query;
        emit stopAfterTrackChanged();
    }
}


void
AudioEngine::setCurrentTrack( const Tomahawk::result_ptr& result )
{
    Q_D( AudioEngine );

    if ( !d->currentTrack.isNull() )
    {
        if ( d->state != Error && TomahawkSettings::instance()->privateListeningMode() == TomahawkSettings::PublicListening )
        {
            d->currentTrack->track()->finishPlaying( d->timeElapsed );
        }

        emit finished( d->currentTrack );
    }

    d->currentTrack = result;

    if ( result )
    {
        if ( d->playlist && d->playlist->currentItem() != result )
        {
            d->playlist->setCurrentIndex( d->playlist->indexOfResult( result ) );
        }
    }
}


void
AudioEngine::checkStateQueue()
{
    Q_D( AudioEngine );

    if ( d->stateQueue.count() )
    {
        AudioState state = (AudioState) d->stateQueue.head();
        tDebug( LOGVERBOSE ) << "Applying state command:" << state;
        switch ( state )
        {
            case Playing:
            {
                d->mediaObject->play();
                break;
            }

            case Paused:
            {
                d->mediaObject->pause();
                break;
            }

            default:
                break;
        }
    }
    else
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Queue is empty";
}


void
AudioEngine::queueStateSafety()
{
    Q_D( AudioEngine );

    tDebug( LOGVERBOSE ) << Q_FUNC_INFO;
    d->stateQueue.clear();
}


void
AudioEngine::queueState( AudioState state )
{
    Q_D( AudioEngine );

    if ( d->stateQueueTimer.isActive() )
        d->stateQueueTimer.stop();

    tDebug( LOGVERBOSE ) << "Enqueuing state command:" << state << d->stateQueue.count();
    d->stateQueue.enqueue( state );

    if ( d->stateQueue.count() == 1 )
    {
        checkStateQueue();
    }

    d->stateQueueTimer.start();
}


void
AudioEngine::setState( AudioState state )
{
    Q_D( AudioEngine );

    AudioState oldState = (AudioState) d->state;
    d->state = state;

    emit stateChanged( state, oldState );
}


qint64
AudioEngine::currentTime() const
{
    return d_func()->mediaObject->currentTime();
}


qint64
AudioEngine::currentTrackTotalTime() const
{
    return d_func()->mediaObject->totalTime();
}


unsigned int
AudioEngine::volume() const
{
    return d_func()->audioOutput->volume() * 100.0;
}


AudioEngine::AudioState
AudioEngine::state() const
{
    return (AudioState) d_func()->state;
}


bool
AudioEngine::isPlaying() const
{
    return d_func()->state == Playing;
}


bool
AudioEngine::isPaused() const
{
    return d_func()->state == Paused;
}


bool
AudioEngine::isStopped() const
{
    return d_func()->state == Stopped;
}


playlistinterface_ptr
AudioEngine::currentTrackPlaylist() const
{
    return d_func()->currentTrackPlaylist;
}


playlistinterface_ptr
AudioEngine::playlist() const
{
    return d_func()->playlist;
}


result_ptr AudioEngine::currentTrack() const
{
    return d_func()->currentTrack;
}


query_ptr AudioEngine::stopAfterTrack() const
{
    return d_func()->stopAfterTrack;
}


void
AudioEngine::onVolumeChanged(qreal volume) {
    emit volumeChanged( volume * 100 );
}
