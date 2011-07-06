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

#include "scrobbler.h"

#include <QDir>
#include <QSettings>
#include <QCryptographicHash>

#include "album.h"
#include "typedefs.h"
#include "audio/audioengine.h"
#include "tomahawksettings.h"
#include "infosystem/infosystem.h"

static QString s_scInfoIdentifier = QString( "SCROBBLER" );

Scrobbler::Scrobbler( QObject* parent )
    : QObject( parent )
    , m_reachedScrobblePoint( false )
{
    connect( AudioEngine::instance(), SIGNAL( timerSeconds( unsigned int ) ),
                                        SLOT( engineTick( unsigned int ) ), Qt::QueuedConnection );

    connect( Tomahawk::InfoSystem::InfoSystem::instance(),
        SIGNAL( info( QString, Tomahawk::InfoSystem::InfoType, QVariant, QVariant, QVariantMap ) ),
        SLOT( infoSystemInfo( QString, Tomahawk::InfoSystem::InfoType, QVariant, QVariant, QVariantMap ) ) );

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
//    qDebug() << Q_FUNC_INFO;

    if( m_reachedScrobblePoint )
    {
        m_reachedScrobblePoint = false;
        scrobble();
    }

    Tomahawk::InfoSystem::InfoCriteriaHash trackInfo;

    trackInfo["title"] = track->track();
    trackInfo["artist"] = track->artist()->name();
    trackInfo["album"] = track->album()->name();
    trackInfo["duration"] = QString::number( track->duration() );
    Tomahawk::InfoSystem::InfoSystem::instance()->pushInfo(
        s_scInfoIdentifier, Tomahawk::InfoSystem::InfoSubmitNowPlaying,
        QVariant::fromValue< Tomahawk::InfoSystem::InfoCriteriaHash >( trackInfo ) );

    m_scrobblePoint = ScrobblePoint( track->duration() / 2 );
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

    if( m_reachedScrobblePoint )
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

    Tomahawk::InfoSystem::InfoSystem::instance()->pushInfo(
        s_scInfoIdentifier, Tomahawk::InfoSystem::InfoSubmitScrobble,
        QVariant() );
}


void
Scrobbler::infoSystemInfo( QString caller, Tomahawk::InfoSystem::InfoType type, QVariant input, QVariant output, QVariantMap customData )
{
    Q_UNUSED( type );
    Q_UNUSED( input );
    Q_UNUSED( output );
    Q_UNUSED( customData );
    if ( caller == s_scInfoIdentifier )
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
