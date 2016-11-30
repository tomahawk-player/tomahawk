/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2016, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "audio/Qnr_IoDeviceStream.h"
#include "filemetadata/MusicScanner.h"
#include "jobview/JobStatusView.h"
#include "jobview/JobStatusModel.h"
#include "jobview/ErrorStatusMessage.h"
#include "network/Servent.h"
#include "playlist/SingleTrackPlaylistInterface.h"
#include "utils/Closure.h"
#include "utils/Logger.h"
#include "utils/NetworkReply.h"
#include "utils/NetworkAccessManager.h"

#include "Album.h"
#include "Artist.h"
#include "Pipeline.h"
#include "PlaylistEntry.h"
#include "SourceList.h"
#include "TomahawkSettings.h"
#include "UrlHandler.h"
#include "resolvers/ScriptJob.h"

#include <QDir>

using namespace Tomahawk;

#define AUDIO_VOLUME_STEP 5

static const uint_fast8_t UNDERRUNTHRESHOLD = 2;

static QString s_aeInfoIdentifier = QString( "AUDIOENGINE" );


void
AudioEnginePrivate::onStateChanged( AudioOutput::AudioState newState, AudioOutput::AudioState oldState )
{
    tDebug() << Q_FUNC_INFO << oldState << newState << q_ptr->state();
    AudioEngine::AudioState previousState = q_ptr->state();

    if ( newState == AudioOutput::Loading )
    {
        // We don't emit this state to listeners - yet.
        state = AudioEngine::Loading;
    }
    else if ( newState == AudioOutput::Buffering )
    {
        if ( underrunCount > UNDERRUNTHRESHOLD && !underrunNotified )
        {
            underrunNotified = true;
            //FIXME: Actually notify
        }
        else
            underrunCount++;
    }
    else if ( newState == AudioOutput::Error )
    {
        q_ptr->setState( AudioEngine::Stopped );
        tDebug() << Q_FUNC_INFO << "AudioOutput Error";
        emit q_ptr->error( AudioEngine::UnknownError );
        q_ptr->setState( AudioEngine::Error );
    }
    else if ( newState == AudioOutput::Playing )
    {
        bool emitSignal = false;
        if ( q_ptr->state() != AudioEngine::Paused && q_ptr->state() != AudioEngine::Playing )
        {
            underrunCount = 0;
            underrunNotified = false;
            emitSignal = true;
        }
        q_ptr->setState( AudioEngine::Playing );
        audioRetryCounter = 0;

        if ( emitSignal )
            emit q_ptr->started( currentTrack );
    }
    else if ( newState == AudioOutput::Paused )
    {
        q_ptr->setState( AudioEngine::Paused );
    }
    else if ( newState == AudioOutput::Stopped )
    {
        q_ptr->setState( AudioEngine::Stopped );
    }

    if ( previousState != AudioEngine::Stopped &&
         ( oldState == AudioOutput::Playing || oldState == AudioOutput::Loading ) )
    {
        bool retry = false;
        if ( newState == AudioOutput::Error )
        {
            retry = ( audioRetryCounter < 2 );
            audioRetryCounter++;

            if ( !retry )
            {
                q_ptr->stop( AudioEngine::UnknownError );
            }
        }

        if ( newState == AudioOutput::Stopped || retry )
        {
            tDebug() << Q_FUNC_INFO << "Finding next track." << oldState << newState;
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
    d->waitingOnNewTrack = false;
    d->state = Stopped;
    d->coverTempFile = 0;

    d->s_instance = this;
    tDebug() << "Init AudioEngine";

    d->audioOutput = new AudioOutput( this );

    connect( d->audioOutput, SIGNAL( initialized() ), this, SIGNAL( initialized() ) );
    connect( d->audioOutput, SIGNAL( stateChanged( AudioOutput::AudioState, AudioOutput::AudioState ) ), d_func(), SLOT( onStateChanged( AudioOutput::AudioState, AudioOutput::AudioState ) ) );
    connect( d->audioOutput, SIGNAL( tick( qint64 ) ), SLOT( timerTriggered( qint64 ) ) );
    connect( d->audioOutput, SIGNAL( positionChanged( float ) ), SLOT( onPositionChanged( float ) ) );
    connect( d->audioOutput, SIGNAL( volumeChanged( qreal ) ), SLOT( onVolumeChanged( qreal ) ) );
    connect( d->audioOutput, SIGNAL( mutedChanged( bool ) ), SIGNAL( mutedChanged( bool ) ) );

    if ( TomahawkSettings::instance()->muted() )
    {
        mute();
    }
    setVolume( TomahawkSettings::instance()->volume() );

    qRegisterMetaType< AudioErrorCode >("AudioErrorCode");
    qRegisterMetaType< AudioState >("AudioState");
}


AudioEngine::~AudioEngine()
{
    tDebug() << Q_FUNC_INFO;

    TomahawkSettings::instance()->setVolume( volume() );
    TomahawkSettings::instance()->setMuted( isMuted() );

    delete d_ptr;
}


void
AudioEngine::playPause()
{
    if ( QThread::currentThread() != thread() )
    {
        QMetaObject::invokeMethod( this, "playPause", Qt::QueuedConnection );
        return;
    }

    if ( isPlaying() )
        pause();
    else
        play();
}


void
AudioEngine::play()
{
    if ( QThread::currentThread() != thread() )
    {
        QMetaObject::invokeMethod( this, "play", Qt::QueuedConnection );
        return;
    }

    Q_D( AudioEngine );

    tDebug( LOGEXTRA ) << Q_FUNC_INFO;

    if ( isPaused() )
    {
        d->audioOutput->play();
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
    if ( QThread::currentThread() != thread() )
    {
        QMetaObject::invokeMethod( this, "pause", Qt::QueuedConnection );
        return;
    }

    Q_D( AudioEngine );

    tDebug( LOGEXTRA ) << Q_FUNC_INFO;

    d->audioOutput->pause();
    emit paused();

    Tomahawk::InfoSystem::InfoSystem::instance()->pushInfo( Tomahawk::InfoSystem::InfoPushData( s_aeInfoIdentifier, Tomahawk::InfoSystem::InfoNowPaused, QVariant(), Tomahawk::InfoSystem::PushNoFlag ) );
}


void
AudioEngine::stop( AudioErrorCode errorCode )
{
    if ( QThread::currentThread() != thread() )
    {
        QMetaObject::invokeMethod( this, "stop", Qt::QueuedConnection );
        return;
    }

    Q_D( AudioEngine );

    tDebug() << Q_FUNC_INFO << errorCode << isStopped();

    if ( errorCode == NoError )
        setState( Stopped );
    else
        setState( Error );

    if ( d->audioOutput->state() != AudioOutput::Stopped )
        d->audioOutput->stop();

    emit stopped();

    if ( !d->playlist.isNull() )
        d->playlist.data()->reset();
    if ( !d->currentTrack.isNull() )
        emit timerPercentage( ( (double)d->timeElapsed / (double)d->currentTrack->track()->duration() ) * 100.0 );

    setCurrentTrack( Tomahawk::result_ptr() );

    if ( d->waitingOnNewTrack )
        sendWaitingNotification();

    if ( d->audioOutput->isInitialized() )
    {
        Tomahawk::InfoSystem::InfoPushData pushData( s_aeInfoIdentifier, Tomahawk::InfoSystem::InfoNowStopped, QVariant(), Tomahawk::InfoSystem::PushNoFlag );
        Tomahawk::InfoSystem::InfoSystem::instance()->pushInfo( pushData );
    }
}


void
AudioEngine::previous()
{
    if ( QThread::currentThread() != thread() )
    {
        QMetaObject::invokeMethod( this, "previous", Qt::QueuedConnection );
        return;
    }

    tDebug( LOGEXTRA ) << Q_FUNC_INFO;

    if ( canGoPrevious() )
        loadPreviousTrack();
}


void
AudioEngine::next()
{
    if ( QThread::currentThread() != thread() )
    {
        QMetaObject::invokeMethod( this, "next", Qt::QueuedConnection );
        return;
    }

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

    return !d->playlist.isNull() && ( d->playlist.data()->seekRestrictions() != PlaylistModes::NoSeek );
}


void
AudioEngine::seek( qint64 ms )
{
    Q_D( AudioEngine );

    /*if ( !canSeek() )
    {
        tDebug( LOGEXTRA ) << "Could not seek!";
        return;
    }*/

    if ( isPlaying() || isPaused() )
    {
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << ms;
        d->audioOutput->seek( ms );
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

    tDebug() << Q_FUNC_INFO << percentage;

    percentage = qBound( 0, percentage, 100 );
    d->audioOutput->setVolume( (qreal)percentage / 100.0 );

    if ( percentage > 0 && d->audioOutput->isMuted() )
        d->audioOutput->setMuted( false );

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


bool
AudioEngine::isMuted() const
{
    return d_func()->audioOutput->isMuted();
}


void
AudioEngine::mute()
{
    Q_D( AudioEngine );
    d->audioOutput->setMuted( true );

    emit volumeChanged( volume() );
}


void
AudioEngine::toggleMute()
{
    Q_D( AudioEngine );
    d->audioOutput->setMuted( !d->audioOutput->isMuted() );

    emit volumeChanged( volume() );
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

    if ( d->currentTrack->track()->coverLoaded() )
    {
        onNowPlayingInfoReady( type );
    }
    else
    {
        NewClosure( d->currentTrack->track().data(), SIGNAL( coverChanged() ), const_cast< AudioEngine* >( this ), SLOT( sendNowPlayingNotification( const Tomahawk::InfoSystem::InfoType ) ), type );
        d->currentTrack->track()->cover( QSize( 0, 0 ), true );
    }
}


void
AudioEngine::onNowPlayingInfoReady( const Tomahawk::InfoSystem::InfoType type )
{
    Q_D( AudioEngine );

    if ( d->currentTrack.isNull() ||
         d->currentTrack->track()->artist().isEmpty() )
        return;

    QVariantMap playInfo;

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

    Tomahawk::InfoSystem::InfoStringHash trackInfo;
    trackInfo["title"] = d->currentTrack->track()->track();
    trackInfo["artist"] = d->currentTrack->track()->artist();
    trackInfo["album"] = d->currentTrack->track()->album();
    trackInfo["duration"] = QString::number( d->currentTrack->track()->duration() );
    trackInfo["albumpos"] = QString::number( d->currentTrack->track()->albumpos() );

    playInfo["trackinfo"] = QVariant::fromValue< Tomahawk::InfoSystem::InfoStringHash >( trackInfo );
    playInfo["private"] = TomahawkSettings::instance()->privateListeningMode();

    Tomahawk::InfoSystem::InfoPushData pushData( s_aeInfoIdentifier, type, playInfo, Tomahawk::InfoSystem::PushShortUrlFlag );
    Tomahawk::InfoSystem::InfoSystem::instance()->pushInfo( pushData );
}


void
AudioEngine::loadTrack( const Tomahawk::result_ptr& result, bool preload )
{
    Q_D( AudioEngine );
    tDebug( LOGEXTRA ) << Q_FUNC_INFO << ( result.isNull() ? QString() : result->url() ) << " preload:" << preload;


    if ( !d->audioOutput->isInitialized() )
    {
        return;
    }

    if ( !result )
    {
        stop();
        return;
    }

    if (preload && d->preloadedTrack == result)
        return;

    if (preload) 
    tDebug( LOGEXTRA ) << Q_FUNC_INFO << "not preloaded yet, preloading";

    if (preload) 
    {
        setPreloadTrack( result );
    }
    else
    {
        // We do this to stop the audio as soon as a user activated another track
        // If we don't block the audioOutput signals, the state change will trigger
        // loading yet another track
        d->audioOutput->blockSignals( true );
        d->audioOutput->stop();
        d->audioOutput->blockSignals( false );

        setCurrentTrack( result );
        if ( result == d->preloadedTrack ) 
        {
            setPreloadTrack( Tomahawk::result_ptr(nullptr) );
            d->state = Loading;
            emit loading( d->currentTrack );
            d->audioOutput->switchToPreloadedMedia();
            if ( !d->input.isNull() )
            {
                d->input->close();
                d->input.clear();
            }
            d->input = d->inputPreloaded;

            d->audioOutput->play();

            if ( TomahawkSettings::instance()->privateListeningMode() != TomahawkSettings::FullyPrivate )
            {
                d->currentTrack->track()->startPlaying();
            }

            sendNowPlayingNotification( Tomahawk::InfoSystem::InfoNowPlaying );
            return;
        }
        setPreloadTrack( Tomahawk::result_ptr(nullptr) );
    }

    ScriptJob* job = result->resolvedBy()->getStreamUrl( result );
    connect( job, SIGNAL( done( QVariantMap ) ), SLOT( gotStreamUrl( QVariantMap ) ) );
    job->setProperty( "result", QVariant::fromValue( result ) );
    job->setProperty( "isPreload", QVariant::fromValue(preload) );
    tDebug() << "preload:" << preload << ", for result:" << result;
    job->start();
}


void
AudioEngine::gotStreamUrl( const QVariantMap& data )
{
    QString streamUrl = data[ "url" ].toString();
    QVariantMap headers = data[ "headers" ].toMap();
    Tomahawk::result_ptr result = sender()->property( "result" ).value<result_ptr>();
    bool isPreload = sender()->property( "isPreload" ).value<bool>();

    tDebug() << Q_FUNC_INFO << " is preload:" << isPreload << ", for result:" << result;

    if ( streamUrl.isEmpty() || headers.isEmpty() ||
         !( TomahawkUtils::isHttpResult( streamUrl ) || TomahawkUtils::isHttpsResult( streamUrl ) ) )
    {
        // We can't supply custom headers to VLC - but prefer using its HTTP streaming due to improved seeking ability
        // Not an RTMP or HTTP-with-headers URL, get IO device
        QSharedPointer< QIODevice > sp;
        performLoadIODevice( result, streamUrl, isPreload );
    }
    else
    {
        // We need our own QIODevice for streaming
        // TODO: just make this part of the http(s) IoDeviceFactory (?)
        QUrl url = QUrl::fromEncoded( streamUrl.toUtf8() );
        QNetworkRequest req( url );

        QMap<QString, QString> parsedHeaders;
        foreach ( const QString& key, headers.keys() )
        {
            Q_ASSERT_X( headers[key].canConvert( QVariant::String ), Q_FUNC_INFO, "Expected a Map of string for additional headers" );
            if ( headers[key].canConvert( QVariant::String ) )
            {
                parsedHeaders.insert( key, headers[key].toString() );
            }
        }

        foreach ( const QString& key, parsedHeaders.keys() )
        {
            req.setRawHeader( key.toLatin1(), parsedHeaders[key].toLatin1() );
        }

        tDebug() << "Creating a QNetworkReply with url:" << req.url().toString();
        NetworkReply* reply = new NetworkReply( Tomahawk::Utils::nam()->get( req ) );
        NewClosure( reply, SIGNAL( finalUrlReached() ), this, SLOT( gotRedirectedStreamUrl( Tomahawk::result_ptr, NetworkReply* ) ), result, reply );
    }

    sender()->deleteLater();
}


void
AudioEngine::gotRedirectedStreamUrl( const Tomahawk::result_ptr& result, NetworkReply* reply )
{
    Q_D( AudioEngine );
    // std::functions cannot accept temporaries as parameters
    QSharedPointer< QIODevice > sp ( reply->reply(), &QObject::deleteLater );
    QString url = reply->reply()->url().toString();
    reply->disconnectFromReply();
    reply->deleteLater();

    bool isPreload = result == d->preloadedTrack;
    tDebug() << Q_FUNC_INFO << " is preload:" << isPreload;

    performLoadTrack( result, url, sp, isPreload );
}


void
AudioEngine::onPositionChanged( float new_position )
{
    if ( new_position >= 0.90 )
        loadNextTrack(true);
//    tDebug() << Q_FUNC_INFO << new_position << state();
    emit trackPosition( new_position );
}


void
AudioEngine::performLoadIODevice( const result_ptr& result, const QString& url, bool preload )
{
    tDebug( LOGEXTRA ) << Q_FUNC_INFO << ( result.isNull() ? QString() : url );

    if ( !TomahawkUtils::isLocalResult( url ) && !TomahawkUtils::isHttpResult( url )
         && !TomahawkUtils::isRtmpResult( url ) )
    {
        std::function< void ( const QString, QSharedPointer< QIODevice > ) > callback =
                std::bind( &AudioEngine::performLoadTrack, this, result,
                           std::placeholders::_1,
                           std::placeholders::_2,
                           preload );
        Tomahawk::UrlHandler::getIODeviceForUrl( result, url, callback );
    }
    else
    {
        QSharedPointer< QIODevice > io;
        performLoadTrack( result, url, io, preload );
    }
}


void
AudioEngine::performLoadTrack( const Tomahawk::result_ptr result, const QString& url, QSharedPointer< QIODevice > io, bool preload )
{
    if ( QThread::currentThread() != thread() )
    {
        QMetaObject::invokeMethod( this, "performLoadTrack", Qt::QueuedConnection,
                                   Q_ARG( const Tomahawk::result_ptr, result ),
                                   Q_ARG( const QString, url ),
                                   Q_ARG( QSharedPointer< QIODevice >, io ),
                                   Q_ARG( bool, preload )
                                   );
        return;
    }

    Q_D( AudioEngine );
    if ( !preload && currentTrack() != result )
    {
        tLog( LOGVERBOSE ) << Q_FUNC_INFO << "Track loaded too late, skip.";
        return;
    }
    tDebug( LOGEXTRA ) << Q_FUNC_INFO << ( result.isNull() ? QString() : result->url() ) << preload;
    QSharedPointer< QIODevice > ioToKeep = io;

    bool err = false;
    {
        if ( !( TomahawkUtils::isLocalResult( url ) || TomahawkUtils::isHttpResult( url ) || TomahawkUtils::isRtmpResult( url )  )
             && ( !io || io.isNull() ) )
        {
            tLog() << Q_FUNC_INFO << "Error getting iodevice for" << result->url();
            err = true;
        }

        if ( !err )
        {
            if (preload)
            {
                tLog() << Q_FUNC_INFO << "Preloading new song:" << url;
            }
            else
            {
                tLog() << Q_FUNC_INFO << "Starting new song:" << url;
                d->state = Loading;
                emit loading( d->currentTrack );
            }

            if ( !TomahawkUtils::isLocalResult( url )
                 && !( TomahawkUtils::isHttpResult( url ) && io.isNull() )
                 && !TomahawkUtils::isRtmpResult( url ) )
            {
                QSharedPointer<QNetworkReply> qnr = io.objectCast<QNetworkReply>();
                if ( !qnr.isNull() )
                {
                    d->audioOutput->setCurrentSource( new QNR_IODeviceStream( qnr, this ), preload );
                    // We keep track of the QNetworkReply in QNR_IODeviceStream
                    // and AudioOutput handles the deletion of the
                    // QNR_IODeviceStream object
                    ioToKeep.clear();
                    d->audioOutput->setAutoDelete( true, preload );
                }
                else
                {
                    d->audioOutput->setCurrentSource( io.data(), preload);
                    // We handle the deletion via tracking in d->input
                    d->audioOutput->setAutoDelete( false, preload);
                }
            }
            else
            {
                /*
                 * TODO: Do we need this anymore as we now do HTTP streaming ourselves?
                 * Maybe this can be useful for letting VLC do other protocols?
                 */
                if ( !TomahawkUtils::isLocalResult( url ) )
                {
                    QUrl furl = url;
                    if ( url.contains( "?" ) )
                    {
                        furl = QUrl( url.left( url.indexOf( '?' ) ) );
                        TomahawkUtils::urlSetQuery( furl, QString( url.mid( url.indexOf( '?' ) + 1 ) ) );
                    }

                    tLog( LOGVERBOSE ) << Q_FUNC_INFO << "Passing to VLC:" << furl;
                    d->audioOutput->setCurrentSource( furl, preload );
                }
                else
                {
                    QString furl = url;
                    if ( furl.startsWith( "file://" ) )
                        furl = furl.right( furl.length() - 7 );

                    tLog( LOGVERBOSE ) << Q_FUNC_INFO << "Passing to VLC:" << QUrl::fromLocalFile( furl );
                    d->audioOutput->setCurrentSource( QUrl::fromLocalFile( furl ), preload );
                }

                d->audioOutput->setAutoDelete( true, preload );
            }

            if ( preload ) {
                if ( !d->inputPreloaded.isNull() )
                {
                    d->inputPreloaded->close();
                    d->inputPreloaded.clear();
                }
                d->inputPreloaded = ioToKeep;
            }
            else
            {
                if ( !d->input.isNull() )
                {
                    d->input->close();
                    d->input.clear();
                }
                d->input = ioToKeep;
                d->audioOutput->play();

                if ( TomahawkSettings::instance()->privateListeningMode() != TomahawkSettings::FullyPrivate )
                {
                    d->currentTrack->track()->startPlaying();
                }

                sendNowPlayingNotification( Tomahawk::InfoSystem::InfoNowPlaying );
            }
        }
    }

    if ( err )
    {
        stop();
        return;
    }

    if ( !preload )
    {
        d->waitingOnNewTrack = false;
    }
    return;
}


void
AudioEngine::loadPreviousTrack()
{
    if ( QThread::currentThread() != thread() )
    {
        QMetaObject::invokeMethod( this, "loadPreviousTrack", Qt::QueuedConnection );
        return;
    }

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
        setCurrentTrackPlaylist( d->playlist );
    }

    if ( result )
        loadTrack( result, false );
    else
        stop();
}


void
AudioEngine::loadNextTrack( bool preload )
{
    if ( QThread::currentThread() != thread() )
    {
        QMetaObject::invokeMethod( this, "loadNextTrack", Qt::QueuedConnection,
                Q_ARG( bool, preload ));
        return;
    }

    Q_D( AudioEngine );

    tDebug( LOGEXTRA ) << Q_FUNC_INFO;

    Tomahawk::result_ptr result;

    if ( d->stopAfterTrack && d->currentTrack )
    {
        if ( d->stopAfterTrack->track()->equals( d->currentTrack->track() ) )
        {
            if ( !preload )
            {
                d->stopAfterTrack.clear();
                stop();
            }
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
            if ( preload )
            {
                result = d->playlist.data()->nextResult();
            }
            else
            {
                result = d->playlist.data()->setSiblingResult( 1 );
                setCurrentTrackPlaylist( d->playlist );
            }
        }
    }

    if ( result )
    {
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Got next item, loading track, preload:" << preload;
        loadTrack( result, preload );
    }
    else if ( !preload )
    {
        if ( !d->playlist.isNull() && d->playlist.data()->retryMode() == Tomahawk::PlaylistModes::Retry )
            d->waitingOnNewTrack = true;

        stop();
    }
}


void
AudioEngine::play( const QUrl& url )
{
    tDebug() << Q_FUNC_INFO << url;

    const QVariantMap tags = MusicScanner::readTags( QFileInfo( url.toLocalFile() ) ).toMap();

    track_ptr t;
    if ( !tags.isEmpty() )
    {
        t = Track::get( tags["artist"].toString(), tags["track"].toString(), tags["album"].toString(),
                        tags["albumArtist"].toString(), tags["duration"].toInt(), tags["composer"].toString(),
                        tags["albumpos"].toUInt(), tags["discnumber"].toUInt() );
    }
    else
    {
        t = Tomahawk::Track::get( "Unknown Artist", "Unknown Track" );
    }

    result_ptr result = Result::get( url.toString(), t );

    if ( !tags.isEmpty() )
    {
        result->setSize( tags["size"].toUInt() );
        result->setBitrate( tags["bitrate"].toUInt() );
        result->setModificationTime( tags["mtime"].toUInt() );
        result->setMimetype( tags["mimetype"].toString() );
    }

    result->setResolvedByCollection( SourceList::instance()->getLocal()->collections().first(), false );

    //    Tomahawk::query_ptr qry = Tomahawk::Query::get( t );
    playItem( playlistinterface_ptr(), result, query_ptr() );
}


void
AudioEngine::playItem( Tomahawk::playlistinterface_ptr playlist, const Tomahawk::result_ptr& result, const Tomahawk::query_ptr& fromQuery )
{
    Q_D( AudioEngine );

    tDebug( LOGEXTRA ) << Q_FUNC_INFO << ( result.isNull() ? QString() : result->url() );

    if ( !d->playlist.isNull() )
        d->playlist.data()->reset();

    setPlaylist( playlist );

    if ( !playlist && fromQuery )
    {
        setCurrentTrackPlaylist( playlistinterface_ptr( new SingleTrackPlaylistInterface( fromQuery ) ) );
    }
    else
    {
        setCurrentTrackPlaylist( playlist );
    }

    if ( result )
    {
        loadTrack( result, false );
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
    if ( query->resolvingFinished() || query->numResults( true ) )
    {
        if ( query->numResults( true ) )
        {
            playItem( playlist, query->results().first(), query );
            return;
        }

        JobStatusView::instance()->model()->addJob(
            new ErrorStatusMessage( tr( "Sorry, %applicationName couldn't find the track '%1' by %2" ).arg( query->queryTrack()->track() ).arg( query->queryTrack()->artist() ), 15 ) );

        if ( isStopped() )
            emit stopped(); // we do this so the original caller knows we couldn't find this track
    }
    else
    {
        Pipeline::instance()->resolve( query );

        NewClosure( query.data(), SIGNAL( resultsChanged() ),
                    const_cast<AudioEngine*>(this), SLOT( playItem( Tomahawk::playlistinterface_ptr, Tomahawk::query_ptr ) ), playlist, query );
    }
}


void
AudioEngine::playItem( const Tomahawk::artist_ptr& artist )
{
    playlistinterface_ptr pli = artist->playlistInterface( Mixed );
    if ( pli->isFinished() )
    {
        if ( pli->tracks().isEmpty() )
        {
            JobStatusView::instance()->model()->addJob(
                new ErrorStatusMessage( tr( "Sorry, %applicationName couldn't find the artist '%1'" ).arg( artist->name() ), 15 ) );

            if ( isStopped() )
                emit stopped(); // we do this so the original caller knows we couldn't find this track
        }
        else
            playPlaylistInterface( pli );
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
        if ( pli->tracks().isEmpty() )
        {
            JobStatusView::instance()->model()->addJob(
                new ErrorStatusMessage( tr( "Sorry, %applicationName couldn't find the album '%1' by %2" ).arg( album->name() ).arg( album->artist()->name() ), 15 ) );

            if ( isStopped() )
                emit stopped(); // we do this so the original caller knows we couldn't find this track
        }
        else
            playPlaylistInterface( pli );
    }
    else
    {
        NewClosure( album.data(), SIGNAL( tracksAdded( QList<Tomahawk::query_ptr>, Tomahawk::ModelMode, Tomahawk::collection_ptr ) ),
                    const_cast<AudioEngine*>(this), SLOT( playItem( Tomahawk::album_ptr ) ), album );
        pli->tracks();
    }
}


void
AudioEngine::playPlaylistInterface( const Tomahawk::playlistinterface_ptr& playlist )
{
    if ( !playlist->hasFirstPlayableTrack() )
    {
        NewClosure( playlist.data(), SIGNAL( foundFirstPlayableTrack() ),
                    const_cast<AudioEngine*>(this), SLOT( playPlaylistInterface( Tomahawk::playlistinterface_ptr ) ), playlist );
        return;
    }

    foreach ( const Tomahawk::query_ptr& query, playlist->tracks() )
    {
        if ( query->playable() )
        {
            playItem( playlist, query );
            return;
        }
    }

    // No playable track found
    JobStatusView::instance()->model()->addJob( new ErrorStatusMessage( tr( "Sorry, couldn't find any playable tracks" ), 15 ) );
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
AudioEngine::setPreloadTrack( const Tomahawk::result_ptr& result )
{
    Q_D( AudioEngine );

    d->preloadedTrack = result;
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
    return d_func()->audioOutput->currentTime();
}


qint64
AudioEngine::currentTrackTotalTime() const
{
    Q_D( const AudioEngine );

    // FIXME : This is too hacky. The problem is that I don't know why
    //         libVLC doesn't report total duration for stream data (imem://)
    // But it's not a real problem for playback, since EndOfStream is emitted by libVLC itself
    // This value is only used by AudioOutput to evaluate if it's close to end of stream
    if ( d->audioOutput->totalTime() <= 0 && d->currentTrack && d->currentTrack->track() ) {
        return d->currentTrack->track()->duration() * 1000 + 1000;
    }
    return d->audioOutput->totalTime();
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


result_ptr
AudioEngine::currentTrack() const
{
    return d_func()->currentTrack;
}


query_ptr
AudioEngine::stopAfterTrack() const
{
    return d_func()->stopAfterTrack;
}


void
AudioEngine::onVolumeChanged( qreal volume )
{
    emit volumeChanged( volume * 100 );
}


void
AudioEngine::setCurrentTrackPlaylist( const playlistinterface_ptr& playlist )
{
    Q_D( AudioEngine );

    if ( d->currentTrackPlaylist != playlist )
    {
        d->currentTrackPlaylist = playlist;
        emit currentTrackPlaylistChanged( d->currentTrackPlaylist );
    }
}


void
AudioEngine::setDspCallback( std::function< void( int state, int frameNumber, float* samples, int nb_channels, int nb_samples ) > cb )
{
    Q_D( AudioEngine );

    d->audioOutput->setDspCallback( cb );
}
