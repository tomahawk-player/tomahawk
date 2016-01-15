/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2015, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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
#include <QDesktopWidget>
#include <QShowEvent>
#include <QHideEvent>
#include <QInputDialog>
#include <QPixmap>
#include <QPropertyAnimation>
#include <QLineEdit>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QShortcut>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>

#include "accounts/AccountManager.h"
#include "sourcetree/SourceTreeView.h"
#include "network/Servent.h"
#include "utils/TomahawkStyle.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/ProxyStyle.h"
#include "utils/WidgetDragFilter.h"
#include "utils/NetworkAccessManager.h"
#include "utils/M3uLoader.h"
#include "utils/JspfLoader.h"
#include "widgets/AccountsToolButton.h"
#include "widgets/AnimatedSplitter.h"
#include "widgets/ContainedMenuButton.h"
#include "thirdparty/Qocoa/qsearchfield.h"
#include "playlist/dynamic/GeneratorInterface.h"
#include "playlist/PlaylistModel.h"
#include "playlist/PlayableProxyModel.h"
#include "playlist/ContextView.h"
#include "playlist/TrackView.h"
#include "playlist/QueueView.h"
#include "jobview/JobStatusView.h"
#include "jobview/JobStatusModel.h"
#include "jobview/ErrorStatusMessage.h"
#include "jobview/JobStatusModel.h"
#include "sip/SipPlugin.h"
#include "filemetadata/ScanManager.h"
#include "viewpages/SearchViewPage.h"
#include "viewpages/whatsnew_0_8/WhatsNew_0_8.h"

#include "Playlist.h"
#include "Query.h"
#include "Artist.h"
#include "ViewManager.h"
#include "ActionCollection.h"
#include "AudioControls.h"
#include "dialogs/SettingsDialog.h"
#include "dialogs/DiagnosticsDialog.h"
#include "TomahawkSettings.h"
#include "SourceList.h"
#include "TomahawkTrayIcon.h"
#include "TomahawkApp.h"
#include "dialogs/LoadPlaylistDialog.h"
#include "utils/ImageRegistry.h"
#include "utils/Logger.h"
#include "utils/GuiHelpers.h"
#include "libtomahawk/widgets/ImageButton.h"

#include "config.h"

#if defined( Q_OS_WIN )
    #if defined ( QTSPARKLE_FOUND )
        #if QT_VERSION < QT_VERSION_CHECK( 5, 0, 0 )
            #include <qtsparkle/Updater>
        #else
            #include <qtsparkle-qt5/Updater>
        #endif
    #endif
    #include <shellapi.h>
#endif

using namespace Tomahawk;
using namespace Accounts;


TomahawkWindow::TomahawkWindow( QWidget* parent )
    : QMainWindow( parent )
    , TomahawkUtils::DpiScaler( this )
    , ui( new Ui::TomahawkWindow )
    , m_searchWidget( 0 )
    , m_trayIcon( 0 )
{
#ifndef Q_OS_MAC
    setWindowIcon( QIcon( RESPATH "icons/tomahawk-icon-128x128.png" ) );
#endif

    new ViewManager( this );
    QueueView* queueView = new QueueView();
    ViewManager::instance()->setQueue( queueView );
    AudioEngine::instance()->setQueue( queueView->view()->trackView()->proxyModel()->playlistInterface() );

    m_audioControls = new AudioControls( this );

    ui->setupUi( this );

    applyPlatformTweaks();

    ui->centralWidget->setContentsMargins( 0, 0, 0, 0 );
    TomahawkUtils::unmarginLayout( ui->centralWidget->layout() );

    if ( QSystemTrayIcon::isSystemTrayAvailable() )
    {
        m_trayIcon = new TomahawkTrayIcon( this );
    }

    setupMenuBar();
    setupToolBar();
    setupSideBar();
    setupStatusBar();

    setupUpdateCheck();
    loadSettings();
    setupSignals();
    setupShortcuts();

#ifdef Q_OS_WIN
    connect( AudioEngine::instance(), SIGNAL( stateChanged( AudioState, AudioState ) ), SLOT( audioStateChanged( AudioState, AudioState ) ) );
    setupWindowsButtons();
#endif

    if ( qApp->arguments().contains( "--debug" ) )
    {
        connect( ActionCollection::instance()->getAction( "crashNow" ), SIGNAL( triggered() ), SLOT( crashNow() ) );
    }

    // set initial state
    audioStopped();

    if ( TomahawkSettings::instance()->fullscreenEnabled() )
    {
        // Window must be fully constructed to toggle fullscreen mode. Queue it up.
        QTimer::singleShot( 0, this, SLOT( toggleFullscreen() ) );
    }
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
    else
    {
        // Set default window geometry
        resize( QDesktopWidget().availableGeometry( this ).size() * 0.8 );
    }

    if ( !s->mainWindowState().isEmpty() )
        restoreState( s->mainWindowState() );
    if ( !s->mainWindowSplitterState().isEmpty() )
        ui->splitter->restoreState( s->mainWindowSplitterState() );

    // Always set stretch factor. If user hasn't manually set splitter sizes,
    // this will ensure a sane default on all startups. If the user has, the manual
    // size will override the default stretching
    ui->splitter->setHandleWidth( 3 );
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

    ActionCollection::instance()->getAction( "showOfflineSources" )->setChecked( TomahawkSettings::instance()->showOfflineSources() );
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
#ifdef Q_OS_MAC
    Tomahawk::setupToolBarMac( this );

    m_backAction = new QAction( this );
    m_forwardAction = new QAction( this );

    connect( this, SIGNAL( searchEdited( QString ) ), SLOT( onSearch( QString ) ) );
#else
    m_toolbar = addToolBar( "TomahawkToolbar" );
    m_toolbar->setObjectName( "TomahawkToolbar" );
    m_toolbar->setMovable( false );
    m_toolbar->setFloatable( false );
    m_toolbar->setIconSize( scaled( 22, 22 ) );
    m_toolbar->setToolButtonStyle( Qt::ToolButtonIconOnly );
    m_toolbar->setStyleSheet( "border-bottom: 0px" );
    // If the toolbar is hidden accidentally it causes trouble on Unity because the user can't
    // easily bring it back (TWK-1046). So we just prevent the user from hiding the toolbar.
    // This should not affect Mac users.
    m_toolbar->setContextMenuPolicy( Qt::PreventContextMenu );

    m_backAction = m_toolbar->addAction( ImageRegistry::instance()->pixmap( RESPATH "images/back.svg", m_toolbar->iconSize() ),
                                         tr( "Back" ),
                                         ViewManager::instance(),
                                         SLOT( historyBack() ) );
    m_forwardAction = m_toolbar->addAction( ImageRegistry::instance()->pixmap( RESPATH "images/forward.svg", m_toolbar->iconSize() ),
                                            tr( "Forward" ),
                                            ViewManager::instance(),
                                            SLOT( historyForward() ) );

    m_toolbarLeftBalancer = new QWidget( this );
    m_toolbarLeftBalancer->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Preferred );
    m_toolbarLeftBalancer->setFixedWidth( 0 );
    m_toolbar->addWidget( m_toolbarLeftBalancer )->setProperty( "kind", QString( "spacer" ) );

    QWidget* toolbarLeftSpacer = new QWidget( this );
    toolbarLeftSpacer->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );
    m_toolbar->addWidget( toolbarLeftSpacer )->setProperty( "kind", QString( "spacer" ) );

    m_searchWidget = new QSearchField( this );
    m_searchWidget->setPlaceholderText( tr( "Search" ) );
    m_searchWidget->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Preferred );
    m_searchWidget->setFixedWidth( scaledX( 340 ) );
    connect( m_searchWidget, SIGNAL( returnPressed() ), SLOT( onFilterEdited() ) );

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
    connect( m_accountsButton, SIGNAL( widthChanged() ), SLOT( balanceToolbar() ) );

    ContainedMenuButton* compactMenuButton = new ContainedMenuButton( m_toolbar );
    compactMenuButton->setIcon( ImageRegistry::instance()->pixmap( RESPATH "images/configure.svg", m_toolbar->iconSize() ) );
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

    balanceToolbar();
#endif

    m_backAction->setToolTip( tr( "Go back one page" ) );
    m_forwardAction->setToolTip( tr( "Go forward one page" ) );
#ifdef Q_OS_MAC
    m_backAction->setShortcut( QKeySequence( "Ctrl+Left" ) );
    m_forwardAction->setShortcut( QKeySequence( "Ctrl+Right" ) );
#else
    m_backAction->setShortcut( QKeySequence( "Alt+Left" ) );
    m_forwardAction->setShortcut( QKeySequence( "Alt+Right" ) );
#endif

    onHistoryBackAvailable( false );
    onHistoryForwardAvailable( false );
}


void
TomahawkWindow::balanceToolbar()
{
    int leftActionsWidth = 0;
    int rightActionsWidth = 0;
    bool flip = false;
    foreach ( QAction* action, m_toolbar->actions() )
    {
        if ( action->property( "kind" ) == QString( "spacer" ) || !action->isVisible() )
        {
            continue;
        }
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
TomahawkWindow::toggleLoved()
{
    if ( !AudioEngine::instance()->currentTrack().isNull() )
    {
        AudioEngine::instance()->currentTrack()->track()->setLoved( !AudioEngine::instance()->currentTrack()->track()->loved() );
#ifdef Q_OS_WIN
        updateWindowsLoveButton();
#endif
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

    m_sidebar->addWidget( m_sourcetree );
    m_sidebar->addWidget( jobsView );

//    m_sidebar->setGreedyWidget( 1 );
    m_sidebar->hide( 1, false );
    m_sidebar->hide( 2, false );

    sidebarWidget->layout()->addWidget( m_sidebar );
    sidebarWidget->setContentsMargins( 0, 0, 0, 0 );
    sidebarWidget->layout()->setContentsMargins( 0, 0, 0, 0 );
    sidebarWidget->layout()->setMargin( 0 );

#ifndef Q_OS_MAC
    sidebarWidget->layout()->setSpacing( 0 );
#endif

    ui->splitter->addWidget( sidebarWidget );
    ui->splitter->addWidget( ViewManager::instance()->widget() );
    ui->splitter->setCollapsible( 0, false );
    ui->splitter->setCollapsible( 1, false );
}


void
TomahawkWindow::setupStatusBar()
{
    statusBar()->hide();
    setStatusBar( 0 );

    ui->centralWidget->layout()->addWidget( m_audioControls );
}


void
TomahawkWindow::setupShortcuts()
{
    {
        // Use Ctrl+F to focus the searchWidget
        QShortcut* shortcut = new QShortcut( QKeySequence( QKeySequence::Find ), this );
        QObject::connect( shortcut, SIGNAL( activated() ), m_searchWidget, SLOT( setFocus() ) );
    }
    {
        // Use Ctrl+W to close current page
        QShortcut* shortcut = new QShortcut( QKeySequence( QKeySequence::Close ), this );
        QObject::connect( shortcut, SIGNAL( activated() ), ViewManager::instance(), SLOT( destroyCurrentPage() ) );
    }
    {
        // Ctrl Up for raising the volume
        QShortcut* shortcut = new QShortcut( QKeySequence( QKeySequence( "Ctrl+Up" ) ), this );
        QObject::connect( shortcut, SIGNAL( activated() ), AudioEngine::instance(), SLOT( raiseVolume() ) );
    }
    {
        // Ctrl Down for lowering the volume
        QShortcut* shortcut = new QShortcut( QKeySequence( QKeySequence( "Ctrl+Down" ) ), this );
        QObject::connect( shortcut, SIGNAL( activated() ), AudioEngine::instance(), SLOT( lowerVolume() ) );
    }
}


void
TomahawkWindow::setupUpdateCheck()
{
#if defined( Q_OS_MAC ) && defined( HAVE_SPARKLE )
    connect( ActionCollection::instance()->getAction( "checkForUpdates" ), SIGNAL( triggered( bool ) ),
             SLOT( checkForUpdates() ) );
    #elif defined( Q_OS_WIN ) && defined( QTSPARKLE_FOUND )
    QUrl updaterUrl;

    if ( qApp->arguments().contains( "--debug" ) )
        updaterUrl.setUrl( "http://download.tomahawk-player.org/sparklewin-debug" );
    else
        updaterUrl.setUrl( "http://download.tomahawk-player.org/sparklewin" );

    qtsparkle::Updater* updater = new qtsparkle::Updater( updaterUrl, this );
    Q_ASSERT( Tomahawk::Utils::nam() != 0 );
    updater->SetNetworkAccessManager( Tomahawk::Utils::nam() );
    updater->SetVersion( TomahawkUtils::appFriendlyVersion() );

    connect( ActionCollection::instance()->getAction( "checkForUpdates" ), SIGNAL( triggered() ),
             updater, SLOT( CheckNow() ) );
#endif
}


#ifdef Q_OS_WIN
void
TomahawkWindow::setupWindowsButtons()
{
    m_taskbarList = new QWinThumbnailToolBar( this );
    m_taskbarList->setWindow( this->windowHandle() );
    updatePreview();

    QWinThumbnailToolButton *back = new QWinThumbnailToolButton( m_taskbarList );
    back->setToolTip( tr( "Back" ) );
    back->setIcon( thumbIcon( TomahawkUtils::PrevButton ) );
    connect( back, SIGNAL( clicked() ) , AudioEngine::instance() , SLOT( previous() ) );
    m_taskbarList->addButton(back);


    QWinThumbnailToolButton *play = new QWinThumbnailToolButton( m_taskbarList );
    play->setToolTip( tr( "Play" ) );
    play->setIcon( thumbIcon( TomahawkUtils::PlayButton ) );
    connect( play, SIGNAL( clicked() ) , AudioEngine::instance() , SLOT( playPause() ) );
    m_taskbarList->addButton(play);

    QWinThumbnailToolButton *next = new QWinThumbnailToolButton( m_taskbarList );
    next->setToolTip( tr( "Next" ) );
    next->setIcon( thumbIcon( TomahawkUtils::NextButton ) );
    connect( next, SIGNAL( clicked() ) , AudioEngine::instance() , SLOT( next() ) );
    m_taskbarList->addButton(next);


    QWinThumbnailToolButton *space = new QWinThumbnailToolButton( m_taskbarList );
    space->setVisible( true );
    space->setFlat( true );
    m_taskbarList->addButton(space);


    QWinThumbnailToolButton *love = new QWinThumbnailToolButton( m_taskbarList );
    love->setToolTip( tr( "Love" ) );
    love->setIcon( thumbIcon( TomahawkUtils::NotLoved ) );
    love->setInteractive( false );
    connect( love , SIGNAL( clicked() ) , this , SLOT( toggleLoved() ) );
    m_taskbarList->addButton(love);
}

void
TomahawkWindow::updatePreview()
{
    const QSize size = QDesktopWidget().availableGeometry().size();
    const qreal margin = size.height() * 0.05;
    const QSize coverSize( size.height() - 2 * margin , size.height() - 2 * margin);
    QPixmap cover;
    QString title( qApp->applicationName() );
    if ( !m_currentTrack.isNull() ) {
        cover = m_currentTrack->track()->albumPtr()->cover( coverSize , false );
        title = tr( "%1<br><br><b>%2</b>", "track, artist name" ).arg( m_currentTrack->track()->track(), m_currentTrack->track()->artist() );
    }
    if ( cover.isNull() ) {
        cover = TomahawkUtils::defaultPixmap( TomahawkUtils::DefaultAlbumCover , TomahawkUtils::Original, coverSize );
    }

    QPixmap thumb( size );
    thumb.fill( QColor( "#FF004C" ) );

    QPainter paint( &thumb );

    QPen pen = paint.pen();
    pen.setColor( Qt::white );
    pen.setWidth( size.height() * 0.01 );
    paint.setPen( pen );

    paint.drawPixmap(margin , margin , coverSize.width() , coverSize.height() , cover );
    paint.drawRect( margin , margin , coverSize.width() , coverSize.height() );
    paint.drawRect( 0 , 0 , size.width() , size.height() );

    QTextDocument doc;

    QFont font = paint.font();
    font.setPixelSize( size.height() * 0.1 );
    doc.setDefaultFont( font );

    doc.setPageSize( QSize( size.width() - 2 * margin - coverSize.width() , size.height() - 2 * margin ) );
    doc.setHtml( QString( "<center><font color=\"white\">%1</font></center>" ).arg( title ));

    paint.save();
    paint.translate( coverSize.width() + 2 * margin , ( size.height() - doc.size().height() ) / 2);

    doc.drawContents( &paint );

    paint.restore();

    m_taskbarList->setIconicThumbnailPixmap( thumb );
    m_taskbarList->setIconicLivePreviewPixmap( thumb );
}

void
TomahawkWindow::updateWindowsLoveButton()
{
    QWinThumbnailToolButton *love = m_taskbarList->buttons()[ TP_LOVE ];

    if ( !AudioEngine::instance()->currentTrack().isNull() )
    {
        love->setInteractive(true);
        if ( AudioEngine::instance()->currentTrack()->track()->loved() )
        {
            love->setIcon(thumbIcon(TomahawkUtils::Loved));
            love->setToolTip( tr( "Unlove" ) );
        }
        else
        {
            love->setIcon( thumbIcon(TomahawkUtils::NotLoved) );
            love->setToolTip( tr( "Love" ) );
        }
    }
    else
    {
        love->setInteractive(false);
        love->setIcon( thumbIcon(TomahawkUtils::NotLoved) );
        love->setToolTip( tr( "Love" ) );
    }
}

QIcon
TomahawkWindow::thumbIcon(TomahawkUtils::ImageType type)
{
    return  TomahawkUtils::defaultPixmap( type , TomahawkUtils::Original, QSize( 40, 40 ) );
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
    connect( ac->getAction( "reportBug" ), SIGNAL( triggered() ), SLOT( reportBug() ) );
    connect( ac->getAction( "getSupport" ), SIGNAL( triggered() ), SLOT( getSupport() ) );
    connect( ac->getAction( "helpTranslate" ), SIGNAL( triggered() ), SLOT( helpTranslate() ) );
    connect( ac->getAction( "openLogfile" ), SIGNAL( triggered() ), SLOT( openLogfile() ) );
    connect( ac->getAction( "updateCollection" ), SIGNAL( triggered() ), SLOT( updateCollectionManually() ) );
    connect( ac->getAction( "rescanCollection" ), SIGNAL( triggered() ), SLOT( rescanCollectionManually() ) );
    connect( ac->getAction( "importPlaylist" ), SIGNAL( triggered() ), SLOT( loadPlaylist() ) );
    connect( ac->getAction( "whatsnew_0_8" ), SIGNAL( triggered() ), SLOT( showWhatsNew_0_8() ) );
    connect( ac->getAction( "aboutTomahawk" ), SIGNAL( triggered() ), SLOT( showAboutTomahawk() ) );
    connect( ac->getAction( "quit" ), SIGNAL( triggered() ), qApp, SLOT( quit() ) );
    connect( ac->getAction( "showOfflineSources" ), SIGNAL( triggered() ), SLOT( showOfflineSources() ) );
    connect( ac->getAction( "createPlaylist" ), SIGNAL( triggered() ), SLOT( createPlaylist() ) );
    connect( ac->getAction( "createStation" ), SIGNAL( triggered() ), SLOT( createStation() ) );

#if defined( Q_OS_MAC )
    connect( ac->getAction( "minimize" ), SIGNAL( triggered() ), SLOT( minimize() ) );
    connect( ac->getAction( "zoom" ), SIGNAL( triggered() ), SLOT( maximize() ) );
    connect( ac->getAction( "fullscreen" ), SIGNAL( triggered() ), SLOT( toggleFullscreen() ) );
#else
    connect( ac->getAction( "toggleMenuBar" ), SIGNAL( triggered() ), SLOT( toggleMenuBar() ) );
#endif

    connect( ViewManager::instance(), SIGNAL( historyBackAvailable( bool ) ), SLOT( onHistoryBackAvailable( bool ) ) );
    connect( ViewManager::instance(), SIGNAL( historyForwardAvailable( bool ) ), SLOT( onHistoryForwardAvailable( bool ) ) );
}


void
TomahawkWindow::setupMenuBar()
{
    // Always create a menubar, but only create a compactMenu on Windows and X11
    m_menuBar = ActionCollection::instance()->createMenuBar( this );
    m_menuBar->setFont( TomahawkUtils::systemFont() );

    setMenuBar( m_menuBar );
#ifndef Q_OS_MAC
    m_compactMainMenu = ActionCollection::instance()->createCompactMenu( this );
#endif
}


bool
TomahawkWindow::eventFilter( QObject* obj, QEvent* event )
{
    if ( event->type() == QEvent::MouseButtonPress )
    {
        QMouseEvent* me = static_cast<QMouseEvent*>(event);
        switch ( me->button() )
        {
            case Qt::XButton1:
                m_backAction->trigger();
                break;

            case Qt::XButton2:
                m_forwardAction->trigger();
                break;

            default:
                break;
        }
    }

    return QObject::eventFilter( obj, event );
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
#endif

    QMainWindow::closeEvent( e );
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

void
TomahawkWindow::audioStateChanged( AudioState newState, AudioState oldState )
{
    Q_UNUSED(oldState);
#ifndef Q_OS_WIN
    Q_UNUSED(newState);
#else
    updatePreview();
        
    QWinThumbnailToolButton *play = m_taskbarList->buttons()[ TP_PLAY_PAUSE ];
    switch ( newState )
    {
    case AudioEngine::Playing:
    {
        play->setIcon( thumbIcon(TomahawkUtils::PauseButton) );
        play->setToolTip( tr( "Pause" ) );
        updateWindowsLoveButton();
    }
        break;

    case AudioEngine::Paused:
    {
        play->setIcon( thumbIcon(TomahawkUtils::PlayButton) );
        play->setToolTip( tr( "Play" ) );
    }
        break;

    case AudioEngine::Stopped:
    {
        if ( !AudioEngine::instance()->currentTrack().isNull() )
        {
            disconnect( AudioEngine::instance()->currentTrack()->track().data(), SIGNAL( socialActionsLoaded() ), this, SLOT( updateWindowsLoveButton() ) );
        }

        play->setIcon( thumbIcon(TomahawkUtils::PlayButton) );
        play->setToolTip( tr( "Play" ) );

        QWinThumbnailToolButton *love = m_taskbarList->buttons()[ TP_LOVE ];
        love->setIcon( thumbIcon(TomahawkUtils::NotLoved) );
        love->setInteractive( false );
    }
        break;

    default:
        return;
    }
#endif//Q_OS_WIN
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
    if ( m_settingsDialog )
    {
        m_settingsDialog->show();
        return;
    }

    m_settingsDialog = new SettingsDialog;
    // This needs to be a QueuedConnection, so that deleteLater() actually works.
    connect( m_settingsDialog.data(), SIGNAL( finished( bool ) ),
             m_settingsDialog.data(), SLOT( deleteLater() ), Qt::QueuedConnection );

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
TomahawkWindow::getSupport()
{
    QDesktopServices::openUrl( QUrl( "https://tomahawk.uservoice.com" ) );
}


void
TomahawkWindow::reportBug()
{
    QDesktopServices::openUrl( QUrl( "https://bugs.tomahawk-player.org" ) );
}


void
TomahawkWindow::helpTranslate()
{
    QDesktopServices::openUrl( QUrl( "https://www.transifex.com/projects/p/tomahawk/" ) );
}


void
TomahawkWindow::openLogfile()
{
#ifdef WIN32
    ShellExecuteW( 0, 0, (LPCWSTR)TomahawkUtils::logFilePath().utf16(), 0, 0, SW_SHOWNORMAL );
#else
    QDesktopServices::openUrl( QUrl::fromLocalFile( TomahawkUtils::logFilePath() ) );
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
    TomahawkSettings::instance()->setFullscreenEnabled( true );
//    statusBar()->setSizeGripEnabled( false );

    // Since we just disabled the size-grip the entire statusbar will shift a bit to the right
    // The volume bar would now have no margin to the right screen edge. Prevent that.
//    QMargins margins = statusBar()->contentsMargins();
//    margins.setRight( 24 );
//    statusBar()->setContentsMargins( margins );

#if defined( Q_OS_MAC )
    ActionCollection::instance()->getAction( "fullscreen" )->setText( tr( "Exit Full Screen" ) );
#endif
}


void
TomahawkWindow::fullScreenExited()
{
    TomahawkSettings::instance()->setFullscreenEnabled( false );
//    statusBar()->setSizeGripEnabled( true );

    // Since we just enabled the size-grip the entire statusbar will shift a bit to the left
    // The volume bar would now have too big a margin to the right screen edge. Prevent that.
//    QMargins margins = statusBar()->contentsMargins();
//    margins.setRight( 0 );
//    statusBar()->setContentsMargins( margins );

#if defined( Q_OS_MAC )
    ActionCollection::instance()->getAction( "fullscreen" )->setText( tr( "Enter Full Screen" ) );
#endif
}


void
TomahawkWindow::loadPlaylist()
{
    LoadPlaylistDialog* diag = new LoadPlaylistDialog( this, Qt::Sheet );
#ifdef Q_OS_MAC
    connect( diag, SIGNAL( finished( int ) ), this, SLOT( loadPlaylistFinished( int ) ) );
    diag->show();
#else
    QPointer< LoadPlaylistDialog > safe( diag );

    int ret = diag->exec();
    if ( !safe.isNull() && ret == QDialog::Accepted )
    {
        importPlaylist( safe->url(), safe->autoUpdate() );
    }
#endif
}


void
TomahawkWindow::loadPlaylistFinished( int ret )
{
    LoadPlaylistDialog* d = qobject_cast< LoadPlaylistDialog* >( sender() );
    Q_ASSERT( d );
    if ( ret == QDialog::Accepted )
    {
        importPlaylist( d->url(), d->autoUpdate() );
    }
    d->deleteLater();
}


void
TomahawkWindow::importPlaylist( const QString& url, bool autoUpdate )
{
    const QUrl u = QUrl::fromUserInput( url );

    const QString ext = u.toString().toLower();
    if ( ext.endsWith( "m3u" ) )
    {
        M3uLoader* loader = new M3uLoader( u.toString(), true );
        loader->parse();
    }
    else if ( ext.endsWith( "jspf" ) )
    {
        JSPFLoader* loader = new JSPFLoader( true );
        connect( loader, SIGNAL( failed() ), SLOT( onJSPFError() ) );
        connect( loader, SIGNAL( ok( Tomahawk::playlist_ptr ) ), SLOT( onNewPlaylistOk( Tomahawk::playlist_ptr ) ) );
        loader->load( u );
    }
    else
    {
        XSPFLoader* loader = new XSPFLoader( true, autoUpdate );
        connect( loader, SIGNAL( error( XSPFLoader::XSPFErrorCode ) ), SLOT( onXSPFError( XSPFLoader::XSPFErrorCode ) ) );
        connect( loader, SIGNAL( ok( Tomahawk::playlist_ptr ) ), SLOT( onNewPlaylistOk( Tomahawk::playlist_ptr ) ) );
        loader->load( u );
    }
}


void
TomahawkWindow::onNewPlaylistOk( const Tomahawk::playlist_ptr& pl )
{
    ViewManager::instance()->show( pl );
}


void
TomahawkWindow::onXSPFError( XSPFLoader::XSPFErrorCode error )
{
    QString msg;
    switch ( error )
    {
        case XSPFLoader::ParseError:
            msg = tr( "This is not a valid XSPF playlist." );
            break;

        case XSPFLoader::InvalidTrackError:
            msg = tr( "Some tracks in the playlist do not contain an artist and a title. They will be ignored." );
            break;
        default:
            return;
    }
    JobStatusView::instance()->model()->addJob( new ErrorStatusMessage( msg, 15 ) );
}


void
TomahawkWindow::onJSPFError()
{
    JobStatusView::instance()->model()->addJob( new ErrorStatusMessage( tr( "Failed to load JSPF playlist"), 15 ) );
}


void
TomahawkWindow::onAudioEngineError( AudioEngine::AudioErrorCode /* error */ )
{
    QString msg = tr( "Sorry, there is a problem accessing your audio device or the desired track, current track will be skipped." );
    tLog() << msg;
    JobStatusView::instance()->model()->addJob( new ErrorStatusMessage( msg, 15 ) );
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
        QList< dynplaylist_ptr > pls = SourceList::instance()->getLocal()->dbCollection()->stations();
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

    QString info  = ""; // FIXME
    QString creator = ""; // FIXME

    dynplaylist_ptr playlist = DynamicPlaylist::create( SourceList::instance()->getLocal(), uuid(), playlistName, info, creator, OnDemand, false );
    playlist->setMode( OnDemand );
    playlist->createNewRevision( uuid(), playlist->currentrevision(), playlist->type(), playlist->generator()->controls() );

    ViewManager::instance()->show( playlist );
}


void
TomahawkWindow::createPlaylist()
{
    QString title = tr( "Playlist" );
    bool ok;
    QString playlistName = QInputDialog( this, Qt::Sheet ).getText( this, tr( "Create New Playlist" ), tr( "Name:" ), QLineEdit::Normal, title, &ok );
    if ( !ok )
        return;

    if ( playlistName.isEmpty() || playlistName == title )
    {
        QList< playlist_ptr > pls = SourceList::instance()->getLocal()->dbCollection()->playlists();
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

    QString info  = ""; // FIXME?
    QString creator = ""; // FIXME?

    playlist_ptr playlist = Tomahawk::Playlist::create( SourceList::instance()->getLocal(), uuid(), playlistName, info, creator, false, QList< query_ptr>() );
    ViewManager::instance()->show( playlist );
}


void
TomahawkWindow::audioStarted()
{
    ActionCollection::instance()->getAction( "playPause" )->setIcon( ImageRegistry::instance()->icon( RESPATH "images/pause.svg" ) );
    ActionCollection::instance()->getAction( "playPause" )->setText( tr( "Pause" ) );
    ActionCollection::instance()->getAction( "stop" )->setEnabled( true );

#ifdef Q_OS_WIN
    connect( AudioEngine::instance()->currentTrack()->track().data(), SIGNAL( socialActionsLoaded() ), SLOT( updateWindowsLoveButton() ) );
#endif
}


void
TomahawkWindow::audioFinished()
{
#ifdef Q_OS_WIN
    disconnect( AudioEngine::instance()->currentTrack()->track().data(), SIGNAL( socialActionsLoaded() ), this, SLOT( updateWindowsLoveButton() ) );
#endif
}


void
TomahawkWindow::audioPaused()
{
    ActionCollection::instance()->getAction( "playPause" )->setIcon( ImageRegistry::instance()->icon( RESPATH "images/play.svg" ) );
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
TomahawkWindow::onPlaybackLoading( const Tomahawk::result_ptr result )
{
    m_currentTrack = result;
    setWindowTitle( m_windowTitle );
}


void
TomahawkWindow::setWindowTitle( const QString& title )
{
    m_windowTitle = title;

    if ( m_currentTrack.isNull() )
        QMainWindow::setWindowTitle( title );
    else
    {
        QString s = tr( "%1 by %2", "track, artist name" ).arg( m_currentTrack->track()->track(), m_currentTrack->track()->artist() );
        QMainWindow::setWindowTitle( tr( "%1 - %2", "current track, some window title" ).arg( s, title ) );
    }
}


void
TomahawkWindow::showAboutTomahawk()
{
    QString head, desc;

#ifdef QT_DEBUG
    head = tr( "<h2><b>%applicationName %1<br/>(%2)</h2>" )
         .arg( TomahawkUtils::appFriendlyVersion() )
         .arg( qApp->applicationVersion() );
#else
    head = tr( "<h2><b>%applicationName %1</h2>" )
         .arg( TomahawkUtils::appFriendlyVersion() );
#endif

    const QString copyright( tr( "Copyright 2010 - 2015" ) );
    const QString thanksto( tr( "Thanks to:" ) );

    desc = QString( "https://tomahawk-player.org<br/><br/>%1<br/><br/>Christian Muehlhaeuser &lt;muesli@tomahawk-player.org&gt;<br/><br/>"
    "%2 Leo Franchi, Jeff Mitchell, Dominik Schmidt, Jason Herskowitz, Alejandro Wainzinger, Hugo Lindstr&ouml;m, Michael Zanetti, Teo Mrnjavac, Christopher Reichert, Uwe L. Korn, Patrick von Reth, Harald Sitter, Syd Lawrence, Jordi Verd&uacute; Orts" )
              .arg( copyright )
              .arg( thanksto );

    QMessageBox::about( this, tr( "About %applicationName" ), head + desc );
}


void
TomahawkWindow::showWhatsNew_0_8()
{
    ViewManager::instance()->showDynamicPage( Tomahawk::Widgets::WHATSNEW_0_8_VIEWPAGE_NAME );
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
    {
        if ( search.startsWith( "tomahawk:" ) )
        {
            APP->loadUrl( search );
        }
        else
        {
            ViewManager::instance()->show( new SearchWidget( search, this ) );
        }
    }
}


void
TomahawkWindow::onFilterEdited()
{
    onSearch( m_searchWidget->text() );
    m_searchWidget->clear();
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
TomahawkWindow::toggleFullscreen()
{
    tDebug() << Q_FUNC_INFO;

#if defined( Q_OS_MAC )
    Tomahawk::toggleFullscreen();
#endif
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
