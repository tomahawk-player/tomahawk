#include "tomahawkwindow.h"
#include "ui_tomahawkwindow.h"

#include <QAction>
#include <QCloseEvent>
#include <QInputDialog>
#include <QPixmap>
#include <QPropertyAnimation>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QToolBar>

#include "tomahawk/tomahawkapp.h"
#include "playlist.h"
#include "query.h"
#include "artist.h"

#include "audio/audioengine.h"
#include "database/database.h"
#include "database/databasecommand_collectionstats.h"
#include "network/controlconnection.h"
#include "playlist/playlistmanager.h"
#include "sip/SipHandler.h"
#include "sourcetree/sourcetreeview.h"
#include "utils/animatedsplitter.h"
#include "utils/proxystyle.h"
#include "utils/widgetdragfilter.h"
#include "utils/xspfloader.h"
#include "widgets/newplaylistwidget.h"
#include "widgets/welcomewidget.h"

#include "audiocontrols.h"
#include "settingsdialog.h"
#include "tomahawksettings.h"
#include "sourcelist.h"
#include "transferview.h"
#include "tomahawktrayicon.h"
#include "playlist/dynamic/GeneratorInterface.h"
#include "scanmanager.h"

using namespace Tomahawk;


TomahawkWindow::TomahawkWindow( QWidget* parent )
    : QMainWindow( parent )
    , ui( new Ui::TomahawkWindow )
    , m_audioControls( new AudioControls( this ) )
    , m_trayIcon( new TomahawkTrayIcon( this ) )
{
    qApp->setStyle( new ProxyStyle() );
    setWindowIcon( QIcon( RESPATH "icons/tomahawk-icon-128x128.png" ) );

#ifdef Q_WS_MAC
    setUnifiedTitleAndToolBarOnMac( true );
#endif

    PlaylistManager* pm = new PlaylistManager( this );

    connect( m_audioControls, SIGNAL( playPressed() ), pm, SLOT( onPlayClicked() ) );
    connect( m_audioControls, SIGNAL( pausePressed() ), pm, SLOT( onPauseClicked() ) );

    ui->setupUi( this );

    delete ui->sidebarWidget;
    delete ui->playlistWidget;

    ui->centralWidget->setContentsMargins( 0, 0, 0, 0 );
    ui->centralWidget->layout()->setContentsMargins( 0, 0, 0, 0 );
    ui->centralWidget->layout()->setMargin( 0 );

    QWidget* sidebarWidget = new QWidget();
    sidebarWidget->setLayout( new QVBoxLayout() );

    AnimatedSplitter* sidebar = new AnimatedSplitter();
    sidebar->setOrientation( Qt::Vertical );
    sidebar->setChildrenCollapsible( false );
    sidebar->setGreedyWidget( 0 );
    sidebar->setStretchFactor( 0, 3 );
    sidebar->setStretchFactor( 1, 1 );

    SourceTreeView* stv = new SourceTreeView();
    TransferView* transferView = new TransferView();

    sidebar->addWidget( stv );
    sidebar->addWidget( transferView );
    sidebar->hide( 1, false );

/*    QWidget* buttonWidget = new QWidget();
    buttonWidget->setLayout( new QVBoxLayout() );
    m_statusButton = new QPushButton();
    buttonWidget->layout()->addWidget( m_statusButton );*/

    sidebarWidget->layout()->addWidget( sidebar );
//    sidebarWidget->layout()->addWidget( buttonWidget );

    sidebarWidget->setContentsMargins( 0, 0, 0, 0 );
    sidebarWidget->layout()->setContentsMargins( 0, 0, 0, 0 );
    sidebarWidget->layout()->setMargin( 0 );
    sidebarWidget->layout()->setSpacing( 0 );
/*    buttonWidget->setContentsMargins( 0, 0, 0, 0 );
    buttonWidget->layout()->setContentsMargins( 0, 0, 0, 0 );
    buttonWidget->layout()->setMargin( 0 );
    buttonWidget->layout()->setSpacing( 0 );*/

    ui->splitter->addWidget( sidebarWidget );
    ui->splitter->addWidget( PlaylistManager::instance()->widget() );

    ui->splitter->setStretchFactor( 0, 1 );
    ui->splitter->setStretchFactor( 1, 3 );
    ui->splitter->setCollapsible( 1, false );
    ui->splitter->setHandleWidth( 1 );

/*    QToolBar* toolbar = addToolBar( "TomahawkToolbar" );
    toolbar->setObjectName( "TomahawkToolbar" );
    toolbar->addWidget( m_topbar );
    toolbar->setMovable( false );
    toolbar->setFloatable( false );
    toolbar->installEventFilter( new WidgetDragFilter( toolbar ) );*/

    statusBar()->addPermanentWidget( m_audioControls, 1 );

    loadSettings();
    setupSignals();

    PlaylistManager::instance()->show( new WelcomeWidget(), tr( "Welcome to Tomahawk!" ), QString(), QPixmap( RESPATH "icons/tomahawk-icon-128x128.png" ) );
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

    if ( !s->mainWindowGeometry().isEmpty() )
        restoreGeometry( s->mainWindowGeometry() );
    if ( !s->mainWindowState().isEmpty() )
        restoreState( s->mainWindowState() );
}


void
TomahawkWindow::saveSettings()
{
    TomahawkSettings* s = TomahawkSettings::instance();
    s->setMainWindowGeometry( saveGeometry() );
    s->setMainWindowState( saveState() );
}


void
TomahawkWindow::setupSignals()
{
    // <From PlaylistManager>
    connect( PlaylistManager::instance(), SIGNAL( repeatModeChanged( PlaylistInterface::RepeatMode ) ),
             m_audioControls,     SLOT( onRepeatModeChanged( PlaylistInterface::RepeatMode ) ) );

    connect( PlaylistManager::instance(), SIGNAL( shuffleModeChanged( bool ) ),
             m_audioControls,     SLOT( onShuffleModeChanged( bool ) ) );

    // <From AudioEngine>
    connect( AudioEngine::instance(), SIGNAL( loading( const Tomahawk::result_ptr& ) ),
                                        SLOT( onPlaybackLoading( const Tomahawk::result_ptr& ) ) );

    // <Menu Items>
    connect( ui->actionPreferences, SIGNAL( triggered() ), SLOT( showSettingsDialog() ) );
    connect( ui->actionAddPeerManually, SIGNAL( triggered() ), SLOT( addPeerManually() ) );
    connect( ui->actionAddFriendManually, SIGNAL( triggered() ), SLOT( addFriendManually() ) );
    connect( ui->actionRescanCollection, SIGNAL( triggered() ), SLOT( rescanCollectionManually() ) );
    connect( ui->actionLoadXSPF, SIGNAL( triggered() ), SLOT( loadSpiff() ));
    connect( ui->actionCreatePlaylist, SIGNAL( triggered() ), SLOT( createPlaylist() ));
    connect( ui->actionCreateAutomaticPlaylist, SIGNAL( triggered() ), SLOT( createAutomaticPlaylist() ));
    connect( ui->actionCreate_New_Station, SIGNAL( triggered() ), SLOT( createStation() ));
    connect( ui->actionAboutTomahawk, SIGNAL( triggered() ), SLOT( showAboutTomahawk() ) );
    connect( ui->actionExit, SIGNAL( triggered() ), APP, SLOT( quit() ) );
//    connect( m_statusButton, SIGNAL( clicked() ), APP->sipHandler(), SLOT( toggleConnect() ) );

    // <SipHandler>
    connect( APP->sipHandler(), SIGNAL( connected() ), SLOT( onSipConnected() ) );
    connect( APP->sipHandler(), SIGNAL( disconnected() ), SLOT( onSipDisconnected() ) );
    connect( APP->sipHandler(), SIGNAL( authError() ), SLOT( onSipError() ) );

    // set initial connection state
    onSipDisconnected();
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
    if ( QSystemTrayIcon::isSystemTrayAvailable() )
    {
        hide();
        e->ignore();
        return;
    }
#endif

    e->accept();
}


void
TomahawkWindow::showSettingsDialog()
{
    qDebug() << Q_FUNC_INFO;
    SettingsDialog win;
    win.exec();
}


/// scan stuff
void
TomahawkWindow::rescanCollectionManually()
{
    TomahawkSettings* s = TomahawkSettings::instance();
    bool ok;
    QString path = QInputDialog::getText( this, tr( "Enter path to music dir:" ),
                                                tr( "Path pls" ), QLineEdit::Normal,
                                                s->scannerPath(), &ok );
    s->setValue( "scannerpath", path );
    if ( ok && !path.isEmpty() )
        ScanManager::instance()->runManualScan( path );
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
TomahawkWindow::addFriendManually()
{
    bool ok;
    QString id = QInputDialog::getText( this, tr( "Add Friend" ),
                                              tr( "Enter Jabber ID:" ), QLineEdit::Normal,
                                              "", &ok );
    if ( !ok )
        return;

    qDebug() << "Attempting to add jabber contact to roster:" << id;
    APP->sipHandler()->addContact( id );
}


void
TomahawkWindow::loadSpiff()
{
    bool ok;
    QString urlstr = QInputDialog::getText( this, "Load XSPF", "Path:", QLineEdit::Normal, "http://ws.audioscrobbler.com/1.0/tag/metal/toptracks.xspf", &ok );
    if ( !ok || urlstr.isEmpty() )
        return;

    QUrl url( urlstr );

    XSPFLoader* loader = new XSPFLoader;
    loader->load( url );
}


void 
TomahawkWindow::createAutomaticPlaylist()
{
    bool ok;
    QString name = QInputDialog::getText( this, "Create New Automatic Playlist", "Name:", QLineEdit::Normal, "New Automatic Playlist", &ok );
    if ( !ok || name.isEmpty() )
        return;
    
    source_ptr author = SourceList::instance()->getLocal();
    QString id = uuid();
    QString info  = ""; // FIXME
    QString creator = "someone"; // FIXME
    dynplaylist_ptr playlist = DynamicPlaylist::create( author, id, name, info, creator, false );
    playlist->setMode( Static );
    playlist->createNewRevision( uuid(), playlist->currentrevision(), playlist->type(), playlist->generator()->controls(), playlist->entries() );
}


void TomahawkWindow::createStation()
{
    bool ok;
    QString name = QInputDialog::getText( this, "Create New Station", "Name:", QLineEdit::Normal, "New Station", &ok );
    if ( !ok || name.isEmpty() )
        return;
    
    source_ptr author = SourceList::instance()->getLocal();
    QString id = uuid();
    QString info  = ""; // FIXME
    QString creator = "someone"; // FIXME
    dynplaylist_ptr playlist = DynamicPlaylist::create( author, id, name, info, creator, false );
    playlist->setMode( OnDemand );
    playlist->createNewRevision( uuid(), playlist->currentrevision(), playlist->type(), playlist->generator()->controls() );
}


void
TomahawkWindow::createPlaylist()
{

    PlaylistManager::instance()->show( new NewPlaylistWidget() );

/*    bool ok;
    QString name = QInputDialog::getText( this, "Create New Playlist", "Name:", QLineEdit::Normal, "New Playlist", &ok );
    if ( !ok || name.isEmpty() )
        return;

    source_ptr author = SourceList::instance()->getLocal();
    QString id = uuid();
    QString info  = ""; // FIXME
    QString creator = "someone"; // FIXME
    if( dynamic )
        DynamicPlaylist::create( author, id, name, info, creator, false );
    else
        Playlist::create( author, id, name, info, creator, false ); */

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
//    m_statusButton->setText( tr( "Online" ) );
}


void
TomahawkWindow::onSipDisconnected()
{
//    m_statusButton->setText( tr( "Offline" ) );
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
        QString s = m_currentTrack->track() + " " + tr( "by" ) + " " + m_currentTrack->artist()->name();
        QMainWindow::setWindowTitle( s + " - " + title );
    }
}


void
TomahawkWindow::showAboutTomahawk()
{
    QMessageBox::about( this, "About Tomahawk", "Copyright 2010, 2011 Christian Muehlhaeuser <muesli@gmail.com>\n\nThanks to: Leo Franchi, Jeff Mitchell, Dominik Schmidt, Alejandro Wainzinger and Steve Robertson" );
}
