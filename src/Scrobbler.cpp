/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include "Scrobbler.h"

#include <QDir>
#include <QSettings>
#include <QCryptographicHash>

#include "Artist.h"
#include "Album.h"
#include "Typedefs.h"
#include "audio/AudioEngine.h"
#include "TomahawkSettings.h"
#include "infosystem/InfoSystem.h"
#include "Source.h"

#include "utils/Logger.h"

static QString s_scInfoIdentifier = QString( "SCROBBLER" );


Scrobbler::Scrobbler( QObject* parent )
    : QObject( parent )
    , m_reachedScrobblePoint( false )
{
    connect( AudioEngine::instance(), SIGNAL( timerSeconds( unsigned int ) ),
                                        SLOT( engineTick( unsigned int ) ), Qt::QueuedConnection );

    connect( Tomahawk::InfoSystem::InfoSystem::instance(),
             SIGNAL( info( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ),
             SLOT( infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ) );

    connect( AudioEngine::instance(), SIGNAL( started( const Tomahawk::result_ptr& ) ),
             SLOT( trackStarted( const Tomahawk::result_ptr& ) ), Qt::QueuedConnection );

    connect( AudioEngine::instance(), SIGNAL( paused() ),
             SLOT( trackPaused() ), Qt::QueuedConnection );

    connect( AudioEngine::instance(), SIGNAL( resumed() ),
             SLOT( trackResumed() ), Qt::QueuedConnection );

    connect( AudioEngine::instance(), SIGNAL( stopped() ),
             SLOT( trackStopped() ), Qt::QueuedConnection );

    connect( Tomahawk::InfoSystem::InfoSystem::instance(), SIGNAL( finished( QString ) ), SLOT( infoSystemFinished( QString ) ) );
}


Scrobbler::~Scrobbler()
{
}


void
Scrobbler::trackStarted( const Tomahawk::result_ptr& track )
{
    Q_ASSERT( QThread::currentThread() == thread() );

    if ( m_reachedScrobblePoint )
    {
        m_reachedScrobblePoint = false;
        scrobble();
    }

    Tomahawk::InfoSystem::InfoStringHash trackInfo;
    trackInfo["title"] = track->track();
    trackInfo["artist"] = track->artist()->name();
    trackInfo["album"] = track->album()->name();
    trackInfo["duration"] = QString::number( track->duration() );
    trackInfo["albumpos"] = QString::number( track->albumpos() );

    QVariantMap playInfo;
    playInfo["trackinfo"] = QVariant::fromValue< Tomahawk::InfoSystem::InfoStringHash >( trackInfo );
    playInfo["private"] = TomahawkSettings::instance()->privateListeningMode();

    Tomahawk::InfoSystem::InfoPushData pushData (
        s_scInfoIdentifier, Tomahawk::InfoSystem::InfoSubmitNowPlaying,
        playInfo,
        Tomahawk::InfoSystem::PushNoFlag );

    Tomahawk::InfoSystem::InfoSystem::instance()->pushInfo( pushData );

    // liblastfm forces 0-length tracks to scrobble after 4 minutes, stupid.
    if ( track->duration() == 0 )
        m_scrobblePoint = lastfm::ScrobblePoint( 30 );
    else
        m_scrobblePoint = lastfm::ScrobblePoint( track->duration() / 2 );
}


void
Scrobbler::trackPaused()
{
    Q_ASSERT( QThread::currentThread() == thread() );
}


void
Scrobbler::trackResumed()
{
    Q_ASSERT( QThread::currentThread() == thread() );
}


void
Scrobbler::trackStopped()
{
    Q_ASSERT( QThread::currentThread() == thread() );

    if ( m_reachedScrobblePoint )
    {
        m_reachedScrobblePoint = false;
        scrobble();
    }
}


void
Scrobbler::engineTick( unsigned int secondsElapsed )
{
    if ( secondsElapsed > m_scrobblePoint )
        m_reachedScrobblePoint = true;
}


void
Scrobbler::scrobble()
{
    Q_ASSERT( QThread::currentThread() == thread() );

    Tomahawk::InfoSystem::InfoPushData pushData (
        s_scInfoIdentifier, Tomahawk::InfoSystem::InfoSubmitScrobble,
        QVariant(), Tomahawk::InfoSystem::PushNoFlag );

    Tomahawk::InfoSystem::InfoSystem::instance()->pushInfo( pushData );
}


void
Scrobbler::infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData requestData, QVariant output )
{
    Q_UNUSED( output );
    if ( requestData.caller == s_scInfoIdentifier )
        qDebug() << Q_FUNC_INFO;
}


void
Scrobbler::infoSystemFinished( QString target )
{
    if ( target == s_scInfoIdentifier )
    {
        qDebug() << Q_FUNC_INFO;
        qDebug() << "Scrobbler received done signal from InfoSystem";
    }
}
