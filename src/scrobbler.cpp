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
#include "tomahawk/tomahawkapp.h"
#include "tomahawk/infosystem.h"

static QString s_infoIdentifier = QString("SCROBBLER");

Scrobbler::Scrobbler( QObject* parent )
    : QObject( parent )
    , m_reachedScrobblePoint( false )
{
    connect( AudioEngine::instance(), SIGNAL( timerSeconds( unsigned int ) ),
                                        SLOT( engineTick( unsigned int ) ), Qt::QueuedConnection );
    
    connect( TomahawkApp::instance()->infoSystem(),
        SIGNAL( info( QString, Tomahawk::InfoSystem::InfoType, QVariant, QVariant, Tomahawk::InfoSystem::InfoCustomData ) ),
        SLOT( infoSystemInfo( QString, Tomahawk::InfoSystem::InfoType, QVariant, QVariant, Tomahawk::InfoSystem::InfoCustomData ) ) );
    
    connect( TomahawkApp::instance()->infoSystem(), SIGNAL( finished( QString ) ), SLOT( infoSystemFinished( QString ) ) );

    connect( AudioEngine::instance(), SIGNAL( started( const Tomahawk::result_ptr& ) ),
             SLOT( trackStarted( const Tomahawk::result_ptr& ) ), Qt::QueuedConnection );

    connect( AudioEngine::instance(), SIGNAL( paused() ),
             SLOT( trackPaused() ), Qt::QueuedConnection );

    connect( AudioEngine::instance(), SIGNAL( resumed() ),
             SLOT( trackResumed() ), Qt::QueuedConnection );

    connect( AudioEngine::instance(), SIGNAL( stopped() ),
             SLOT( trackStopped() ), Qt::QueuedConnection );
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

    Tomahawk::InfoSystem::InfoCustomData trackInfo;
    
    trackInfo["title"] = QVariant::fromValue< QString >( track->track() );
    trackInfo["artist"] = QVariant::fromValue< QString >( track->artist()->name() );
    trackInfo["album"] = QVariant::fromValue< QString >( track->album()->name() );
    trackInfo["duration"] = QVariant::fromValue< uint >( track->duration() );
    TomahawkApp::instance()->infoSystem()->getInfo(
        s_infoIdentifier, Tomahawk::InfoSystem::InfoMiscSubmitNowPlaying,
        QVariant::fromValue< Tomahawk::InfoSystem::InfoCustomData >( trackInfo ), Tomahawk::InfoSystem::InfoCustomData() );
    
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
    
    TomahawkApp::instance()->infoSystem()->getInfo(
        s_infoIdentifier, Tomahawk::InfoSystem::InfoMiscSubmitScrobble,
        QVariant(), Tomahawk::InfoSystem::InfoCustomData() );
}

void
Scrobbler::infoSystemInfo( QString caller, Tomahawk::InfoSystem::InfoType type, QVariant input, QVariant output, Tomahawk::InfoSystem::InfoCustomData customData )
{
    if ( caller == s_infoIdentifier )
    {
        qDebug() << Q_FUNC_INFO;
        if ( type == Tomahawk::InfoSystem::InfoMiscSubmitNowPlaying )
            qDebug() << "Scrobbler received now playing response from InfoSystem";
        else if ( type == Tomahawk::InfoSystem::InfoMiscSubmitScrobble )
            qDebug() << "Scrobbler received scrobble response from InfoSystem";
    }
}

void 
Scrobbler::infoSystemFinished( QString target )
{
    if ( target == s_infoIdentifier )
    {
        qDebug() << Q_FUNC_INFO;
        qDebug() << "Scrobbler received done signal from InfoSystem";
    }
}
