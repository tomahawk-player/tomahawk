#include "tomahawkwindow.h"
#include "ui_tomahawkwindow.h"

#include <QAction>
#include <QInputDialog>
#include <QPixmap>
#include <QPropertyAnimation>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QToolBar>

#include "tomahawk/tomahawkapp.h"
#include "tomahawk/functimeout.h"
#include "tomahawk/playlist.h"
#include "tomahawk/query.h"

#include "database/databasecommand_collectionstats.h"
#include "topbar/topbar.h"

#include "audiocontrols.h"
#include "controlconnection.h"
#include "database.h"
#include "musicscanner.h"
#include "playlistmanager.h"
#include "proxystyle.h"
#include "settingsdialog.h"
#include "xspfloader.h"
#include "proxystyle.h"
#include "tomahawksettings.h"
#include "tomahawktrayicon.h"
#include "widgetdragfilter.h"

using namespace Tomahawk;


TomahawkWindow::TomahawkWindow( QWidget* parent )
    : QMainWindow( parent )
    , ui( new Ui::TomahawkWindow )
    , m_topbar( new TopBar( this ) )
    , m_audioControls( new AudioControls( this ) )
    , m_playlistManager( new PlaylistManager( this ) )
    , m_trayIcon( new TomahawkTrayIcon( this ) )
{
    qApp->setStyle( new ProxyStyle() );
    setWindowIcon( QIcon( RESPATH "icons/tomahawk-icon-128x128.png" ) );

#ifdef Q_WS_MAC
    setUnifiedTitleAndToolBarOnMac( true );
#endif

    ui->setupUi( this );

#ifndef Q_WS_MAC
    ui->centralWidget->layout()->setContentsMargins( 4, 4, 4, 2 );
#else
//     ui->actionProgress->setAttribute( Qt::WA_MacShowFocusRect, 0 );
//     ui->playlistView->setAttribute( Qt::WA_MacShowFocusRect, 0 );
    ui->sourceTreeView->setAttribute( Qt::WA_MacShowFocusRect, 0 );
#endif

    delete ui->playlistWidget;
    ui->splitter->addWidget( m_playlistManager->widget() );
    ui->splitter->setStretchFactor( 0, 1 );
    ui->splitter->setStretchFactor( 1, 3 );

    QToolBar* toolbar = addToolBar( "TomahawkToolbar" );
    toolbar->setObjectName( "TomahawkToolbar" );
    toolbar->addWidget( m_topbar );
    toolbar->setMovable( false );
    toolbar->setFloatable( false );
    toolbar->installEventFilter( new WidgetDragFilter( toolbar ) );
    
    statusBar()->addPermanentWidget( m_audioControls, 1 );

    loadSettings();
    setupSignals();
}


TomahawkWindow::~TomahawkWindow()
{
    saveSettings();
    delete ui;
}


void
TomahawkWindow::loadSettings()
{
    TomahawkSettings* s = APP->settings();

    if ( !s->mainWindowGeometry().isEmpty() )
        restoreGeometry( s->mainWindowGeometry() );
    if ( !s->mainWindowState().isEmpty() )
        restoreState( s->mainWindowState() );
}


void
TomahawkWindow::saveSettings()
{
    TomahawkSettings* s = APP->settings();
    s->setMainWindowGeometry( saveGeometry() );
    s->setMainWindowState( saveState() );
}


void
TomahawkWindow::setupSignals()
{
    connect( ui->actionExit, SIGNAL( triggered() ),
             qApp,             SLOT( closeAllWindows() ) );

    connect( ui->actionLoadXSPF, SIGNAL( triggered() ), SLOT( loadSpiff() ));
    connect( ui->actionCreatePlaylist, SIGNAL( triggered() ), SLOT( createPlaylist() ));

    // <Playlist>
    connect( m_topbar,         SIGNAL( filterTextChanged( const QString& ) ),
             playlistManager(),  SLOT( setFilter( const QString& ) ) );

    connect( playlistManager(), SIGNAL( numSourcesChanged( unsigned int ) ),
             m_topbar,            SLOT( setNumSources( unsigned int ) ) );

    connect( playlistManager(), SIGNAL( numTracksChanged( unsigned int ) ),
             m_topbar,            SLOT( setNumTracks( unsigned int ) ) );

    connect( playlistManager(), SIGNAL( numArtistsChanged( unsigned int ) ),
             m_topbar,            SLOT( setNumArtists( unsigned int ) ) );

    connect( playlistManager(), SIGNAL( numShownChanged( unsigned int ) ),
             m_topbar,            SLOT( setNumShown( unsigned int ) ) );

    connect( m_topbar,         SIGNAL( flatMode() ),
             m_playlistManager,  SLOT( setTableMode() ) );

    connect( m_topbar,         SIGNAL( artistMode() ),
             m_playlistManager,  SLOT( setTreeMode() ) );

    // <From PlaylistManager>
    connect( playlistManager(), SIGNAL( repeatModeChanged( PlaylistInterface::RepeatMode ) ),
             m_audioControls,     SLOT( onRepeatModeChanged( PlaylistInterface::RepeatMode ) ) );

    connect( playlistManager(), SIGNAL( shuffleModeChanged( bool ) ),
             m_audioControls,     SLOT( onShuffleModeChanged( bool ) ) );

    // <From AudioEngine>
    connect( (QObject*)APP->audioEngine(), SIGNAL( loading( const Tomahawk::result_ptr& ) ),
                                             SLOT( onPlaybackLoading( const Tomahawk::result_ptr& ) ) );

    // <Menu Items>
    connect( ui->actionPreferences, SIGNAL( triggered() ), SLOT( showSettingsDialog() ) );
    connect( ui->actionAddPeerManually, SIGNAL( triggered() ), SLOT( addPeerManually() ) );
    connect( ui->actionRescanCollection, SIGNAL( triggered() ), SLOT( rescanCollectionManually() ) );
    connect( ui->actionAbout_Tomahawk, SIGNAL( triggered() ), SLOT( showAboutTomahawk() ) );
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


PlaylistManager*
TomahawkWindow::playlistManager()
{
    return m_playlistManager;
}


void
TomahawkWindow::showSettingsDialog()
{
    qDebug() << Q_FUNC_INFO;
    SettingsDialog win;
    win.exec();

    // settings are written in SettingsDialog destructor, bleh
    QTimer::singleShot( 0, this, SIGNAL( settingsChanged() ) );
}


/// scan stuff
void
TomahawkWindow::rescanCollectionManually()
{
    TomahawkSettings* s = APP->settings();
    bool ok;
    QString path = QInputDialog::getText( this, tr( "Enter path to music dir:" ),
                                                tr( "Path pls" ), QLineEdit::Normal,
                                                s->scannerPath(), &ok );
    s->setValue( "scannerpath", path );
    if ( ok && !path.isEmpty() )
    {
        MusicScanner* scanner = new MusicScanner( path );
        connect( scanner, SIGNAL( finished() ), this, SLOT( scanFinished() ) );
        scanner->start();
    }
}


void
TomahawkWindow::scanFinished()
{
    qDebug() << Q_FUNC_INFO;
    MusicScanner* scanner = (MusicScanner*) sender();
    scanner->deleteLater();
}


void
TomahawkWindow::addPeerManually()
{
    TomahawkSettings* s = APP->settings();
    // stealing this for connecting to peers for now:
    bool ok;
    QString addr = QInputDialog::getText( this, tr( "Connect to peer" ),
                                                tr( "Enter peer address:" ), QLineEdit::Normal,
                                                s->value( "connip" ).toString(), &ok ); // FIXME
    if ( !ok )
        return;

    s->setValue( "connip", addr );
    QString ports = QInputDialog::getText( this, tr( "Connect to peer" ),
                                                 tr( "Enter peer port:" ), QLineEdit::Normal,
                                                 s->value( "connport", "50210" ).toString(), &ok );
    if ( !ok )
        return;

    s->setValue( "connport", ports );
    int port = ports.toInt();
    QString key = QInputDialog::getText( this, tr( "Connect to peer" ),
                                               tr( "Enter peer key:" ), QLineEdit::Normal,
                                               "whitelist", &ok );
    if ( !ok )
        return;

    qDebug() << "Attempting to connect to " << addr;
    APP->servent().connectToPeer( addr, port, key );
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
TomahawkWindow::createPlaylist()
{
    qDebug() << Q_FUNC_INFO;

    bool ok;
    QString name = QInputDialog::getText( this, "Create New Playlist", "Name:", QLineEdit::Normal, "New Playlist", &ok );
    if ( !ok || name.isEmpty() )
        return;

    source_ptr author = APP->sourcelist().getLocal();
    QString id = uuid();
    QString info  = ""; // FIXME
    QString creator = "someone"; // FIXME
    Playlist::create( author, id, name, info, creator, false /* shared */ );
}


void
TomahawkWindow::onPlaybackLoading( const Tomahawk::result_ptr& result )
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
        QString s = m_currentTrack->track() + " " + tr( "by" ) + " " + m_currentTrack->artist()->name();
        QMainWindow::setWindowTitle( s + " - " + title );
    }
}


void
TomahawkWindow::showAboutTomahawk()
{
    QMessageBox::about( this, "About Tomahawk", "Copyright 2010 Christian Muehlhaeuser <muesli@gmail.com>\n\nThanks to: Leo Franchi, Jeff Mitchell, Dominik Schmidt and Steve Robertson" );
}
