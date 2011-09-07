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

#include "tomahawkwindow.h"


#include "artist.h"
#include "tomahawksettings.h"
#include "tomahawktrayicon.h"
#include "tomahawkapp.h"
#include "sourcetree/sourcesmodel.h"
#include "viewmanager.h"

#include <QCloseEvent>

using namespace Tomahawk;

SourcesModel* TomahawkWindow::s_sourcesModel = 0;

TomahawkWindow::TomahawkWindow( QWidget* parent )
    : QMainWindow( parent )
    , m_trayIcon( new TomahawkTrayIcon( this ) )
{
    setWindowIcon( QIcon( RESPATH "icons/tomahawk-icon-128x128.png" ) );

    if( !ViewManager::instance() )
        new ViewManager( this );

    if( !s_sourcesModel )
        s_sourcesModel = new SourcesModel( this );
}


TomahawkWindow::~TomahawkWindow()
{
    saveSettings();
}


void
TomahawkWindow::loadSettings()
{
    TomahawkSettings* s = TomahawkSettings::instance();

    // Workaround for broken window geometry restoring on Qt Cocoa when setUnifiedTitleAndToolBarOnMac is true.
    // See http://bugreports.qt.nokia.com/browse/QTBUG-3116 and
    // http://lists.qt.nokia.com/pipermail/qt-interest/2009-August/011491.html
    // for the 'fix'
#ifdef QT_MAC_USE_COCOA
     bool workaround = !isVisible();
     if ( workaround )
     {
       // make "invisible"
       setWindowOpacity( 0 );
       // let Qt update its frameStruts
       show();
     }
#endif

    if ( !s->mainWindowGeometry().isEmpty() )
        restoreGeometry( s->mainWindowGeometry() );
    if ( !s->mainWindowState().isEmpty() )
        restoreState( s->mainWindowState() );

#ifdef QT_MAC_USE_COCOA
     if ( workaround )
     {
       // Make it visible again
       setWindowOpacity( 1 );
     }
#endif
}


void
TomahawkWindow::saveSettings()
{
    TomahawkSettings* s = TomahawkSettings::instance();
    s->setMainWindowGeometry( saveGeometry() );
    s->setMainWindowState( saveState() );
}


void
TomahawkWindow::changeEvent( QEvent* e )
{
    QMainWindow::changeEvent( e );

    switch ( e->type() )
    {
        case QEvent::LanguageChange:
            retranslateUi();
            break;

        default:
            break;
    }
}


void
TomahawkWindow::closeEvent( QCloseEvent* e )
{
#ifndef Q_WS_MAC
    if ( QSystemTrayIcon::isSystemTrayAvailable() )
    {
        hide();
        e->ignore();
        return;
    }
#else
    m_trayIcon->setShowHideWindow( false );
#endif

    e->accept();
}


void
TomahawkWindow::showEvent( QShowEvent* e )
{
    QMainWindow::showEvent( e );

#if defined( Q_OS_DARWIN )
    ui->actionMinimize->setDisabled( false );
    ui->actionZoom->setDisabled( false );
#endif
}


void
TomahawkWindow::hideEvent( QHideEvent* e )
{
    QMainWindow::hideEvent( e );

#if defined( Q_OS_DARWIN )
    ui->actionMinimize->setDisabled( true );
    ui->actionZoom->setDisabled( true );
#endif
}

void
TomahawkWindow::setWindowTitle( const QString& title )
{
    m_windowTitle = title;

    if ( m_currentTrack.isNull() )
        QMainWindow::setWindowTitle( title );
    else
    {
        QString s = tr( "%1 by %2", "track, artist name" ).arg( m_currentTrack->track(), m_currentTrack->artist()->name() );
        QMainWindow::setWindowTitle( tr( "%1 - %2", "current track, some window title" ).arg( s, title ) );
    }
}


void
TomahawkWindow::minimize()
{
    if ( isMinimized() )
    {
        showNormal();
    }
    else
    {
        showMinimized();
    }
}


void
TomahawkWindow::maximize()
{
    if ( isMaximized() )
    {
        showNormal();
    }
    else
    {
        showMaximized();
    }
}


void
TomahawkWindow::retranslateUi()
{

}
