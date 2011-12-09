/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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
#include "ui_tomahawkwindow.h"

#include <QAction>
#include <QCloseEvent>
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

#include "playlist.h"
#include "query.h"
#include "artist.h"
#include "viewmanager.h"
#include "sip/SipHandler.h"
#include "sourcetree/sourcetreeview.h"
#include "network/servent.h"
#include "utils/proxystyle.h"
#include "widgets/animatedsplitter.h"
#include "widgets/newplaylistwidget.h"
#include "widgets/searchwidget.h"
#include "widgets/playlisttypeselectordlg.h"
#include "thirdparty/Qocoa/qsearchfield.h"
#include "playlist/dynamic/GeneratorInterface.h"
#include "playlist/playlistmodel.h"
#include "playlist/playlistview.h"
#include "playlist/queueview.h"

#include "audiocontrols.h"
#include "settingsdialog.h"
#include "diagnosticsdialog.h"
#include "tomahawksettings.h"
#include "sourcelist.h"
#include "jobview/JobStatusView.h"
#include "tomahawktrayicon.h"
#include "scanmanager.h"
#include "tomahawkapp.h"

#ifdef Q_WS_WIN
#include <qtsparkle/Updater>
#endif

#include "utils/logger.h"
#include "jobview/JobStatusModel.h"
#include "LoadXSPFDialog.h"
#include <actioncollection.h>

using namespace Tomahawk;


TomahawkWindow::TomahawkWindow( QWidget* parent )
    : QMainWindow( parent )
    , ui( new Ui::TomahawkWindow )
    , m_searchWidget( 0 )
    , m_audioControls( new AudioControls( this ) )
    , m_trayIcon( new TomahawkTrayIcon( this ) )
{
    setWindowIcon( QIcon( RESPATH "icons/tomahawk-icon-128x128.png" ) );

    ViewManager* vm = new ViewManager( this );
    connect( vm, SIGNAL( showQueueRequested() ), SLOT( showQueue() ) );
    connect( vm, SIGNAL( hideQueueRequested() ), SLOT( hideQueue() ) );

    ui->setupUi( this );

    ui->menuApp->insertAction( ui->actionCreatePlaylist, ActionCollection::instance()->getAction( "togglePrivacy" ) );
    ui->menuApp->insertSeparator( ui->actionCreatePlaylist );

    applyPlatformTweaks();

    ui->centralWidget->setContentsMargins( 0, 0, 0, 0 );
    TomahawkUtils::unmarginLayout( ui->centralWidget->layout() );

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
    onSipDisconnected();
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
    if ( !s->mainWindowSplitterState().isEmpty() )
        ui->splitter->restoreState( s->mainWindowSplitterState() );
    else
    {
        ui->splitter->setStretchFactor( 0, 0 );
        ui->splitter->setStretchFactor( 1, 1 );
    }

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

#ifdef Q_WS_MAC
    setUnifiedTitleAndToolBarOnMac( true );
    delete ui->hline1;
    delete ui->hline2;
#else
    ui->hline1->setStyleSheet( "border: 1px solid gray;" );
    ui->hline2->setStyleSheet( "border: 1px solid gray;" );
#endif
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

    m_searchWidget = new QSearchField( m_sidebar );
    m_searchWidget->setPlaceholderText( tr( "Global Search..." ) );
    connect( m_searchWidget, SIGNAL( returnPressed() ), this, SLOT( onFilterEdited() ) );

    m_sourcetree = new SourceTreeView();
    JobStatusView* jobsView = new JobStatusView( m_sidebar );
    m_jobsModel = new JobStatusModel( jobsView );
    jobsView->setModel( m_jobsModel );

    m_queueView = new QueueView( m_sidebar );
    m_queueModel = new PlaylistModel( m_queueView );
    m_queueModel->setStyle( PlaylistModel::Short );
    m_queueView->queue()->setPlaylistModel( m_queueModel );
    m_queueView->queue()->playlistModel()->setReadOnly( false );
    AudioEngine::instance()->setQueue( m_queueView->queue()->proxyModel()->getSharedPointer() );

    m_sidebar->addWidget( m_searchWidget );
    m_sidebar->addWidget( m_sourcetree );
    m_sidebar->addWidget( jobsView );
    m_sidebar->addWidget( m_queueView );

    m_sidebar->setGreedyWidget( 1 );
    m_sidebar->hide( 1, false );
    m_sidebar->hide( 2, false );
    m_sidebar->hide( 3, false );
    m_sidebar->hide( 4, false );

    sidebarWidget->layout()->addWidget( m_sidebar );
    sidebarWidget->setContentsMargins( 0, 0, 0, 0 );
    sidebarWidget->layout()->setContentsMargins( 0, 0, 0, 0 );
    sidebarWidget->layout()->setMargin( 0 );

#ifndef Q_WS_MAC
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
#ifndef Q_WS_MAC
    ui->menu_Help->insertSeparator( ui->actionAboutTomahawk );
#endif

#if defined( Q_WS_MAC ) && defined( HAVE_SPARKLE )
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


void
TomahawkWindow::setupSignals()
{
    // <From PlaylistManager>
    connect( ViewManager::instance(), SIGNAL( repeatModeChanged( Tomahawk::PlaylistInterface::RepeatMode ) ),
             m_audioControls,           SLOT( onRepeatModeChanged( Tomahawk::PlaylistInterface::RepeatMode ) ) );
    connect( ViewManager::instance(), SIGNAL( shuffleModeChanged( bool ) ),
             m_audioControls,           SLOT( onShuffleModeChanged( bool ) ) );

    // <From AudioEngine>
    connect( AudioEngine::instance(), SIGNAL( error( AudioEngine::AudioErrorCode ) ), SLOT( onAudioEngineError( AudioEngine::AudioErrorCode ) ) );
    connect( AudioEngine::instance(), SIGNAL( loading( const Tomahawk::result_ptr& ) ), SLOT( onPlaybackLoading( const Tomahawk::result_ptr& ) ) );
    connect( AudioEngine::instance(), SIGNAL( started( Tomahawk::result_ptr ) ), SLOT( audioStarted() ) );
    connect( AudioEngine::instance(), SIGNAL( resumed()), SLOT( audioStarted() ) );
    connect( AudioEngine::instance(), SIGNAL( paused() ), SLOT( audioStopped() ) );
    connect( AudioEngine::instance(), SIGNAL( stopped() ), SLOT( audioStopped() ) );

    // <Menu Items>
    //    connect( ui->actionAddPeerManually, SIGNAL( triggered() ), SLOT( addPeerManually() ) );
    connect( ui->actionPreferences, SIGNAL( triggered() ), SLOT( showSettingsDialog() ) );
    connect( ui->actionDiagnostics, SIGNAL( triggered() ), SLOT( showDiagnosticsDialog() ) );
    connect( ui->actionToggleConnect, SIGNAL( triggered() ), SipHandler::instance(), SLOT( toggleConnect() ) );
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

#if defined( Q_WS_MAC )
    connect( ui->actionMinimize, SIGNAL( triggered() ), SLOT( minimize() ) );
    connect( ui->actionZoom, SIGNAL( triggered() ), SLOT( maximize() ) );
#else
    ui->menuWindow->clear();
    ui->menuWindow->menuAction()->setVisible( false );
#endif

    // <SipHandler>
    connect( SipHandler::instance(), SIGNAL( connected( SipPlugin* ) ), SLOT( onSipConnected() ) );
    connect( SipHandler::instance(), SIGNAL( disconnected( SipPlugin* ) ), SLOT( onSipDisconnected() ) );
    connect( SipHandler::instance(), SIGNAL( authError( SipPlugin* ) ), SLOT( onSipError() ) );

    // <SipMenu>
    connect( SipHandler::instance(), SIGNAL( pluginAdded( SipPlugin* ) ), this, SLOT( onSipPluginAdded( SipPlugin* ) ) );
    connect( SipHandler::instance(), SIGNAL( pluginRemoved( SipPlugin* ) ), this, SLOT( onSipPluginRemoved( SipPlugin* ) ) );
    foreach( SipPlugin *plugin, SipHandler::instance()->allPlugins() )
    {
        connect( plugin, SIGNAL( addMenu( QMenu* ) ), this, SLOT( pluginMenuAdded( QMenu* ) ) );
        connect( plugin, SIGNAL( removeMenu( QMenu* ) ), this, SLOT( pluginMenuRemoved( QMenu* ) ) );
    }
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
#ifndef Q_WS_MAC
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

#if defined( Q_WS_MAC )
    ui->actionMinimize->setDisabled( false );
    ui->actionZoom->setDisabled( false );
#endif
}


void
TomahawkWindow::hideEvent( QHideEvent* e )
{
    QMainWindow::hideEvent( e );

#if defined( Q_WS_MAC )
    ui->actionMinimize->setDisabled( true );
    ui->actionZoom->setDisabled( true );
#endif
}


void
TomahawkWindow::showSettingsDialog()
{
    SettingsDialog win;
    win.exec();
}


void TomahawkWindow::showDiagnosticsDialog()
{
    DiagnosticsDialog win;
    win.exec();
}


void
TomahawkWindow::updateCollectionManually()
{
    if ( TomahawkSettings::instance()->hasScannerPaths() )
        ScanManager::instance()->runScan();
}


void
TomahawkWindow::rescanCollectionManually()
{
    if ( TomahawkSettings::instance()->hasScannerPaths() )
        ScanManager::instance()->runScan( true );
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
    foreach( QAction* action, ui->menuNetwork->actions() )
    {
        if( action->menu() == menu )
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
TomahawkWindow::loadSpiff()
{
    LoadXSPFDialog* diag = new LoadXSPFDialog( this, Qt::Sheet );
#ifdef Q_WS_MAC
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
#ifdef Q_WS_X11
    QMessageBox::warning( this, tr( "Playback Error" ), tr( "Sorry, there is a problem accessing your audio device. Make sure you have a suitable Phonon backend and required plugins installed." ), QMessageBox::Ok );
#else
    QMessageBox::warning( this, tr( "Playback Error" ), tr( "Sorry, there is a problem accessing your audio device." ), QMessageBox::Ok );
#endif
}


void
TomahawkWindow::createAutomaticPlaylist( QString playlistName )
{
    QString name = playlistName;

    if ( name.isEmpty() )
        return;

    source_ptr author = SourceList::instance()->getLocal();
    QString id = uuid();
    QString info  = ""; // FIXME
    QString creator = "someone"; // FIXME
    dynplaylist_ptr playlist = DynamicPlaylist::create( author, id, name, info, creator, Static, false );
    playlist->setMode( Static );
    playlist->createNewRevision( uuid(), playlist->currentrevision(), playlist->type(), playlist->generator()->controls(), playlist->entries() );
    ViewManager::instance()->show( playlist );
}


void
TomahawkWindow::createStation()
{
    bool ok;
    QString name = QInputDialog::getText( this, tr( "Create New Station" ), tr( "Name:" ), QLineEdit::Normal, tr( "New Station" ), &ok );
    if ( !ok || name.isEmpty() )
        return;

    source_ptr author = SourceList::instance()->getLocal();
    QString id = uuid();
    QString info  = ""; // FIXME
    QString creator = "someone"; // FIXME
    dynplaylist_ptr playlist = DynamicPlaylist::create( author, id, name, info, creator, OnDemand, false );
    playlist->setMode( OnDemand );
    playlist->createNewRevision( uuid(), playlist->currentrevision(), playlist->type(), playlist->generator()->controls() );
    ViewManager::instance()->show( playlist );
}


void
TomahawkWindow::createPlaylist()
{
    PlaylistTypeSelectorDlg* playlistSelectorDlg = new PlaylistTypeSelectorDlg( TomahawkApp::instance()->mainWindow(), Qt::Sheet );
#ifndef Q_WS_MAC
    playlistSelectorDlg->setModal( true );
#endif
    connect( playlistSelectorDlg, SIGNAL( finished( int ) ), this, SLOT( playlistCreateDialogFinished( int ) ) );

    playlistSelectorDlg->show();
}


void
TomahawkWindow::playlistCreateDialogFinished( int ret )
{
    PlaylistTypeSelectorDlg* playlistSelectorDlg = qobject_cast< PlaylistTypeSelectorDlg* >( sender() );
    Q_ASSERT( playlistSelectorDlg );

    QString playlistName = playlistSelectorDlg->playlistName();
    if ( playlistName.isEmpty() )
        playlistName = tr( "New Playlist" );

    if ( !playlistSelectorDlg->playlistTypeIsAuto() && ret ) {

        playlist_ptr playlist = Tomahawk::Playlist::create( SourceList::instance()->getLocal(), uuid(), playlistName, "", "", false, QList< query_ptr>() );
        ViewManager::instance()->show( playlist );
    } else if ( playlistSelectorDlg->playlistTypeIsAuto() && ret ) {
       // create Auto Playlist
       createAutomaticPlaylist( playlistName );
    }
    playlistSelectorDlg->deleteLater();
}


void
TomahawkWindow::audioStarted()
{
    ui->actionPlay->setText( tr( "Pause" ) );
}


void
TomahawkWindow::audioStopped()
{

    ui->actionPlay->setText( tr( "Play" ) );
}


void
TomahawkWindow::onPlaybackLoading( const Tomahawk::result_ptr& result )
{
    m_currentTrack = result;
    setWindowTitle( m_windowTitle );
}


void
TomahawkWindow::onSipConnected()
{
    ui->actionToggleConnect->setText( tr( "Go &offline" ) );
}


void
TomahawkWindow::onSipDisconnected()
{
    ui->actionToggleConnect->setText( tr( "Go &online" ) );
}


void
TomahawkWindow::onSipPluginAdded( SipPlugin* p )
{
    connect( p, SIGNAL( addMenu( QMenu* ) ), SLOT( pluginMenuAdded( QMenu* ) ) );
    connect( p, SIGNAL( removeMenu( QMenu* ) ), SLOT( pluginMenuRemoved( QMenu* ) ) );
}


void
TomahawkWindow::onSipPluginRemoved( SipPlugin* p )
{
    Q_UNUSED( p );
}


void
TomahawkWindow::onSipError()
{
    onSipDisconnected();

    QMessageBox::warning( this,
                          tr( "Authentication Error" ),
                          QString( "Error connecting to SIP: Authentication failed!" ),
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
    QMessageBox::about( this, tr( "About Tomahawk" ),
                        tr( "<h2><b>Tomahawk %1<br/>(%2)</h2>Copyright 2010, 2011<br/>Christian Muehlhaeuser &lt;muesli@tomahawk-player.org&gt;<br/><br/>"
                            "Thanks to: Leo Franchi, Jeff Mitchell, Dominik Schmidt, Jason Herskowitz, Alejandro Wainzinger, Hugo Lindstr&ouml;m, Michael Zanetti, Harald Sitter and Steve Robertson" )
                        .arg( TomahawkUtils::appFriendlyVersion() )
                        .arg( qApp->applicationVersion() ) );
}


void
TomahawkWindow::checkForUpdates()
{
#ifdef Q_WS_MAC
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
