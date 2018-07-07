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
#include "Source.h"
#include "audio/AudioEngine.h"
#include "utils/ImageRegistry.h"
#include "utils/TomahawkUtils.h"
#include "utils/Logger.h"

#include <QCoreApplication>


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
    // ATTENTION: Don't set ApplicationSpecificRole for submenu actions: they won't show up on OS X (Qt 5.5)

    QAction *latchOn = new QAction( tr( "&Listen Along" ), this );
    latchOn->setIcon( ImageRegistry::instance()->icon( RESPATH "images/headphones.svg" ) );
    m_actionCollection[ "latchOn" ] = latchOn;
    QAction *latchOff = new QAction( tr( "Stop &Listening Along" ), this );
    latchOff->setIcon( ImageRegistry::instance()->icon( RESPATH "images/headphones-off.svg" ) );
    m_actionCollection[ "latchOff" ] = latchOff;

    QAction *realtimeFollowingAlong = new QAction( tr( "&Follow in Real-Time" ), this );
    realtimeFollowingAlong->setCheckable( true );
    m_actionCollection[ "realtimeFollowingAlong" ] = realtimeFollowingAlong;

    bool isPublic = TomahawkSettings::instance()->privateListeningMode() == TomahawkSettings::PublicListening;
    m_actionCollection[ "togglePrivacy" ] = new QAction( tr( "&Listen Privately" ) , this );
    m_actionCollection[ "togglePrivacy" ]->setCheckable( true );
    m_actionCollection[ "togglePrivacy" ]->setChecked( !isPublic );
    connect( m_actionCollection[ "togglePrivacy" ], SIGNAL( triggered() ), SLOT( togglePrivateListeningMode() ), Qt::UniqueConnection );

    m_actionCollection[ "loadPlaylist" ] =   new QAction( tr( "&Load Playlist" ), this );
    m_actionCollection[ "loadStation" ] =    new QAction( tr( "&Load Station" ), this );
    m_actionCollection[ "renamePlaylist" ] = new QAction( tr( "&Rename Playlist" ), this );
    m_actionCollection[ "renameStation" ] = new QAction( tr( "&Rename Station" ), this );
    m_actionCollection[ "copyPlaylist" ] =   new QAction( tr( "&Copy Playlist Link" ), this );
    m_actionCollection[ "playPause" ] =      new QAction( tr( "&Play" ), this );
    m_actionCollection[ "playPause" ]->setIcon( ImageRegistry::instance()->icon( RESPATH "images/play.svg" ) );
    m_actionCollection[ "playPause" ]->setShortcut( Qt::Key_Space );
    m_actionCollection[ "playPause" ]->setShortcutContext( Qt::ApplicationShortcut );
    m_actionCollection[ "stop" ] =           new QAction( tr( "&Stop" ), this );
    m_actionCollection[ "previousTrack" ] =  new QAction( tr( "&Previous Track" ), this );
    m_actionCollection[ "previousTrack" ]->setIcon( ImageRegistry::instance()->icon( RESPATH "images/back.svg" ) );
//    m_actionCollection[ "previousTrack" ]->setShortcut( QKeySequence( "Left" ) );
    m_actionCollection[ "nextTrack" ] =      new QAction( tr( "&Next Track" ), this );
    m_actionCollection[ "nextTrack" ]->setIcon( ImageRegistry::instance()->icon( RESPATH "images/forward.svg" ) );
//    m_actionCollection[ "nextTrack" ]->setShortcut( QKeySequence( "Right" ) );
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
    m_actionCollection[ "importPlaylist" ] = new QAction( tr( "Import Playlist..." ), this );
    m_actionCollection[ "updateCollection" ] = new QAction( tr( "U&pdate Collection" ), this );
    m_actionCollection[ "rescanCollection" ] = new QAction( tr( "Fully &Rescan Collection" ), this );
    m_actionCollection[ "showOfflineSources" ] = new QAction( tr( "Show Offline Friends" ), this );
    m_actionCollection[ "showOfflineSources" ]->setCheckable( true );
    m_actionCollection[ "preferences" ] = new QAction( tr( "&Configure %applicationName..." ), this );
    m_actionCollection[ "preferences" ]->setMenuRole( QAction::PreferencesRole );
    m_actionCollection[ "createPlaylist" ] = new QAction( tr( "Create Playlist" ), this );
    m_actionCollection[ "createPlaylist" ]->setShortcut( QKeySequence( "Ctrl+N" ) );
    m_actionCollection[ "createPlaylist" ]->setShortcutContext( Qt::ApplicationShortcut );
    // echonest is dead, disable stations
    /*
    m_actionCollection[ "createStation" ] = new QAction( tr( "Create Station" ), this );
    m_actionCollection[ "createStation" ]->setShortcut( QKeySequence( "Ctrl+S" ) );
    m_actionCollection[ "createStation" ]->setShortcutContext( Qt::ApplicationShortcut );
    */
#ifdef Q_OS_MAC
    m_actionCollection[ "minimize" ] = new QAction( tr( "Minimize" ), this );
    m_actionCollection[ "minimize" ]->setShortcut( QKeySequence( "Ctrl+M" ) );
    m_actionCollection[ "zoom" ] = new QAction( tr( "Zoom" ), this );
    m_actionCollection[ "zoom" ]->setShortcut( QKeySequence( "Meta+Ctrl+Z" ) );
    m_actionCollection[ "fullscreen" ] = new QAction( tr( "Enter Full Screen" ), this );
    m_actionCollection[ "fullscreen" ]->setShortcut( QKeySequence( "Meta+Ctrl+F" ) );
#else
    m_actionCollection[ "toggleMenuBar" ] = new QAction( tr( "Hide Menu Bar" ), this );
    m_actionCollection[ "toggleMenuBar" ]->setShortcut( QKeySequence( "Ctrl+M" ) );
    m_actionCollection[ "toggleMenuBar" ]->setShortcutContext( Qt::ApplicationShortcut );
#endif
    m_actionCollection[ "diagnostics" ] = new QAction( tr( "Diagnostics..." ), this );
    m_actionCollection[ "diagnostics" ]->setMenuRole( QAction::ApplicationSpecificRole );
    m_actionCollection[ "aboutTomahawk" ] = new QAction( tr( "About &%applicationName..." ), this );
    m_actionCollection[ "aboutTomahawk" ]->setMenuRole( QAction::AboutRole );
    m_actionCollection[ "legalInfo" ] = new QAction( tr( "&Legal Information..." ), this );
    m_actionCollection[ "legalInfo" ]->setMenuRole( QAction::ApplicationSpecificRole );
    m_actionCollection[ "openLogfile" ] = new QAction( tr( "&View Logfile" ), this );
    m_actionCollection[ "openLogfile" ]->setMenuRole( QAction::ApplicationSpecificRole );
    #if defined( Q_OS_MAC ) && defined( HAVE_SPARKLE ) || defined( Q_OS_WIN )
    m_actionCollection[ "checkForUpdates" ] = new QAction( tr( "Check For Updates..." ), this );
    m_actionCollection[ "checkForUpdates" ]->setMenuRole( QAction::ApplicationSpecificRole );
#endif
    m_actionCollection[ "crashNow" ] = new QAction( "Crash now...", this );
    m_actionCollection[ "whatsnew_0_8" ] = new QAction( tr( "%applicationName 0.8" ) , this );
    m_actionCollection[ "reportBug" ] = new QAction( tr( "Report a Bug" ) , this );
    m_actionCollection[ "getSupport" ] = new QAction( tr( "Get Support" ) , this );
    m_actionCollection[ "helpTranslate" ] = new QAction( tr( "Help Us Translate" ) , this );
}


QMenuBar*
ActionCollection::createMenuBar( QWidget *parent )
{
    QMenuBar* menuBar = new QMenuBar( parent );
    menuBar->setFont( TomahawkUtils::systemFont() );

    QMenu* controlsMenu = new QMenu( tr( "&Controls" ), menuBar );
    controlsMenu->setFont( TomahawkUtils::systemFont() );
    controlsMenu->addAction( m_actionCollection[ "playPause" ] );
    controlsMenu->addAction( m_actionCollection[ "previousTrack" ] );
    controlsMenu->addAction( m_actionCollection[ "nextTrack" ] );
    controlsMenu->addSeparator();
    controlsMenu->addAction( m_actionCollection[ "togglePrivacy" ] );
    controlsMenu->addAction( m_actionCollection[ "showOfflineSources" ] );
    controlsMenu->addSeparator();
    controlsMenu->addAction( m_actionCollection[ "createPlaylist" ] );
    // echonest is dead, disable stations
    // controlsMenu->addAction( m_actionCollection[ "createStation" ] );
    controlsMenu->addAction( m_actionCollection[ "importPlaylist" ] );
    controlsMenu->addAction( m_actionCollection[ "updateCollection" ] );
    controlsMenu->addAction( m_actionCollection[ "rescanCollection" ] );
    controlsMenu->addSeparator();
    controlsMenu->addAction( m_actionCollection[ "quit" ] );

    QMenu* settingsMenu = new QMenu( tr( "&Settings" ), menuBar );
    settingsMenu->setFont( TomahawkUtils::systemFont() );
#ifndef Q_OS_MAC
    settingsMenu->addAction( m_actionCollection[ "toggleMenuBar" ] );
#endif
    settingsMenu->addAction( m_actionCollection[ "preferences" ] );

    QMenu* helpMenu = new QMenu( tr( "&Help" ), menuBar );
    helpMenu->setFont( TomahawkUtils::systemFont() );
    helpMenu->addAction( m_actionCollection[ "diagnostics" ] );
    helpMenu->addAction( m_actionCollection[ "openLogfile" ] );
    helpMenu->addAction( m_actionCollection[ "legalInfo" ] );
    helpMenu->addAction( m_actionCollection["getSupport"] );
    helpMenu->addAction( m_actionCollection["reportBug"] );
    helpMenu->addAction( m_actionCollection["helpTranslate"] );
    helpMenu->addSeparator();
    QMenu* whatsNew = helpMenu->addMenu( ImageRegistry::instance()->icon( RESPATH "images/whatsnew.svg" ), tr( "What's New in ..." ) );
    whatsNew->setFont( TomahawkUtils::systemFont() );
    whatsNew->addAction( m_actionCollection[ "whatsnew_0_8" ] );
    helpMenu->addAction( m_actionCollection[ "aboutTomahawk" ] );

    // Setup update check
#ifndef Q_OS_MAC
    helpMenu->insertSeparator( m_actionCollection[ "legalInfo" ] );
#endif

#if defined( Q_OS_MAC ) && defined( HAVE_SPARKLE )
    helpMenu->addAction( m_actionCollection[ "checkForUpdates" ] );
#elif defined( Q_OS_WIN )
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
    QMenu* windowMenu = new QMenu( tr( "&Window" ), menuBar );
    windowMenu->setFont( TomahawkUtils::systemFont() );
    windowMenu->addAction( m_actionCollection[ "minimize" ] );
    windowMenu->addAction( m_actionCollection[ "zoom" ] );
    windowMenu->addAction( m_actionCollection[ "fullscreen" ] );

    menuBar->addMenu( windowMenu );
#endif

    menuBar->addMenu( helpMenu );
    return menuBar;
}


QMenu*
ActionCollection::createCompactMenu( QWidget* parent )
{
    QMenu* compactMenu = new QMenu( tr( "Main Menu" ), parent );
    compactMenu->setFont( TomahawkUtils::systemFont() );

    compactMenu->addAction( m_actionCollection[ "playPause" ] );
    compactMenu->addAction( m_actionCollection[ "previousTrack" ] );
    compactMenu->addAction( m_actionCollection[ "nextTrack" ] );
    compactMenu->addSeparator();
    compactMenu->addAction( m_actionCollection[ "togglePrivacy" ] );
    compactMenu->addAction( m_actionCollection[ "showOfflineSources" ] );
    compactMenu->addSeparator();
    compactMenu->addAction( m_actionCollection[ "importPlaylist" ] );
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
    compactMenu->addAction( m_actionCollection[ "openLogfile" ] );
    compactMenu->addAction( m_actionCollection[ "legalInfo" ] );
    QMenu* whatsNew = compactMenu->addMenu( ImageRegistry::instance()->icon( RESPATH "images/whatsnew.svg" ), tr( "What's New in ..." ) );
    whatsNew->addAction( m_actionCollection[ "whatsnew_0_8" ] );
    compactMenu->addAction( m_actionCollection[ "aboutTomahawk" ] );

    // Setup update check
#ifndef Q_OS_MAC
    compactMenu->insertSeparator( m_actionCollection[ "legalInfo" ] );
#endif

#if defined( Q_OS_MAC ) && defined( HAVE_SPARKLE )
    compactMenu->addAction( m_actionCollection[ "checkForUpdates" ] );
#elif defined( Q_OS_WIN )
    compactMenu->addSeparator();
    compactMenu->addAction( m_actionCollection[ "checkForUpdates" ] );
#endif
    if ( qApp->arguments().contains( "--debug" ) )
    {
        compactMenu->addSeparator();
        compactMenu->addAction( m_actionCollection[ "crashNow" ] );
    }
    compactMenu->addSeparator();
    compactMenu->addAction( m_actionCollection["getSupport"] );
    compactMenu->addAction( m_actionCollection["reportBug"] );
    compactMenu->addAction( m_actionCollection["helpTranslate"] );
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

    bool isPublic = TomahawkSettings::instance()->privateListeningMode() == TomahawkSettings::PublicListening;
    m_actionCollection[ "togglePrivacy" ]->setChecked( !isPublic );

    emit privacyModeChanged();
}
