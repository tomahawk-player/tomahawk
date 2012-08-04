/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright 2012,      Leo Franchi   <lfranchi@kde.org>
 *   Copyright 2012,      Teo Mrnjavac   <teo@kde.org>
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

#include "ActionCollection.h"
#include "TomahawkSettings.h"
#include "audio/AudioEngine.h"
#include "utils/TomahawkUtils.h"
#include "utils/Logger.h"
#include "Source.h"

ActionCollection* ActionCollection::s_instance = 0;
ActionCollection* ActionCollection::instance()
{
    return s_instance;
}


ActionCollection::ActionCollection( QObject* parent )
    : QObject( parent )
{
    s_instance = this;
    initActions();
}


ActionCollection::~ActionCollection()
{
    s_instance = 0;
    foreach( QString key, m_actionCollection.keys() )
        delete m_actionCollection[ key ];
}


void
ActionCollection::initActions()
{
    QAction *latchOn = new QAction( tr( "&Listen Along" ), this );
    latchOn->setIcon( QIcon( RESPATH "images/headphones-sidebar.png" ) );
    m_actionCollection[ "latchOn" ] = latchOn;
    QAction *latchOff = new QAction( tr( "Stop &Listening Along" ), this );
    latchOff->setIcon( QIcon( RESPATH "images/headphones-off.png" ) );
    m_actionCollection[ "latchOff" ] = latchOff;

    QAction *realtimeFollowingAlong = new QAction( tr( "&Follow in real-time" ), this );
    realtimeFollowingAlong->setCheckable( true );
    m_actionCollection[ "realtimeFollowingAlong" ] = realtimeFollowingAlong;

    bool isPublic = TomahawkSettings::instance()->privateListeningMode() == TomahawkSettings::PublicListening;
    QAction *privacyToggle = new QAction( ( isPublic ? tr( "&Listen Privately" ) : tr( "&Listen Publicly" ) ), this );
    privacyToggle->setIcon( QIcon( RESPATH "images/private-listening.png" ) );
    privacyToggle->setIconVisibleInMenu( isPublic );
    m_actionCollection[ "togglePrivacy" ] = privacyToggle;
    connect( m_actionCollection[ "togglePrivacy" ], SIGNAL( triggered() ), SLOT( togglePrivateListeningMode() ), Qt::UniqueConnection );

    m_actionCollection[ "loadPlaylist" ] =   new QAction( tr( "&Load Playlist" ), this );
    m_actionCollection[ "renamePlaylist" ] = new QAction( tr( "&Rename Playlist" ), this );
    m_actionCollection[ "copyPlaylist" ] =   new QAction( tr( "&Copy Playlist Link" ), this );
    m_actionCollection[ "playPause" ] =      new QAction( tr( "&Play" ), this );
    m_actionCollection[ "playPause" ]->setShortcut( Qt::Key_Space );
    m_actionCollection[ "playPause" ]->setShortcutContext( Qt::ApplicationShortcut );
    m_actionCollection[ "stop" ] =           new QAction( tr( "&Stop" ), this );
    m_actionCollection[ "previousTrack" ] =  new QAction( tr( "&Previous Track" ), this );
    m_actionCollection[ "nextTrack" ] =      new QAction( tr( "&Next Track" ), this );
    m_actionCollection[ "quit" ] =           new QAction( tr( "&Quit" ), this );
    m_actionCollection[ "quit" ]->setShortcut( QKeySequence::Quit );
    m_actionCollection[ "quit" ]->setShortcutContext( Qt::ApplicationShortcut );
    m_actionCollection[ "quit" ]->setMenuRole( QAction::QuitRole );

    // connect actions to AudioEngine
    AudioEngine *ae = AudioEngine::instance();
    connect( m_actionCollection[ "playPause" ],     SIGNAL( triggered() ), ae,   SLOT( playPause() ), Qt::UniqueConnection );
    connect( m_actionCollection[ "stop" ],          SIGNAL( triggered() ), ae,   SLOT( stop() ),      Qt::UniqueConnection );
    connect( m_actionCollection[ "previousTrack" ], SIGNAL( triggered() ), ae,   SLOT( previous() ),  Qt::UniqueConnection );
    connect( m_actionCollection[ "nextTrack" ],     SIGNAL( triggered() ), ae,   SLOT( next() ),      Qt::UniqueConnection );

    // main menu actions
    m_actionCollection[ "loadXSPF" ] =           new QAction( tr( "Load &XSPF..." ), this );
    m_actionCollection[ "updateCollection" ] =   new QAction( tr( "U&pdate Collection" ), this );
    m_actionCollection[ "rescanCollection" ] =   new QAction( tr( "Fully &Rescan Collection" ), this );
    m_actionCollection[ "showOfflineSources" ] = new QAction( tr( "Show Offline Sources" ), this );
    m_actionCollection[ "showOfflineSources" ]->setCheckable( true );
    m_actionCollection[ "preferences" ] =        new QAction( tr( "&Configure Tomahawk..." ), this );
    m_actionCollection[ "preferences" ]->setMenuRole( QAction::PreferencesRole );
#ifdef Q_OS_MAC
    m_actionCollection[ "minimize" ] =           new QAction( tr( "Minimize" ), this );
    m_actionCollection[ "minimize" ]->setShortcut( QKeySequence( "Ctrl+M" ) );
    m_actionCollection[ "zoom" ] =               new QAction( tr( "Zoom" ), this );
    m_actionCollection[ "zoom" ]->setShortcut( QKeySequence( "Meta+Ctrl+Z" ) );
#else
    m_actionCollection[ "toggleMenuBar" ] =     new QAction( tr( "Hide Menu Bar" ), this );
    m_actionCollection[ "toggleMenuBar" ]->setShortcut( QKeySequence( "Ctrl+M" ) );
    m_actionCollection[ "toggleMenuBar" ]->setShortcutContext( Qt::ApplicationShortcut );
#endif
    m_actionCollection[ "diagnostics" ] =        new QAction( tr( "Diagnostics..." ), this );
    m_actionCollection[ "diagnostics" ]->setMenuRole( QAction::ApplicationSpecificRole );
    m_actionCollection[ "aboutTomahawk" ] =      new QAction( tr( "About &Tomahawk..." ), this );
    m_actionCollection[ "aboutTomahawk" ]->setMenuRole( QAction::AboutRole );
    m_actionCollection[ "legalInfo" ] =          new QAction( tr( "&Legal Information..." ), this );
    m_actionCollection[ "legalInfo" ]->setMenuRole( QAction::ApplicationSpecificRole );
#if defined( Q_OS_MAC ) && defined( HAVE_SPARKLE ) || defined( Q_WS_WIN )
    m_actionCollection[ "checkForUpdates" ] =    new QAction( tr( "Check For Updates..." ), this );
    m_actionCollection[ "checkForUpdates" ]->setMenuRole( QAction::ApplicationSpecificRole );
#endif
    m_actionCollection[ "crashNow" ] =           new QAction( "Crash now...", this );
}

QMenuBar *
ActionCollection::createMenuBar( QWidget *parent )
{
    QMenuBar *menuBar = new QMenuBar( parent );

    QMenu *controlsMenu = new QMenu( tr( "&Controls" ), menuBar );
    controlsMenu->addAction( m_actionCollection[ "playPause" ] );
    controlsMenu->addAction( m_actionCollection[ "previousTrack" ] );
    controlsMenu->addAction( m_actionCollection[ "nextTrack" ] );
    controlsMenu->addSeparator();
    controlsMenu->addAction( m_actionCollection[ "togglePrivacy" ] );
    controlsMenu->addAction( m_actionCollection[ "showOfflineSources" ] );
    controlsMenu->addSeparator();
    controlsMenu->addAction( m_actionCollection[ "loadXSPF" ] );
    controlsMenu->addAction( m_actionCollection[ "updateCollection" ] );
    controlsMenu->addAction( m_actionCollection[ "rescanCollection" ] );
    controlsMenu->addSeparator();
    controlsMenu->addAction( m_actionCollection[ "quit" ] );

    QMenu *settingsMenu = new QMenu( tr( "&Settings" ), menuBar );
#ifndef Q_OS_MAC
    settingsMenu->addAction( m_actionCollection[ "toggleMenuBar" ] );
#endif
    settingsMenu->addAction( m_actionCollection[ "preferences" ] );

    QMenu *helpMenu = new QMenu( tr( "&Help" ), menuBar );
    helpMenu->addAction( m_actionCollection[ "diagnostics" ] );
    helpMenu->addAction( m_actionCollection[ "aboutTomahawk" ] );
    helpMenu->addAction( m_actionCollection[ "legalInfo" ] );

    // Setup update check
#ifndef Q_OS_MAC
    helpMenu->insertSeparator( m_actionCollection[ "aboutTomahawk" ] );
#endif

#if defined( Q_OS_MAC ) && defined( HAVE_SPARKLE )
    helpMenu->addAction( m_actionCollection[ "checkForUpdates" ] );
#elif defined( Q_WS_WIN )
    helpMenu->addSeparator();
    helpMenu->addAction( m_actionCollection[ "checkForUpdates" ] );
#endif
    if ( qApp->arguments().contains( "--debug" ) )
    {
        helpMenu->addSeparator();
        helpMenu->addAction( m_actionCollection[ "crashNow" ] );
    }

    menuBar->addMenu( controlsMenu );
    menuBar->addMenu( settingsMenu );
#if defined( Q_OS_MAC )
    QMenu *windowMenu = new QMenu( tr( "&Window" ), menuBar );
    windowMenu->addAction( m_actionCollection[ "minimize" ] );
    windowMenu->addAction( m_actionCollection[ "zoom" ] );

    menuBar->addMenu( windowMenu );
#endif
    menuBar->addMenu( helpMenu );
    return menuBar;
}

QMenu *
ActionCollection::createCompactMenu( QWidget *parent )
{
    QMenu *compactMenu = new QMenu( tr( "Main Menu" ), parent );

    compactMenu->addAction( m_actionCollection[ "playPause" ] );
    compactMenu->addAction( m_actionCollection[ "previousTrack" ] );
    compactMenu->addAction( m_actionCollection[ "nextTrack" ] );
    compactMenu->addSeparator();
    compactMenu->addAction( m_actionCollection[ "togglePrivacy" ] );
    compactMenu->addAction( m_actionCollection[ "showOfflineSources" ] );
    compactMenu->addSeparator();
    compactMenu->addAction( m_actionCollection[ "loadXSPF" ] );
    compactMenu->addAction( m_actionCollection[ "updateCollection" ] );
    compactMenu->addAction( m_actionCollection[ "rescanCollection" ] );
    compactMenu->addSeparator();

#ifdef Q_OS_MAC // This should never happen anyway
    compactMenu->addAction( m_actionCollection[ "minimize" ] );
    compactMenu->addAction( m_actionCollection[ "zoom" ] );
#else
    compactMenu->addAction( m_actionCollection[ "toggleMenuBar" ] );
#endif
    compactMenu->addAction( m_actionCollection[ "preferences" ] );
    compactMenu->addSeparator();

    compactMenu->addAction( m_actionCollection[ "diagnostics" ] );
    compactMenu->addAction( m_actionCollection[ "aboutTomahawk" ] );
    compactMenu->addAction( m_actionCollection[ "legalInfo" ] );

    // Setup update check
#ifndef Q_OS_MAC
    compactMenu->insertSeparator( m_actionCollection[ "aboutTomahawk" ] );
#endif

#if defined( Q_OS_MAC ) && defined( HAVE_SPARKLE )
    compactMenu->addAction( m_actionCollection[ "checkForUpdates" ] );
#elif defined( Q_WS_WIN )
    compactMenu->addSeparator();
    compactMenu->addAction( m_actionCollection[ "checkForUpdates" ] );
#endif
    if ( qApp->arguments().contains( "--debug" ) )
    {
        compactMenu->addSeparator();
        compactMenu->addAction( m_actionCollection[ "crashNow" ] );
    }
    compactMenu->addSeparator();
    compactMenu->addAction( m_actionCollection[ "quit" ] );

    return compactMenu;
}


void
ActionCollection::addAction( ActionCollection::ActionDestination category, QAction* action, QObject* notify )
{
    QList< QAction* > actions = m_categoryActions.value( category );
    actions.append( action );
    m_categoryActions[ category ] = actions;

    if ( notify )
        m_actionNotifiers[ action ] = notify;
}


QAction*
ActionCollection::getAction( const QString& name )
{
    return m_actionCollection.value( name, 0 );
}


QObject*
ActionCollection::actionNotifier( QAction* action )
{
    return m_actionNotifiers.value( action, 0 );
}


QList< QAction* >
ActionCollection::getAction( ActionCollection::ActionDestination category )
{
    return m_categoryActions.value( category );
}


void
ActionCollection::removeAction( QAction* action )
{
    removeAction( action, LocalPlaylists );
}


void
ActionCollection::removeAction( QAction* action, ActionCollection::ActionDestination category )
{
    QList< QAction* > actions = m_categoryActions.value( category );
    actions.removeAll( action );
    m_categoryActions[ category ] = actions;

    m_actionNotifiers.remove( action );
}


void
ActionCollection::togglePrivateListeningMode()
{
    tDebug() << Q_FUNC_INFO;
    if ( TomahawkSettings::instance()->privateListeningMode() == TomahawkSettings::PublicListening )
        TomahawkSettings::instance()->setPrivateListeningMode( TomahawkSettings::FullyPrivate );
    else
        TomahawkSettings::instance()->setPrivateListeningMode( TomahawkSettings::PublicListening );

    QAction *privacyToggle = m_actionCollection[ "togglePrivacy" ];
    bool isPublic = TomahawkSettings::instance()->privateListeningMode() == TomahawkSettings::PublicListening;
    privacyToggle->setText( ( isPublic ? tr( "&Listen Privately" ) : tr( "&Listen Publicly" ) ) );
    privacyToggle->setIconVisibleInMenu( isPublic );

    emit privacyModeChanged();
}
