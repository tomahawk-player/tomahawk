/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2012, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright 2012,      Teo Mrnjavac <teo@kde.org>
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

#include "TomahawkWindow.h"
#include "ui_TomahawkWindow.h"

#include <QAction>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QShowEvent>
#include <QHideEvent>
#include <QInputDialog>
#include <QPixmap>
#include <QPropertyAnimation>
#include <QLineEdit>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>

#include "accounts/AccountManager.h"
#include "sourcetree/SourceTreeView.h"
#include "network/Servent.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/ProxyStyle.h"
#include "utils/WidgetDragFilter.h"
#include "widgets/AccountsToolButton.h"
#include "widgets/AnimatedSplitter.h"
#include "widgets/NewPlaylistWidget.h"
#include "widgets/SearchWidget.h"
#include "widgets/PlaylistTypeSelectorDialog.h"
#include "widgets/ContainedMenuButton.h"
#include "thirdparty/Qocoa/qsearchfield.h"
#include "playlist/dynamic/GeneratorInterface.h"
#include "playlist/PlaylistModel.h"
#include "playlist/PlaylistView.h"
#include "playlist/QueueView.h"
#include "jobview/JobStatusView.h"
#include "jobview/JobStatusModel.h"
#include "jobview/ErrorStatusMessage.h"
#include "jobview/JobStatusModel.h"
#include "sip/SipPlugin.h"
#include "filemetadata/ScanManager.h"

#include "Playlist.h"
#include "Query.h"
#include "Artist.h"
#include "ViewManager.h"
#include "ActionCollection.h"
#include "AudioControls.h"
#include "SettingsDialog.h"
#include "DiagnosticsDialog.h"
#include "TomahawkSettings.h"
#include "SourceList.h"
#include "TomahawkTrayIcon.h"
#include "TomahawkApp.h"
#include "LoadXSPFDialog.h"
#include "utils/ImageRegistry.h"
#include "utils/Logger.h"

#include "config.h"

#if defined( Q_WS_WIN )
    #if defined ( WITH_QtSparkle )
        #include <qtsparkle/Updater>
    #endif

    #include <windows.h>
    #include <shellapi.h>

    #ifndef THBN_CLICKED
        #define THBN_CLICKED    0x1800
    #endif
#endif

using namespace Tomahawk;
using namespace Accounts;


TomahawkWindow::TomahawkWindow( QWidget* parent )
    : QMainWindow( parent )
#ifdef Q_OS_WIN
    , m_buttonCreatedID( RegisterWindowMessage( L"TaskbarButtonCreated" ) )
  #ifdef HAVE_THUMBBUTTON
    , m_taskbarList( 0 )
  #endif
#endif
    , ui( new Ui::TomahawkWindow )
    , m_searchWidget( 0 )
    , m_audioControls( new AudioControls( this ) )
    , m_trayIcon( new TomahawkTrayIcon( this ) )
    , m_settingsDialog( 0 )
    , m_audioRetryCounter( 0 )
{
    setWindowIcon( QIcon( RESPATH "icons/tomahawk-icon-128x128.png" ) );

    ViewManager* vm = new ViewManager( this );
    connect( vm, SIGNAL( showQueueRequested() ), SLOT( showQueue() ) );
    connect( vm, SIGNAL( hideQueueRequested() ), SLOT( hideQueue() ) );
    connect( APP, SIGNAL( tomahawkLoaded() ), vm, SLOT( setTomahawkLoaded() ) ); // Pass loaded signal into libtomahawk so components in there can connect to ViewManager

#ifdef Q_OS_WIN
    connect( AudioEngine::instance(), SIGNAL( stateChanged( AudioState, AudioState) ), SLOT( audioStateChanged( AudioState, AudioState) ) );
#endif
    ui->setupUi( this );

    applyPlatformTweaks();

    ui->centralWidget->setContentsMargins( 0, 0, 0, 0 );
    TomahawkUtils::unmarginLayout( ui->centralWidget->layout() );

    setupMenuBar();
    setupToolBar();
    setupSideBar();
    statusBar()->addPermanentWidget( m_audioControls, 1 );

    setupUpdateCheck();
    loadSettings();
    setupSignals();

    if ( qApp->arguments().contains( "--debug" ) )
    {
        connect( ActionCollection::instance()->getAction( "crashNow" ), SIGNAL( triggered() ), SLOT( crashNow() ) );
    }

    // set initial state
    audioStopped();

    vm->setQueue( m_queueView );
    vm->showWelcomePage();
}


TomahawkWindow::~TomahawkWindow()
{
    saveSettings();
    delete ui;
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
     bool workaround = isVisible();
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
    if ( !s->mainWindowSplitterState().isEmpty() )
        ui->splitter->restoreState( s->mainWindowSplitterState() );

    // Always set stretch factor. If user hasn't manually set splitter sizes,
    // this will ensure a sane default on all startups. If the user has, the manual
    // size will override the default stretching
    ui->splitter->setStretchFactor( 0, 0 );
    ui->splitter->setStretchFactor( 1, 1 );

#ifdef QT_MAC_USE_COCOA
     if ( workaround )
     {
       // Make it visible again
       setWindowOpacity( 1 );
     }
#endif

#ifndef Q_OS_MAC
    bool mbVisible = s->menuBarVisible();
    menuBar()->setVisible( mbVisible );
    m_compactMenuAction->setVisible( !mbVisible );
    ActionCollection::instance()->getAction( "toggleMenuBar" )->setText( mbVisible ? tr( "Hide Menu Bar" ) : tr( "Show Menu Bar" ) );
#endif
}


void
TomahawkWindow::saveSettings()
{
    TomahawkSettings* s = TomahawkSettings::instance();
    s->setMainWindowGeometry( saveGeometry() );
    s->setMainWindowState( saveState() );
    s->setMainWindowSplitterState( ui->splitter->saveState() );
    s->setMenuBarVisible( menuBar()->isVisible() );
}


void
TomahawkWindow::applyPlatformTweaks()
{
    // HACK: QtCurve causes an infinite loop on startup. This is because
    //       setStyle calls setPalette, which calls ensureBaseStyle, which loads
    //       QtCurve. QtCurve calls setPalette, which creates an infinite loop.
    //       We could simply not use ProxyStyle under QtCurve, but that would
    //       make the whole UI look like crap.
    //       Instead, we tell ProxyStyle that it's running under QtCurve, so it
    //       can intercept QStyle::polish (which in the base implementation does
    //       nothing and in QtCurve does evil things), and avoid forwarding it
    //       to QtCurve.
    bool isQtCurve = false;
    if ( QString( qApp->style()->metaObject()->className() ).toLower().contains( "qtcurve" ) )
        isQtCurve = true;
    qApp->setStyle( new ProxyStyle( isQtCurve ) );

#ifdef Q_OS_MAC
    setUnifiedTitleAndToolBarOnMac( true );
    delete ui->hline1;
    delete ui->hline2;
#else
    ui->hline1->setStyleSheet( "border: 1px solid gray;" );
    ui->hline2->setStyleSheet( "border: 1px solid gray;" );
#endif
}


void
TomahawkWindow::setupToolBar()
{
    m_toolbar = addToolBar( "TomahawkToolbar" );
    m_toolbar->setObjectName( "TomahawkToolbar" );
    m_toolbar->setMovable( false );
    m_toolbar->setFloatable( false );
    m_toolbar->setIconSize( QSize( 22, 22 ) );
    m_toolbar->setToolButtonStyle( Qt::ToolButtonIconOnly );
    m_toolbar->setStyleSheet( "border-bottom: 0px" );
    // If the toolbar is hidden accidentally it causes trouble on Unity because the user can't
    // easily bring it back (TWK-1046). So we just prevent the user from hiding the toolbar.
    // This should not affect Mac users.
    m_toolbar->setContextMenuPolicy( Qt::PreventContextMenu );

#ifdef Q_OS_MAC
    m_toolbar->installEventFilter( new WidgetDragFilter( m_toolbar ) );
#endif

    m_backAction = m_toolbar->addAction( ImageRegistry::instance()->icon( RESPATH "images/back.svg" ), tr( "Back" ), ViewManager::instance(), SLOT( historyBack() ) );
    m_backAction->setToolTip( tr( "Go back one page" ) );
    m_forwardAction = m_toolbar->addAction( ImageRegistry::instance()->icon( RESPATH "images/forward.svg" ), tr( "Forward" ), ViewManager::instance(), SLOT( historyForward() ) );
    m_forwardAction->setToolTip( tr( "Go forward one page" ) );

    m_toolbarLeftBalancer = new QWidget( this );
    m_toolbarLeftBalancer->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Preferred );
    m_toolbarLeftBalancer->setFixedWidth( 0 );
    m_toolbar->addWidget( m_toolbarLeftBalancer )->setProperty( "kind", QString( "spacer" ) );

    QWidget* toolbarLeftSpacer = new QWidget( this );
    toolbarLeftSpacer->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );
    m_toolbar->addWidget( toolbarLeftSpacer )->setProperty( "kind", QString( "spacer" ) );

    m_searchWidget = new QSearchField( this );
    m_searchWidget->setPlaceholderText( tr( "Search for any artist, album or song..." ) );
    m_searchWidget->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Preferred );
    m_searchWidget->setFixedWidth( 340 );
    connect( m_searchWidget, SIGNAL( returnPressed() ), this, SLOT( onFilterEdited() ) );

    m_toolbar->addWidget( m_searchWidget )->setProperty( "kind", QString( "search" ) );

    QWidget* rightSpacer = new QWidget( this );
    rightSpacer->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );
    m_toolbar->addWidget( rightSpacer )->setProperty( "kind", QString( "spacer" ) );

    m_toolbarRightBalancer = new QWidget( this );
    m_toolbarRightBalancer->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Preferred );
    m_toolbarRightBalancer->setFixedWidth( 0 );
    m_toolbar->addWidget( m_toolbarRightBalancer )->setProperty( "kind", QString( "spacer" ) );

    m_accountsButton = new AccountsToolButton( m_toolbar );
    m_toolbar->addWidget( m_accountsButton );
    connect( m_accountsButton, SIGNAL( widthChanged() ),
             this, SLOT( balanceToolbar() ) );

#ifndef Q_OS_MAC
    ContainedMenuButton* compactMenuButton = new ContainedMenuButton( m_toolbar );
    compactMenuButton->setIcon( ImageRegistry::instance()->icon( RESPATH "images/configure.svg" ) );
    compactMenuButton->setText( tr( "&Main Menu" ) );
    compactMenuButton->setMenu( m_compactMainMenu );
    compactMenuButton->setToolButtonStyle( Qt::ToolButtonIconOnly );
    m_compactMenuAction = m_toolbar->addWidget( compactMenuButton );
    //HACK: adding the toggle action to the window, otherwise the shortcut keys
    //      won't be picked up when the menu is hidden.
    //      This must be done for all menu bar actions that have shortcut keys :(
    //      Does not apply to Mac which always shows the menu bar.
    addAction( ActionCollection::instance()->getAction( "playPause" ) );
    addAction( ActionCollection::instance()->getAction( "toggleMenuBar" ) );
    addAction( ActionCollection::instance()->getAction( "quit" ) );
#endif
    balanceToolbar();
}


void
TomahawkWindow::balanceToolbar()
{
    int leftActionsWidth = 0;
    int rightActionsWidth = 0;
    bool flip = false;
    foreach ( QAction* action, m_toolbar->actions() )
    {
        if ( action->property( "kind" ) == QString( "spacer" ) ||
            !action->isVisible() )
            continue;
        else if ( action->property( "kind" ) == QString( "search" ) )
        {
            flip = true;
            continue;
        }

        QWidget* widget = m_toolbar->widgetForAction( action );

        if ( !flip ) //we accumulate on the left
        {
            leftActionsWidth += widget->sizeHint().width()
                             +  m_toolbar->layout()->spacing();
        }
        else //then, on the right
        {
            rightActionsWidth += widget->sizeHint().width()
                              +  m_toolbar->layout()->spacing();
        }
    }

    if ( leftActionsWidth > rightActionsWidth )
    {
        m_toolbarLeftBalancer->setFixedWidth( 0 );
        m_toolbarRightBalancer->setFixedWidth( leftActionsWidth - rightActionsWidth );
    }
    else
    {
        m_toolbarLeftBalancer->setFixedWidth( rightActionsWidth - leftActionsWidth );
        m_toolbarRightBalancer->setFixedWidth( 0 );
    }
}


void
TomahawkWindow::setupSideBar()
{
    // Delete fake designer widgets
    delete ui->sidebarWidget;
    delete ui->playlistWidget;

    QWidget* sidebarWidget = new QWidget();
    sidebarWidget->setLayout( new QVBoxLayout() );

    m_sidebar = new AnimatedSplitter();
    m_sidebar->setOrientation( Qt::Vertical );
    m_sidebar->setChildrenCollapsible( false );

    m_sourcetree = new SourceTreeView( this );
    JobStatusView* jobsView = new JobStatusView( m_sidebar );
    JobStatusModel* sourceModel = new JobStatusModel( jobsView );
    m_jobsModel = new JobStatusSortModel( jobsView );
    m_jobsModel->setJobModel( sourceModel );
    jobsView->setModel( m_jobsModel );

    m_queueView = new QueueView( m_sidebar );
    AudioEngine::instance()->setQueue( m_queueView->queue()->proxyModel()->playlistInterface() );

    m_sidebar->addWidget( m_sourcetree );
    m_sidebar->addWidget( jobsView );
    m_sidebar->addWidget( m_queueView );

//    m_sidebar->setGreedyWidget( 1 );
    m_sidebar->hide( 1, false );
    m_sidebar->hide( 2, false );
    m_sidebar->hide( 3, false );

    sidebarWidget->layout()->addWidget( m_sidebar );
    sidebarWidget->setContentsMargins( 0, 0, 0, 0 );
    sidebarWidget->layout()->setContentsMargins( 0, 0, 0, 0 );
    sidebarWidget->layout()->setMargin( 0 );

#ifndef Q_OS_MAC
    sidebarWidget->layout()->setSpacing( 0 );
#endif

    ui->splitter->addWidget( sidebarWidget );
    ui->splitter->addWidget( ViewManager::instance()->widget() );
    ui->splitter->setCollapsible( 1, false );

    ActionCollection::instance()->getAction( "showOfflineSources" )
            ->setChecked( TomahawkSettings::instance()->showOfflineSources() );
}


void
TomahawkWindow::setupUpdateCheck()
{
#if defined( Q_OS_MAC ) && defined( HAVE_SPARKLE )
    connect( ActionCollection::instance()->getAction( "checkForUpdates" ), SIGNAL( triggered( bool ) ),
             SLOT( checkForUpdates() ) );
    #elif defined( Q_WS_WIN ) && defined( WITH_QtSparkle )
    QUrl updaterUrl;

    if ( qApp->arguments().contains( "--debug" ) )
        updaterUrl.setUrl( "http://download.tomahawk-player.org/sparklewin-debug" );
    else
        updaterUrl.setUrl( "http://download.tomahawk-player.org/sparklewin" );

    qtsparkle::Updater* updater = new qtsparkle::Updater( updaterUrl, this );
    Q_ASSERT( TomahawkUtils::nam() != 0 );
    updater->SetNetworkAccessManager( TomahawkUtils::nam() );
    updater->SetVersion( TomahawkUtils::appFriendlyVersion() );

    connect( ActionCollection::instance()->getAction( "checkForUpdates" ), SIGNAL( triggered() ),
             updater, SLOT( CheckNow() ) );
#endif
}


#ifdef Q_OS_WIN
bool
TomahawkWindow::setupWindowsButtons()
{
#ifdef HAVE_THUMBBUTTON
    const GUID IID_ITaskbarList3 = { 0xea1afb91,0x9e28,0x4b86, { 0x90,0xe9,0x9e,0x9f,0x8a,0x5e,0xef,0xaf } };
    HRESULT hr = S_OK;

    THUMBBUTTONMASK dwMask = THUMBBUTTONMASK( THB_ICON | THB_TOOLTIP | THB_FLAGS );
    m_thumbButtons[TP_PREVIOUS].dwMask = dwMask;
    m_thumbButtons[TP_PREVIOUS].iId = TP_PREVIOUS;
    m_thumbButtons[TP_PREVIOUS].hIcon = thumbIcon(TomahawkUtils::PrevButton);
    m_thumbButtons[TP_PREVIOUS].dwFlags = THBF_ENABLED;
    m_thumbButtons[TP_PREVIOUS].szTip[ tr( "Back" ).toWCharArray( m_thumbButtons[TP_PREVIOUS].szTip ) ] = 0;

    m_thumbButtons[TP_PLAY_PAUSE].dwMask = dwMask;
    m_thumbButtons[TP_PLAY_PAUSE].iId = TP_PLAY_PAUSE;
    m_thumbButtons[TP_PLAY_PAUSE].hIcon = thumbIcon(TomahawkUtils::PlayButton);
    m_thumbButtons[TP_PLAY_PAUSE].dwFlags = THBF_ENABLED;
    m_thumbButtons[TP_PLAY_PAUSE].szTip[ tr( "Play" ).toWCharArray( m_thumbButtons[TP_PLAY_PAUSE].szTip ) ] = 0;

    m_thumbButtons[TP_NEXT].dwMask = dwMask;
    m_thumbButtons[TP_NEXT].iId = TP_NEXT;
    m_thumbButtons[TP_NEXT].hIcon = thumbIcon(TomahawkUtils::NextButton);
    m_thumbButtons[TP_NEXT].dwFlags = THBF_ENABLED;
    m_thumbButtons[TP_NEXT].szTip[ tr( "Next" ).toWCharArray( m_thumbButtons[TP_NEXT].szTip ) ] = 0;

    m_thumbButtons[3].dwMask = dwMask;
    m_thumbButtons[3].iId = -1;
    m_thumbButtons[3].hIcon = 0;
    m_thumbButtons[3].dwFlags = THBF_NOBACKGROUND | THBF_DISABLED;
    m_thumbButtons[3].szTip[0] = 0;

    m_thumbButtons[TP_LOVE].dwMask = dwMask;
    m_thumbButtons[TP_LOVE].iId = TP_LOVE;
    m_thumbButtons[TP_LOVE].hIcon = thumbIcon(TomahawkUtils::NotLoved);
    m_thumbButtons[TP_LOVE].dwFlags = THBF_DISABLED;
    m_thumbButtons[TP_LOVE].szTip[ tr( "Love" ).toWCharArray( m_thumbButtons[TP_LOVE].szTip ) ] = 0;

    if ( S_OK == CoCreateInstance( CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, IID_ITaskbarList3, (void **)&m_taskbarList ) )
    {
        hr = m_taskbarList->HrInit();
        if ( SUCCEEDED( hr ) )
        {
            hr = m_taskbarList->ThumbBarAddButtons( winId(), ARRAYSIZE( m_thumbButtons ), m_thumbButtons );
        }
        else
        {
            m_taskbarList->Release();
            m_taskbarList = 0;
        }
    }

    return SUCCEEDED( hr );
#else // HAVE_THUMBBUTTON
    return false;
#endif
}


HICON
TomahawkWindow::thumbIcon(TomahawkUtils::ImageType type)
{
    static QMap<TomahawkUtils::ImageType,HICON> thumbIcons;
    if (!thumbIcons.contains( type ) )
    {
        QPixmap pix ( TomahawkUtils::defaultPixmap(type , TomahawkUtils::Original, QSize( 20, 20 ) ) );
        thumbIcons[type] = pix.toWinHICON();
    }
    return thumbIcons[type];
}
#endif


void
TomahawkWindow::setupSignals()
{
    // <From AudioEngine>
    connect( AudioEngine::instance(), SIGNAL( error( AudioEngine::AudioErrorCode ) ), SLOT( onAudioEngineError( AudioEngine::AudioErrorCode ) ) );
    connect( AudioEngine::instance(), SIGNAL( loading( const Tomahawk::result_ptr& ) ), SLOT( onPlaybackLoading( const Tomahawk::result_ptr& ) ) );
    connect( AudioEngine::instance(), SIGNAL( started( Tomahawk::result_ptr ) ), SLOT( audioStarted() ) );
    connect( AudioEngine::instance(), SIGNAL( finished( Tomahawk::result_ptr ) ), SLOT( audioFinished() ) );
    connect( AudioEngine::instance(), SIGNAL( resumed() ), SLOT( audioStarted() ) );
    connect( AudioEngine::instance(), SIGNAL( paused() ), SLOT( audioPaused() ) );
    connect( AudioEngine::instance(), SIGNAL( stopped() ), SLOT( audioStopped() ) );

    // <Menu Items>
    ActionCollection *ac = ActionCollection::instance();
    connect( ac->getAction( "preferences" ), SIGNAL( triggered() ), SLOT( showSettingsDialog() ) );
    connect( ac->getAction( "diagnostics" ), SIGNAL( triggered() ), SLOT( showDiagnosticsDialog() ) );
    connect( ac->getAction( "legalInfo" ), SIGNAL( triggered() ), SLOT( legalInfo() ) );
    connect( ac->getAction( "openLogfile" ), SIGNAL( triggered() ), SLOT( openLogfile() ) );
    connect( ac->getAction( "updateCollection" ), SIGNAL( triggered() ), SLOT( updateCollectionManually() ) );
    connect( ac->getAction( "rescanCollection" ), SIGNAL( triggered() ), SLOT( rescanCollectionManually() ) );
    connect( ac->getAction( "loadXSPF" ), SIGNAL( triggered() ), SLOT( loadSpiff() ) );
    connect( ac->getAction( "aboutTomahawk" ), SIGNAL( triggered() ), SLOT( showAboutTomahawk() ) );
    connect( ac->getAction( "quit" ), SIGNAL( triggered() ), qApp, SLOT( quit() ) );
    connect( ac->getAction( "showOfflineSources" ), SIGNAL( triggered() ), SLOT( showOfflineSources() ) );

#if defined( Q_OS_MAC )
    connect( ac->getAction( "minimize" ), SIGNAL( triggered() ), SLOT( minimize() ) );
    connect( ac->getAction( "zoom" ), SIGNAL( triggered() ), SLOT( maximize() ) );
#else
    connect( ac->getAction( "toggleMenuBar" ), SIGNAL( triggered() ), SLOT( toggleMenuBar() ) );
#endif

    // <AccountHandler>
    connect( AccountManager::instance(), SIGNAL( authError( Tomahawk::Accounts::Account* ) ), SLOT( onAccountError() ) );

    connect( ViewManager::instance(), SIGNAL( historyBackAvailable( bool ) ), SLOT( onHistoryBackAvailable( bool ) ) );
    connect( ViewManager::instance(), SIGNAL( historyForwardAvailable( bool ) ), SLOT( onHistoryForwardAvailable( bool ) ) );
}


void
TomahawkWindow::setupMenuBar()
{
    // Always create a menubar, but only create a compactMenu on Windows and X11
    m_menuBar = ActionCollection::instance()->createMenuBar( this );
    setMenuBar( m_menuBar );
#ifndef Q_OS_MAC
    m_compactMainMenu = ActionCollection::instance()->createCompactMenu( this );
#endif
}


void
TomahawkWindow::changeEvent( QEvent* e )
{
    QMainWindow::changeEvent( e );

    switch ( e->type() )
    {
        case QEvent::LanguageChange:
            ui->retranslateUi( this );
            break;

        default:
            break;
    }
}


void
TomahawkWindow::closeEvent( QCloseEvent* e )
{
#ifndef Q_OS_MAC
    if ( e->spontaneous() && QSystemTrayIcon::isSystemTrayAvailable() )
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

#if defined( Q_OS_MAC )
    ActionCollection::instance()->getAction( "minimize" )->setDisabled( false );
    ActionCollection::instance()->getAction( "zoom" )->setDisabled( false );
#endif
}


void
TomahawkWindow::hideEvent( QHideEvent* e )
{
    QMainWindow::hideEvent( e );

#if defined( Q_OS_MAC )
    ActionCollection::instance()->getAction( "minimize" )->setDisabled( true );
    ActionCollection::instance()->getAction( "zoom" )->setDisabled( true );
#endif
}


void
TomahawkWindow::keyPressEvent( QKeyEvent* e )
{
    bool accept = true;

#if ! defined ( Q_OS_MAC )
#define KEY_PRESSED Q_FUNC_INFO << "Multimedia Key Pressed:"
    switch ( e->key() )
    {
        case Qt::Key_MediaPlay:
            tLog() << KEY_PRESSED << "Play";
            AudioEngine::instance()->playPause();
            break;
        case Qt::Key_MediaStop:
            tLog() << KEY_PRESSED << "Stop";
            AudioEngine::instance()->stop();
            break;
        case Qt::Key_MediaPrevious:
            tLog() << KEY_PRESSED << "Previous";
            AudioEngine::instance()->previous();
            break;
        case Qt::Key_MediaNext:
            tLog() << KEY_PRESSED << "Next";
            AudioEngine::instance()->next();
            break;
        case Qt::Key_MediaPause:
            tLog() << KEY_PRESSED << "Pause";
            AudioEngine::instance()->pause();
            break;
        case Qt::Key_MediaTogglePlayPause:
            tLog() << KEY_PRESSED << "PlayPause";
            AudioEngine::instance()->playPause();
            break;
        case Qt::Key_MediaRecord:
        default:
            accept = false;
    }
#else
    accept = false;
#endif

    if ( accept )
        e->accept();

    QMainWindow::keyPressEvent( e );
}


#ifdef Q_OS_WIN
bool
TomahawkWindow::winEvent( MSG* msg, long* result )
{
    #define TB_PRESSED Q_FUNC_INFO << "Taskbar Button Pressed:"

    switch ( msg->message )
    {
    case WM_COMMAND:
        if ( HIWORD( msg->wParam ) == THBN_CLICKED )
        {
            switch ( TB_STATES( LOWORD( msg->wParam ) ) )
            {
            case TP_PREVIOUS:
                tLog() << TB_PRESSED << "Previous";
                AudioEngine::instance()->previous();
                break;
            case TP_PLAY_PAUSE:
                tLog() << TB_PRESSED << "Play/Pause";
                AudioEngine::instance()->playPause();
                break;
            case TP_NEXT:
                tLog() << TB_PRESSED << "Next";
                AudioEngine::instance()->next();
                break;
            case TP_LOVE:
                tLog() << TB_PRESSED << "Love";
                if ( !AudioEngine::instance()->currentTrack().isNull() )
                {
                    AudioEngine::instance()->currentTrack()->toQuery()->setLoved( !AudioEngine::instance()->currentTrack()->toQuery()->loved() );
                    updateWindowsLoveButton();
                }
                break;
            }
            return true;
        }
        break;
    }

    if ( msg->message == m_buttonCreatedID )
        return setupWindowsButtons();

    return false;
}


#endif // Q_OS_WIN

void
TomahawkWindow::audioStateChanged( AudioState newState, AudioState oldState )
{
#ifdef HAVE_THUMBBUTTON
    if ( m_taskbarList == 0 )
        return;
    switch ( newState )
    {
    case AudioEngine::Playing:
    {
        m_thumbButtons[TP_PLAY_PAUSE].hIcon = thumbIcon(TomahawkUtils::PauseButton);
        m_thumbButtons[TP_PLAY_PAUSE].szTip[ tr( "Pause" ).toWCharArray( m_thumbButtons[TP_PLAY_PAUSE].szTip ) ] = 0;
        updateWindowsLoveButton();

    }
        break;

    case AudioEngine::Paused:
    {
        m_thumbButtons[TP_PLAY_PAUSE].hIcon = thumbIcon(TomahawkUtils::PlayButton);
        m_thumbButtons[TP_PLAY_PAUSE].szTip[ tr( "Play" ).toWCharArray( m_thumbButtons[TP_PLAY_PAUSE].szTip ) ] = 0;
    }
        break;

    case AudioEngine::Stopped:
    {
        if ( !AudioEngine::instance()->currentTrack().isNull() )
        {
            disconnect( AudioEngine::instance()->currentTrack()->toQuery().data(), SIGNAL( socialActionsLoaded() ), this, SLOT( updateWindowsLoveButton() ) );
        }

        m_thumbButtons[TP_PLAY_PAUSE].hIcon = thumbIcon(TomahawkUtils::PlayButton);
        m_thumbButtons[TP_PLAY_PAUSE].szTip[ tr( "Play" ).toWCharArray( m_thumbButtons[TP_PLAY_PAUSE].szTip ) ] = 0;

        m_thumbButtons[TP_LOVE].hIcon = thumbIcon(TomahawkUtils::NotLoved);
        m_thumbButtons[TP_LOVE].dwFlags = THBF_DISABLED;
    }
        break;

    default:
        return;
    }

    m_taskbarList->ThumbBarUpdateButtons( winId(), ARRAYSIZE( m_thumbButtons ), m_thumbButtons );
#else
    Q_UNUSED( newState );
    Q_UNUSED( oldState );
#endif // HAVE_THUMBBUTTON
}


void
TomahawkWindow::updateWindowsLoveButton()
{
#ifdef HAVE_THUMBBUTTON
    if ( m_taskbarList == 0 )
        return;
    if ( !AudioEngine::instance()->currentTrack().isNull() && AudioEngine::instance()->currentTrack()->toQuery()->loved() )
    {
        m_thumbButtons[TP_LOVE].hIcon = thumbIcon(TomahawkUtils::Loved);
        m_thumbButtons[TP_LOVE].szTip[ tr( "Unlove" ).toWCharArray( m_thumbButtons[TP_LOVE].szTip ) ] = 0;
    }
    else
    {
        m_thumbButtons[TP_LOVE].hIcon = thumbIcon(TomahawkUtils::NotLoved);
        m_thumbButtons[TP_LOVE].szTip[ tr( "Love" ).toWCharArray( m_thumbButtons[TP_LOVE].szTip ) ] = 0;
    }

    m_thumbButtons[TP_LOVE].dwFlags = THBF_ENABLED;
    m_taskbarList->ThumbBarUpdateButtons( winId(), ARRAYSIZE( m_thumbButtons ), m_thumbButtons );
#endif // HAVE_THUMBBUTTON
}


void
TomahawkWindow::onHistoryBackAvailable( bool avail )
{
    m_backAction->setEnabled( avail );
}


void
TomahawkWindow::onHistoryForwardAvailable( bool avail )
{
    m_forwardAction->setEnabled( avail );
}


void
TomahawkWindow::showSettingsDialog()
{
    if ( !m_settingsDialog )
        m_settingsDialog = new SettingsDialog;

    m_settingsDialog->show();
}


void
TomahawkWindow::showDiagnosticsDialog()
{
    DiagnosticsDialog win;
    win.exec();
}


void
TomahawkWindow::legalInfo()
{
    QDesktopServices::openUrl( QUrl( "http://www.tomahawk-player.org/legal.html" ) );
}


void
TomahawkWindow::openLogfile()
{
#ifdef WIN32
    ShellExecuteW( 0, 0, (LPCWSTR)Logger::logFile().utf16(), 0, 0, SW_SHOWNORMAL );
#else
    QDesktopServices::openUrl( QUrl::fromLocalFile( Logger::logFile() ) );
#endif
}


void
TomahawkWindow::updateCollectionManually()
{
    if ( TomahawkSettings::instance()->hasScannerPaths() )
        ScanManager::instance()->runNormalScan();
}


void
TomahawkWindow::rescanCollectionManually()
{
    if ( TomahawkSettings::instance()->hasScannerPaths() )
        ScanManager::instance()->runFullRescan();
}


void
TomahawkWindow::showOfflineSources()
{
    m_sourcetree->showOfflineSources(
        ActionCollection::instance()->getAction( "showOfflineSources" )->isChecked() );
    TomahawkSettings::instance()->setShowOfflineSources(
        ActionCollection::instance()->getAction( "showOfflineSources" )->isChecked() );
}


void
TomahawkWindow::fullScreenEntered()
{
    statusBar()->setSizeGripEnabled( false );
}


void
TomahawkWindow::fullScreenExited()
{
    statusBar()->setSizeGripEnabled( true );
}


void
TomahawkWindow::loadSpiff()
{
    LoadXSPFDialog* diag = new LoadXSPFDialog( this, Qt::Sheet );
#ifdef Q_OS_MAC
    connect( diag, SIGNAL( finished( int ) ), this, SLOT( loadXspfFinished( int ) ) );
    diag->show();
#else
    QPointer< LoadXSPFDialog > safe( diag );

    int ret = diag->exec();
    if ( !safe.isNull() && ret == QDialog::Accepted )
    {
        QUrl url = QUrl::fromUserInput( safe.data()->xspfUrl() );
        bool autoUpdate = safe.data()->autoUpdate();

        XSPFLoader* loader = new XSPFLoader( true, autoUpdate );
        connect( loader, SIGNAL( error( XSPFLoader::XSPFErrorCode ) ), SLOT( onXSPFError( XSPFLoader::XSPFErrorCode ) ) );
        connect( loader, SIGNAL( ok( Tomahawk::playlist_ptr ) ), SLOT( onXSPFOk( Tomahawk::playlist_ptr ) ) );
        loader->load( url );
    }
#endif
}


void
TomahawkWindow::loadXspfFinished( int ret )
{
    LoadXSPFDialog* d = qobject_cast< LoadXSPFDialog* >( sender() );
    Q_ASSERT( d );
    if ( ret == QDialog::Accepted )
    {
        QUrl url = QUrl::fromUserInput( d->xspfUrl() );
        bool autoUpdate = d->autoUpdate();

        XSPFLoader* loader = new XSPFLoader( true, autoUpdate );
        connect( loader, SIGNAL( error( XSPFLoader::XSPFErrorCode ) ), SLOT( onXSPFError( XSPFLoader::XSPFErrorCode ) ) );
        connect( loader, SIGNAL( ok( Tomahawk::playlist_ptr ) ), SLOT( onXSPFOk( Tomahawk::playlist_ptr ) ) );
        loader->load( url );
    }
    d->deleteLater();
}


void
TomahawkWindow::onXSPFOk( const Tomahawk::playlist_ptr& pl )
{
    ViewManager::instance()->show( pl );
}


void
TomahawkWindow::onXSPFError( XSPFLoader::XSPFErrorCode error )
{
    switch ( error )
    {
        case XSPFLoader::ParseError:
            QMessageBox::critical( this, tr( "XSPF Error" ), tr( "This is not a valid XSPF playlist." ) );
            break;

        case XSPFLoader::InvalidTrackError:
            QMessageBox::warning( this, tr( "Failed to save tracks" ), tr( "Some tracks in the playlist do not contain an artist and a title. They will be ignored." ), QMessageBox::Ok );
            break;
        default:
            //FIXME: This includes FetchError
            break;
    }
}


void
TomahawkWindow::onAudioEngineError( AudioEngine::AudioErrorCode /* error */ )
{
    QString msg;
#ifdef Q_WS_X11
    msg = tr( "Sorry, there is a problem accessing your audio device or the desired track, current track will be skipped. Make sure you have a suitable Phonon backend and required plugins installed." );
#else
    msg = tr( "Sorry, there is a problem accessing your audio device or the desired track, current track will be skipped." );
#endif
    JobStatusView::instance()->model()->addJob( new ErrorStatusMessage( msg, 15 ) );

    if ( m_audioRetryCounter < 3 )
        AudioEngine::instance()->play();
    m_audioRetryCounter++;
}


void
TomahawkWindow::createAutomaticPlaylist( QString playlistName )
{
    if ( playlistName.isEmpty() )
        return;

    source_ptr author = SourceList::instance()->getLocal();
    QString id = uuid();
    QString info  = ""; // FIXME
    QString creator = "someone"; // FIXME

    dynplaylist_ptr playlist = DynamicPlaylist::create( author, id, playlistName, info, creator, Static, false );
    playlist->setMode( Static );
    playlist->createNewRevision( uuid(), playlist->currentrevision(), playlist->type(), playlist->generator()->controls(), playlist->entries() );

    ViewManager::instance()->show( playlist );
}


void
TomahawkWindow::createStation()
{
    QString title = tr( "Station" );
    bool ok;
    QString playlistName = QInputDialog( this, Qt::Sheet ).getText( this, tr( "Create New Station" ), tr( "Name:" ), QLineEdit::Normal, title, &ok );
    if ( !ok )
        return;

    if ( playlistName.isEmpty() || playlistName == title )
    {
        QList< dynplaylist_ptr > pls = SourceList::instance()->getLocal()->collection()->stations();
        QStringList titles;
        foreach ( const playlist_ptr& pl, pls )
            titles << pl->title();

        playlistName = title;
        int i = 2;
        while ( titles.contains( playlistName ) )
        {
            playlistName = QString( "%1 (%2)" ).arg( title ).arg( i++ );
        }
    }

    source_ptr author = SourceList::instance()->getLocal();
    QString id = uuid();
    QString info  = ""; // FIXME
    QString creator = "someone"; // FIXME

    dynplaylist_ptr playlist = DynamicPlaylist::create( author, id, playlistName, info, creator, OnDemand, false );
    playlist->setMode( OnDemand );
    playlist->createNewRevision( uuid(), playlist->currentrevision(), playlist->type(), playlist->generator()->controls() );

    ViewManager::instance()->show( playlist );
}


void
TomahawkWindow::createPlaylist()
{
    PlaylistTypeSelectorDlg* playlistSelectorDlg = new PlaylistTypeSelectorDlg( TomahawkApp::instance()->mainWindow(), Qt::Sheet );

#ifndef Q_OS_MAC
    playlistSelectorDlg->setModal( true );
#endif

    connect( playlistSelectorDlg, SIGNAL( finished( int ) ), SLOT( playlistCreateDialogFinished( int ) ) );
    playlistSelectorDlg->show();
}


void
TomahawkWindow::playlistCreateDialogFinished( int ret )
{
    PlaylistTypeSelectorDlg* playlistSelectorDlg = qobject_cast< PlaylistTypeSelectorDlg* >( sender() );
    Q_ASSERT( playlistSelectorDlg );

    QString playlistName = playlistSelectorDlg->playlistName();

    if ( !playlistSelectorDlg->playlistTypeIsAuto() && ret )
    {
        if ( playlistName.isEmpty() )
        {
            QList< playlist_ptr > pls = SourceList::instance()->getLocal()->collection()->playlists();
            QStringList titles;
            foreach ( const playlist_ptr& pl, pls )
                titles << pl->title();

            QString title = tr( "Playlist" );
            playlistName = title;
            int i = 2;
            while ( titles.contains( playlistName ) )
            {
                playlistName = QString( "%1 (%2)" ).arg( title ).arg( i++ );
            }
        }

        playlist_ptr playlist = Tomahawk::Playlist::create( SourceList::instance()->getLocal(), uuid(), playlistName, "", "", false, QList< query_ptr>() );
        ViewManager::instance()->show( playlist );
    }
    else if ( playlistSelectorDlg->playlistTypeIsAuto() && ret )
    {
       // create Auto Playlist
        if ( playlistName.isEmpty() )
        {
            QList< dynplaylist_ptr > pls = SourceList::instance()->getLocal()->collection()->autoPlaylists();
            QStringList titles;
            foreach ( const dynplaylist_ptr& pl, pls )
                titles << pl->title();

            QString title = tr( "Automatic Playlist" );
            playlistName = title;
            int i = 2;
            while ( titles.contains( playlistName ) )
            {
                playlistName = QString( "%1 (%2)" ).arg( title ).arg( i++ );
            }
        }

       createAutomaticPlaylist( playlistName );
    }

    playlistSelectorDlg->deleteLater();
}


void
TomahawkWindow::audioStarted()
{
    m_audioRetryCounter = 0;

    ActionCollection::instance()->getAction( "playPause" )->setIcon( ImageRegistry::instance()->icon( RESPATH "images/pause-rest.svg" ) );
    ActionCollection::instance()->getAction( "playPause" )->setText( tr( "Pause" ) );
    ActionCollection::instance()->getAction( "stop" )->setEnabled( true );

#ifdef Q_OS_WIN
    connect( AudioEngine::instance()->currentTrack()->toQuery().data(), SIGNAL( socialActionsLoaded() ), SLOT( updateWindowsLoveButton() ) );
#endif
}


void
TomahawkWindow::audioFinished()
{
#ifdef Q_OS_WIN
    disconnect( AudioEngine::instance()->currentTrack()->toQuery().data(), SIGNAL( socialActionsLoaded() ), this, SLOT( updateWindowsLoveButton() ) );
#endif
}


void
TomahawkWindow::audioPaused()
{
    ActionCollection::instance()->getAction( "playPause" )->setIcon( ImageRegistry::instance()->icon( RESPATH "images/play-rest.svg" ) );
    ActionCollection::instance()->getAction( "playPause" )->setText( tr( "&Play" ) );
}


void
TomahawkWindow::audioStopped()
{
    audioPaused();
    ActionCollection::instance()->getAction( "stop" )->setEnabled( false );

    m_currentTrack = result_ptr();
    setWindowTitle( m_windowTitle );
}


void
TomahawkWindow::onPlaybackLoading( const Tomahawk::result_ptr& result )
{
    m_currentTrack = result;
    setWindowTitle( m_windowTitle );
}


void
TomahawkWindow::onAccountError()
{
    // TODO fix.
//     onAccountDisconnected();

    // TODO real error message from plugin kthxbbq
    QMessageBox::warning( this,
                          tr( "Authentication Error" ),
                          tr( "Error connecting to SIP: Authentication failed!" ),
                          QMessageBox::Ok );
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
TomahawkWindow::showAboutTomahawk()
{
    QString head, desc;

#ifdef DEBUG_BUILD
    head = tr( "<h2><b>Tomahawk %1<br/>(%2)</h2>" )
         .arg( TomahawkUtils::appFriendlyVersion() )
         .arg( qApp->applicationVersion() );
#else
    head = tr( "<h2><b>Tomahawk %1</h2>" )
         .arg( TomahawkUtils::appFriendlyVersion() );
#endif

    const QString copyright( tr( "Copyright 2010 - 2013" ) );
    const QString thanksto( tr( "Thanks to:" ) );

    desc = QString( "%1<br/>Christian Muehlhaeuser &lt;muesli@tomahawk-player.org&gt;<br/><br/>"
    "%2 Leo Franchi, Jeff Mitchell, Dominik Schmidt, Jason Herskowitz, Alejandro Wainzinger, Hugo Lindstr&ouml;m, Syd Lawrence, Michael Zanetti, Teo Mrnjavac, Christopher Reichert, Harald Sitter" )
              .arg( copyright )
              .arg( thanksto );

    QMessageBox::about( this, tr( "About Tomahawk" ), head + desc );
}


void
TomahawkWindow::checkForUpdates()
{
#ifdef Q_OS_MAC
    Tomahawk::checkForUpdates();
#endif
}


void
TomahawkWindow::onSearch( const QString& search )
{
    if ( !search.trimmed().isEmpty() )
        ViewManager::instance()->show( new SearchWidget( search, this ) );
}


void
TomahawkWindow::onFilterEdited()
{
    onSearch( m_searchWidget->text() );
    m_searchWidget->clear();
}


void
TomahawkWindow::showQueue()
{
    if ( QThread::currentThread() != thread() )
    {
        qDebug() << "Reinvoking in correct thread:" << Q_FUNC_INFO;
        QMetaObject::invokeMethod( this, "showQueue", Qt::QueuedConnection );
        return;
    }

    m_queueView->show();
}


void
TomahawkWindow::hideQueue()
{
    if ( QThread::currentThread() != thread() )
    {
        qDebug() << "Reinvoking in correct thread:" << Q_FUNC_INFO;
        QMetaObject::invokeMethod( this, "hideQueue", Qt::QueuedConnection );
        return;
    }

    m_queueView->hide();
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
TomahawkWindow::crashNow()
{
    TomahawkUtils::crash();
}


void
TomahawkWindow::toggleMenuBar() //SLOT
{
#ifndef Q_OS_MAC
    if ( menuBar()->isVisible() )
    {
        menuBar()->setVisible( false );
        ActionCollection::instance()->getAction( "toggleMenuBar" )->setText( tr( "Show Menu Bar" ) );
        m_compactMenuAction->setVisible( true );
    }
    else
    {
        m_compactMenuAction->setVisible( false );
        ActionCollection::instance()->getAction( "toggleMenuBar" )->setText( tr( "Hide Menu Bar" ) );
        menuBar()->setVisible( true );
    }
    balanceToolbar();
    saveSettings();
#endif
}


AudioControls*
TomahawkWindow::audioControls()
{
    return m_audioControls;
}


SourceTreeView*
TomahawkWindow::sourceTreeView() const
{
    return m_sourcetree;
}
