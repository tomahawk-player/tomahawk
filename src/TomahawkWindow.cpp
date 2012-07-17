/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2012, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include "accounts/AccountManager.h"
#include "sourcetree/SourceTreeView.h"
#include "network/Servent.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/ProxyStyle.h"
#include "utils/WidgetDragFilter.h"
#include "widgets/AnimatedSplitter.h"
#include "widgets/NewPlaylistWidget.h"
#include "widgets/SearchWidget.h"
#include "widgets/PlaylistTypeSelectorDialog.h"
#include "thirdparty/Qocoa/qsearchfield.h"
#include "playlist/dynamic/GeneratorInterface.h"
#include "playlist/PlaylistModel.h"
#include "playlist/PlaylistView.h"
#include "playlist/QueueView.h"
#include "jobview/JobStatusView.h"
#include "jobview/JobStatusModel.h"
#include "jobview/ErrorStatusMessage.h"
#include "jobview/JobStatusModel.h"

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
#include "libtomahawk/filemetadata/ScanManager.h"
#include "TomahawkApp.h"
#include "LoadXSPFDialog.h"

#ifdef Q_OS_WIN
    #include <qtsparkle/Updater>
    #ifndef THBN_CLICKED
        #define THBN_CLICKED    0x1800
    #endif
#endif

#include "utils/Logger.h"

using namespace Tomahawk;
using namespace Accounts;


TomahawkWindow::TomahawkWindow( QWidget* parent )
    : QMainWindow( parent )
#ifdef Q_OS_WIN
    , m_buttonCreatedID( RegisterWindowMessage( L"TaskbarButtonCreated" ) )
    , m_taskbarList(0)
#endif
    , ui( new Ui::TomahawkWindow )
    , m_searchWidget( 0 )
    , m_audioControls( new AudioControls( this ) )
    , m_trayIcon( new TomahawkTrayIcon( this ) )
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

    ui->menuApp->insertAction( ui->actionCreatePlaylist, ActionCollection::instance()->getAction( "togglePrivacy" ) );
    ui->menuApp->insertSeparator( ui->actionCreatePlaylist );

    applyPlatformTweaks();

    ui->centralWidget->setContentsMargins( 0, 0, 0, 0 );
    TomahawkUtils::unmarginLayout( ui->centralWidget->layout() );

    setupToolBar();
    setupSideBar();
    statusBar()->addPermanentWidget( m_audioControls, 1 );

    setupUpdateCheck();
    loadSettings();
    setupSignals();

    if ( qApp->arguments().contains( "--debug" ) )
    {
        ui->menu_Help->addSeparator();
        ui->menu_Help->addAction( "Crash now...", this, SLOT( crashNow() ) );
    }

    // set initial state
    onAccountDisconnected();
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
}


void
TomahawkWindow::saveSettings()
{
    TomahawkSettings* s = TomahawkSettings::instance();
    s->setMainWindowGeometry( saveGeometry() );
    s->setMainWindowState( saveState() );
    s->setMainWindowSplitterState( ui->splitter->saveState() );
}


void
TomahawkWindow::applyPlatformTweaks()
{
    // HACK QtCurve causes an infinite loop on startup. This is because setStyle calls setPalette, which calls ensureBaseStyle,
    // which loads QtCurve. QtCurve calls setPalette, which creates an infinite loop. The UI will look like CRAP with QtCurve, but
    // the user is asking for it explicitly... so he's gonna be stuck with an ugly UI.
    if ( !QString( qApp->style()->metaObject()->className() ).toLower().contains( "qtcurve" ) )
        qApp->setStyle( new ProxyStyle() );

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
    QToolBar* toolbar = addToolBar( "TomahawkToolbar" );
    toolbar->setObjectName( "TomahawkToolbar" );
    toolbar->setMovable( false );
    toolbar->setFloatable( false );
    toolbar->setIconSize( QSize( 22, 22 ) );
    toolbar->setToolButtonStyle( Qt::ToolButtonIconOnly );
    toolbar->setStyleSheet( "border-bottom: 0px" );

#ifdef Q_OS_MAC
    toolbar->installEventFilter( new WidgetDragFilter( toolbar ) );
#endif

    m_backAction = toolbar->addAction( QIcon( RESPATH "images/back.png" ), tr( "Back" ), ViewManager::instance(), SLOT( historyBack() ) );
    m_backAction->setToolTip( tr( "Go back one page" ) );
    m_forwardAction = toolbar->addAction( QIcon( RESPATH "images/forward.png" ), tr( "Forward" ), ViewManager::instance(), SLOT( historyForward() ) );
    m_forwardAction->setToolTip( tr( "Go forward one page" ) );

    QWidget* spacer = new QWidget( this );
    spacer->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );
    toolbar->addWidget( spacer );

    m_searchWidget = new QSearchField( this );
    m_searchWidget->setPlaceholderText( tr( "Global Search..." ) );
    m_searchWidget->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );
    m_searchWidget->setMaximumWidth( 340 );
    connect( m_searchWidget, SIGNAL( returnPressed() ), this, SLOT( onFilterEdited() ) );

    toolbar->addWidget( m_searchWidget );
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

    ui->actionShowOfflineSources->setChecked( TomahawkSettings::instance()->showOfflineSources() );
}


void
TomahawkWindow::setupUpdateCheck()
{
#ifndef Q_OS_MAC
    ui->menu_Help->insertSeparator( ui->actionAboutTomahawk );
#endif

#if defined( Q_OS_MAC ) && defined( HAVE_SPARKLE )
    QAction* checkForUpdates = ui->menu_Help->addAction( tr( "Check For Updates..." ) );
    checkForUpdates->setMenuRole( QAction::ApplicationSpecificRole );
    connect( checkForUpdates, SIGNAL( triggered( bool ) ), SLOT( checkForUpdates() ) );
#elif defined( Q_WS_WIN )
    QUrl updaterUrl;

    if ( qApp->arguments().contains( "--debug" ) )
        updaterUrl.setUrl( "http://download.tomahawk-player.org/sparklewin-debug" );
    else
        updaterUrl.setUrl( "http://download.tomahawk-player.org/sparklewin" );

    qtsparkle::Updater* updater = new qtsparkle::Updater( updaterUrl, this );
    Q_ASSERT( TomahawkUtils::nam() != 0 );
    updater->SetNetworkAccessManager( TomahawkUtils::nam() );
    updater->SetVersion( TomahawkUtils::appFriendlyVersion() );

    ui->menu_Help->addSeparator();
    QAction* checkForUpdates = ui->menu_Help->addAction( tr( "Check For Updates..." ) );
    connect( checkForUpdates, SIGNAL( triggered() ), updater, SLOT( CheckNow() ) );
#endif
}


#ifdef Q_OS_WIN
bool
TomahawkWindow::setupWindowsButtons()
{
    const GUID IID_ITaskbarList3 = { 0xea1afb91,0x9e28,0x4b86, { 0x90,0xe9,0x9e,0x9f,0x8a,0x5e,0xef,0xaf } };
    HRESULT hr = S_OK;

    QPixmap play( RESPATH "images/play-rest.png" );
    QPixmap back( RESPATH "images/back-rest.png" );
    QPixmap love( RESPATH "images/not-loved.png" );

    QTransform transform;
    transform.rotate( 180 );
    QPixmap next( back.transformed( transform ) );

    THUMBBUTTONMASK dwMask = THUMBBUTTONMASK( THB_ICON | THB_TOOLTIP | THB_FLAGS );
    m_thumbButtons[TP_PREVIOUS].dwMask = dwMask;
    m_thumbButtons[TP_PREVIOUS].iId = TP_PREVIOUS;
    m_thumbButtons[TP_PREVIOUS].hIcon = back.toWinHICON();
    m_thumbButtons[TP_PREVIOUS].dwFlags = THBF_ENABLED;
    m_thumbButtons[TP_PREVIOUS].szTip[ tr( "Back" ).toWCharArray( m_thumbButtons[TP_PREVIOUS].szTip ) ] = 0;

    m_thumbButtons[TP_PLAY_PAUSE].dwMask = dwMask;
    m_thumbButtons[TP_PLAY_PAUSE].iId = TP_PLAY_PAUSE;
    m_thumbButtons[TP_PLAY_PAUSE].hIcon = play.toWinHICON();
    m_thumbButtons[TP_PLAY_PAUSE].dwFlags = THBF_ENABLED;
    m_thumbButtons[TP_PLAY_PAUSE].szTip[ tr( "Play" ).toWCharArray( m_thumbButtons[TP_PLAY_PAUSE].szTip ) ] = 0;

    m_thumbButtons[TP_NEXT].dwMask = dwMask;
    m_thumbButtons[TP_NEXT].iId = TP_NEXT;
    m_thumbButtons[TP_NEXT].hIcon = next.toWinHICON();
    m_thumbButtons[TP_NEXT].dwFlags = THBF_ENABLED;
    m_thumbButtons[TP_NEXT].szTip[ tr( "Next" ).toWCharArray( m_thumbButtons[TP_NEXT].szTip ) ] = 0;

    m_thumbButtons[3].dwMask = dwMask;
    m_thumbButtons[3].iId = -1;
    m_thumbButtons[3].hIcon = 0;
    m_thumbButtons[3].dwFlags = THBF_NOBACKGROUND | THBF_DISABLED;
    m_thumbButtons[3].szTip[0] = 0;

    m_thumbButtons[TP_LOVE].dwMask = dwMask;
    m_thumbButtons[TP_LOVE].iId = TP_LOVE;
    m_thumbButtons[TP_LOVE].hIcon = love.toWinHICON();
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
}
#endif


void
TomahawkWindow::setupSignals()
{
    // <From PlaylistManager>
    connect( ViewManager::instance(), SIGNAL( repeatModeChanged( Tomahawk::PlaylistModes::RepeatMode ) ),
             m_audioControls,           SLOT( onRepeatModeChanged( Tomahawk::PlaylistModes::RepeatMode ) ) );
    connect( ViewManager::instance(), SIGNAL( shuffleModeChanged( bool ) ),
             m_audioControls,           SLOT( onShuffleModeChanged( bool ) ) );

    // <From AudioEngine>
    connect( AudioEngine::instance(), SIGNAL( error( AudioEngine::AudioErrorCode ) ), SLOT( onAudioEngineError( AudioEngine::AudioErrorCode ) ) );
    connect( AudioEngine::instance(), SIGNAL( loading( const Tomahawk::result_ptr& ) ), SLOT( onPlaybackLoading( const Tomahawk::result_ptr& ) ) );
    connect( AudioEngine::instance(), SIGNAL( started( Tomahawk::result_ptr ) ), SLOT( audioStarted() ) );
    connect( AudioEngine::instance(), SIGNAL( finished(Tomahawk::result_ptr) ), SLOT( audioFinished() ) );
    connect( AudioEngine::instance(), SIGNAL( resumed()), SLOT( audioStarted() ) );
    connect( AudioEngine::instance(), SIGNAL( paused() ), SLOT( audioPaused() ) );
    connect( AudioEngine::instance(), SIGNAL( stopped() ), SLOT( audioStopped() ) );

    // <Menu Items>
    //    connect( ui->actionAddPeerManually, SIGNAL( triggered() ), SLOT( addPeerManually() ) );
    connect( ui->actionPreferences, SIGNAL( triggered() ), SLOT( showSettingsDialog() ) );
    connect( ui->actionDiagnostics, SIGNAL( triggered() ), SLOT( showDiagnosticsDialog() ) );
    connect( ui->actionLegalInfo, SIGNAL( triggered() ), SLOT( legalInfo() ) );
    connect( ui->actionToggleConnect, SIGNAL( triggered() ), AccountManager::instance(), SLOT( toggleAccountsConnected() ) );
    connect( ui->actionUpdateCollection, SIGNAL( triggered() ), SLOT( updateCollectionManually() ) );
    connect( ui->actionRescanCollection, SIGNAL( triggered() ), SLOT( rescanCollectionManually() ) );
    connect( ui->actionLoadXSPF, SIGNAL( triggered() ), SLOT( loadSpiff() ));
    connect( ui->actionCreatePlaylist, SIGNAL( triggered() ), SLOT( createPlaylist() ));
    connect( ui->actionCreate_New_Station, SIGNAL( triggered() ), SLOT( createStation() ));
    connect( ui->actionAboutTomahawk, SIGNAL( triggered() ), SLOT( showAboutTomahawk() ) );
    connect( ui->actionExit, SIGNAL( triggered() ), qApp, SLOT( quit() ) );
    connect( ui->actionShowOfflineSources, SIGNAL( triggered() ), SLOT( showOfflineSources() ) );

    connect( ui->actionPlay, SIGNAL( triggered() ), AudioEngine::instance(), SLOT( playPause() ) );
    connect( ui->actionNext, SIGNAL( triggered() ), AudioEngine::instance(), SLOT( next() ) );
    connect( ui->actionPrevious, SIGNAL( triggered() ), AudioEngine::instance(), SLOT( previous() ) );

#if defined( Q_OS_MAC )
    connect( ui->actionMinimize, SIGNAL( triggered() ), SLOT( minimize() ) );
    connect( ui->actionZoom, SIGNAL( triggered() ), SLOT( maximize() ) );
#else
    ui->menuWindow->clear();
    ui->menuWindow->menuAction()->setVisible( false );
#endif

    // <AccountHandler>
    connect( AccountManager::instance(), SIGNAL( connected( Tomahawk::Accounts::Account* ) ), SLOT( onAccountConnected() ) );
    connect( AccountManager::instance(), SIGNAL( disconnected( Tomahawk::Accounts::Account* ) ), SLOT( onAccountDisconnected() ) );
    connect( AccountManager::instance(), SIGNAL( authError( Tomahawk::Accounts::Account* ) ), SLOT( onAccountError() ) );

    // Menus for accounts that support them
    connect( AccountManager::instance(), SIGNAL( added( Tomahawk::Accounts::Account* ) ), this, SLOT( onAccountAdded( Tomahawk::Accounts::Account* ) ) );
    foreach ( Account* account, AccountManager::instance()->accounts( Tomahawk::Accounts::SipType ) )
    {
        if ( !account || !account->sipPlugin() )
            continue;

        connect( account->sipPlugin(), SIGNAL( addMenu( QMenu* ) ), this, SLOT( pluginMenuAdded( QMenu* ) ) );
        connect( account->sipPlugin(), SIGNAL( removeMenu( QMenu* ) ), this, SLOT( pluginMenuRemoved( QMenu* ) ) );
    }

    connect( ViewManager::instance(), SIGNAL( historyBackAvailable( bool ) ), SLOT( onHistoryBackAvailable( bool ) ) );
    connect( ViewManager::instance(), SIGNAL( historyForwardAvailable( bool ) ), SLOT( onHistoryForwardAvailable( bool ) ) );
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
    ui->actionMinimize->setDisabled( false );
    ui->actionZoom->setDisabled( false );
#endif
}


void
TomahawkWindow::hideEvent( QHideEvent* e )
{
    QMainWindow::hideEvent( e );

#if defined( Q_OS_MAC )
    ui->actionMinimize->setDisabled( true );
    ui->actionZoom->setDisabled( true );
#endif
}


void
TomahawkWindow::keyPressEvent( QKeyEvent* e )
{
    bool accept = true;
#if ! defined ( Q_OS_MAC )
#define KEY_PRESSED Q_FUNC_INFO << "Multimedia Key Pressed:"
    switch( e->key() )
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

    switch( msg->message )
    {
    case WM_COMMAND:
        if ( HIWORD( msg->wParam ) == THBN_CLICKED )
        {
            switch( TB_STATES(LOWORD( msg->wParam )) )
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


void
TomahawkWindow::audioStateChanged( AudioState newState, AudioState oldState )
{
    if ( m_taskbarList == 0 )
        return;

    switch ( newState )
    {
        case AudioEngine::Playing:
        {
            QPixmap pause( RESPATH "images/pause-rest.png" );
            m_thumbButtons[TP_PLAY_PAUSE].hIcon = pause.toWinHICON();
            m_thumbButtons[TP_PLAY_PAUSE].szTip[ tr( "Pause" ).toWCharArray( m_thumbButtons[TP_PLAY_PAUSE].szTip ) ] = 0;
            updateWindowsLoveButton();

        }
        break;

        case AudioEngine::Paused:
        {
            QPixmap play( RESPATH "images/play-rest.png" );
            m_thumbButtons[TP_PLAY_PAUSE].hIcon = play.toWinHICON();
            m_thumbButtons[TP_PLAY_PAUSE].szTip[ tr( "Play" ).toWCharArray( m_thumbButtons[TP_PLAY_PAUSE].szTip ) ] = 0;
        }
        break;

        case AudioEngine::Stopped:
        {
            if ( !AudioEngine::instance()->currentTrack().isNull() )
            {
                disconnect(AudioEngine::instance()->currentTrack()->toQuery().data(),SIGNAL(socialActionsLoaded()),this,SLOT(updateWindowsLoveButton()));
            }
            QPixmap play( RESPATH "images/play-rest.png" );
            m_thumbButtons[TP_PLAY_PAUSE].hIcon = play.toWinHICON();
            m_thumbButtons[TP_PLAY_PAUSE].szTip[ tr( "Play" ).toWCharArray( m_thumbButtons[TP_PLAY_PAUSE].szTip ) ] = 0;

            QPixmap not_loved( RESPATH "images/not-loved.png" );
            m_thumbButtons[TP_LOVE].hIcon = not_loved.toWinHICON();
            m_thumbButtons[TP_LOVE].dwFlags = THBF_DISABLED;
        }
        break;

        default:
            return;
    }

    m_taskbarList->ThumbBarUpdateButtons( winId(), ARRAYSIZE( m_thumbButtons ), m_thumbButtons );
}

void
TomahawkWindow::updateWindowsLoveButton()
{
    if ( !AudioEngine::instance()->currentTrack().isNull() && AudioEngine::instance()->currentTrack()->toQuery()->loved() )
    {
        QPixmap loved( RESPATH "images/loved.png" );
        m_thumbButtons[TP_LOVE].hIcon = loved.toWinHICON();
        m_thumbButtons[TP_LOVE].szTip[ tr( "Unlove" ).toWCharArray( m_thumbButtons[TP_LOVE].szTip ) ] = 0;
    }
    else
    {
        QPixmap not_loved( RESPATH "images/not-loved.png" );
        m_thumbButtons[TP_LOVE].hIcon = not_loved.toWinHICON();
        m_thumbButtons[TP_LOVE].szTip[ tr( "Love" ).toWCharArray( m_thumbButtons[TP_LOVE].szTip ) ] = 0;
    }
    m_thumbButtons[TP_LOVE].dwFlags = THBF_ENABLED;
    m_taskbarList->ThumbBarUpdateButtons( winId(), ARRAYSIZE( m_thumbButtons ), m_thumbButtons );
}

#endif


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
    SettingsDialog win;
    win.exec();
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
TomahawkWindow::addPeerManually()
{
    TomahawkSettings* s = TomahawkSettings::instance();
    bool ok;
    QString addr = QInputDialog::getText( this, tr( "Connect To Peer" ),
                                                tr( "Enter peer address:" ), QLineEdit::Normal,
                                                s->value( "connip" ).toString(), &ok ); // FIXME
    if ( !ok )
        return;

    s->setValue( "connip", addr );
    QString ports = QInputDialog::getText( this, tr( "Connect To Peer" ),
                                                 tr( "Enter peer port:" ), QLineEdit::Normal,
                                                 s->value( "connport", "50210" ).toString(), &ok );
    if ( !ok )
        return;

    s->setValue( "connport", ports );
    int port = ports.toInt();
    QString key = QInputDialog::getText( this, tr( "Connect To Peer" ),
                                               tr( "Enter peer key:" ), QLineEdit::Normal,
                                               "whitelist", &ok );
    if ( !ok )
        return;

    qDebug() << "Attempting to connect to" << addr;
    Servent::instance()->connectToPeer( addr, port, key );
}


void
TomahawkWindow::pluginMenuAdded( QMenu* menu )
{
    ui->menuNetwork->addMenu( menu );
}


void
TomahawkWindow::pluginMenuRemoved( QMenu* menu )
{
    foreach ( QAction* action, ui->menuNetwork->actions() )
    {
        if ( action->menu() == menu )
        {
            ui->menuNetwork->removeAction( action );
            return;
        }
    }
}


void
TomahawkWindow::showOfflineSources()
{
    m_sourcetree->showOfflineSources( ui->actionShowOfflineSources->isChecked() );
    TomahawkSettings::instance()->setShowOfflineSources( ui->actionShowOfflineSources->isChecked() );
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
    QWeakPointer< LoadXSPFDialog > safe( diag );

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

    ui->actionPlay->setText( tr( "Pause" ) );
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
    ui->actionPlay->setText( tr( "Play" ) );
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
TomahawkWindow::onAccountConnected()
{
    ui->actionToggleConnect->setText( tr( "Go &offline" ) );
}


void
TomahawkWindow::onAccountDisconnected()
{
    ui->actionToggleConnect->setText( tr( "Go &online" ) );
}


void
TomahawkWindow::onAccountAdded( Account* acc )
{
    if ( !acc->types() & SipType || !acc->sipPlugin() )
        return;

    connect( acc->sipPlugin(), SIGNAL( addMenu( QMenu* ) ), this, SLOT( pluginMenuAdded( QMenu* ) ) );
    connect( acc->sipPlugin(), SIGNAL( removeMenu( QMenu* ) ), this, SLOT( pluginMenuRemoved( QMenu* ) ) );
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

    const QString copyright( tr( "Copyright 2010 - 2012" ) );
    const QString thanksto( tr( "Thanks to:" ) );

    desc = QString( "%1<br/>Christian Muehlhaeuser &lt;muesli@tomahawk-player.org&gt;<br/><br/>"
                    "%2 Leo Franchi, Jeff Mitchell, Dominik Schmidt, Jason Herskowitz, Alejandro Wainzinger, Hugo Lindstr&ouml;m, Syd Lawrence, Michael Zanetti, Harald Sitter, Steve Robertson" )
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
