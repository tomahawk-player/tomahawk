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

#include "tomahawktrayicon.h"

#include <QWheelEvent>

#include "artist.h"

#include "audio/audioengine.h"
#include "tomahawkapp.h"
#include "tomahawkwindow.h"

#include "utils/logger.h"
#include <actioncollection.h>


TomahawkTrayIcon::TomahawkTrayIcon( QObject* parent )
    : QSystemTrayIcon( parent )
    , m_currentAnimationFrame( 0 )
    , m_showWindowAction( 0 )
{
    QIcon icon( RESPATH "icons/tomahawk-icon-128x128.png" );
    setIcon( icon );

    refreshToolTip();

    m_contextMenu = new QMenu();
    setContextMenu( m_contextMenu );

    m_playPauseAction = m_contextMenu->addAction( tr( "Play" ) );
    //m_pauseAction = m_contextMenu->addAction( tr( "Pause" ) );
    m_stopAction = m_contextMenu->addAction( tr( "Stop" ) );
    m_contextMenu->addSeparator();
    m_prevAction = m_contextMenu->addAction( tr( "Previous Track" ) );
    m_nextAction = m_contextMenu->addAction( tr( "Next Track" ) );
    m_contextMenu->addSeparator();
    m_contextMenu->addAction( ActionCollection::instance()->getAction( "togglePrivacy" ) );

#ifdef Q_WS_MAC
    // On mac you can close the windows while leaving the app open. We then need a way to show the main window again
    m_contextMenu->addSeparator();
    m_showWindowAction = m_contextMenu->addAction( tr( "Hide Tomahawk Window" ) );
    m_showWindowAction->setData( true );
    connect( m_showWindowAction, SIGNAL( triggered() ), this, SLOT( showWindow() ) );
#endif

    m_contextMenu->addSeparator();
    m_quitAction = m_contextMenu->addAction( tr( "Quit" ) );

    connect( AudioEngine::instance(), SIGNAL( loading( Tomahawk::result_ptr ) ), SLOT( setResult( Tomahawk::result_ptr ) ) );
    connect( AudioEngine::instance(), SIGNAL( started( Tomahawk::result_ptr ) ), SLOT( enablePause() ) );
    connect( AudioEngine::instance(), SIGNAL( resumed() ), this, SLOT( enablePause() ) );
    connect( AudioEngine::instance(), SIGNAL( stopped() ), this, SLOT( enablePlay() ) );
    connect( AudioEngine::instance(), SIGNAL( paused() ),  this, SLOT( enablePlay() ) );

    connect( m_playPauseAction, SIGNAL( triggered() ), AudioEngine::instance(), SLOT( playPause() ) );
    connect( m_stopAction,      SIGNAL( triggered() ), AudioEngine::instance(), SLOT( stop() ) );
    connect( m_prevAction,      SIGNAL( triggered() ), AudioEngine::instance(), SLOT( previous() ) );
    connect( m_nextAction,      SIGNAL( triggered() ), AudioEngine::instance(), SLOT( next() ) );
    connect( m_quitAction,      SIGNAL( triggered() ), (QObject*)APP,           SLOT( quit() ) );

    connect( &m_animationTimer, SIGNAL( timeout() ), SLOT( onAnimationTimer() ) );
    connect( this, SIGNAL( activated( QSystemTrayIcon::ActivationReason ) ), SLOT( onActivated( QSystemTrayIcon::ActivationReason ) ) );

    show();
}


TomahawkTrayIcon::~TomahawkTrayIcon()
{
    delete m_contextMenu;
}


void
TomahawkTrayIcon::setShowHideWindow( bool show )
{
    if ( show )
    {
        m_showWindowAction->setText( tr( "Hide Tomahawk Window" ) );
        m_showWindowAction->setData( show );
    }
    else
    {
        m_showWindowAction->setText( tr( "Show Tomahawk Window" ) );
    }

    m_showWindowAction->setData( show );
}


void
TomahawkTrayIcon::showWindow()
{
    if( !m_showWindowAction->data().toBool() )
    {
        APP->mainWindow()->show();
        APP->mainWindow()->raise();

        setShowHideWindow( true );
    }
    else
    {
        APP->mainWindow()->hide();

        setShowHideWindow( false );
    }
}


void
TomahawkTrayIcon::setResult( const Tomahawk::result_ptr& result )
{
    m_currentTrack = result;
    refreshToolTip();
}


void
TomahawkTrayIcon::refreshToolTip()
{
    #ifdef Q_WS_MAC
    // causes issues with OS X menubar, also none
    // of the other OS X menubar icons have a tooltip
    return;
    #endif

    QString tip;
    if ( !m_currentTrack.isNull() )
    {
        tip = m_currentTrack->artist()->name() + " " + QChar( 8211 ) /*en dash*/ + " " + m_currentTrack->track();
    }
    else
    {
        tip = tr( "Currently not playing." );
    }

    #ifdef Q_WS_WIN
        // Good old crappy Win32
        tip.replace( "&", "&&&" );
    #endif

    setToolTip( tip );
}


void
TomahawkTrayIcon::onAnimationTimer()
{
/*    m_currentAnimationFrame++;
    if( m_currentAnimationFrame >= m_animationPixmaps.count() )
        m_currentAnimationFrame = 0;

    setIcon( m_animationPixmaps.at( m_currentAnimationFrame ) );*/
}


void
TomahawkTrayIcon::onActivated( QSystemTrayIcon::ActivationReason reason )
{
#ifdef Q_WS_MAC
    return;
#endif

    switch( reason )
    {
        case QSystemTrayIcon::Trigger:
        {
            TomahawkWindow* mainwindow = APP->mainWindow();
            if ( mainwindow->isVisible() )
            {
                mainwindow->hide();
            }
            else
            {
                mainwindow->show();
            }
        }
        break;

        default:
            break;
    }
}


void
TomahawkTrayIcon::enablePlay()
{
    m_playPauseAction->setText( tr( "Play" ) );
}


void
TomahawkTrayIcon::enablePause()
{
    m_playPauseAction->setText( tr( "Pause" ) );
}


bool
TomahawkTrayIcon::event( QEvent* e )
{
    // Beginning with Qt 4.3, QSystemTrayIcon supports wheel events, but only
    // on X11. Let's make it adjust the volume.
    if ( e->type() == QEvent::Wheel )
    {
        if ( ((QWheelEvent*)e)->delta() > 0 )
        {
            AudioEngine::instance()->raiseVolume();
        }
        else
        {
            AudioEngine::instance()->lowerVolume();
        }

        return true;
    }

    return QSystemTrayIcon::event( e );
}

