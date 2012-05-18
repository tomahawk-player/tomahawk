/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Tomahawk is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */

#include "TomahawkApp.h"

#include <iostream>

#include <QtCore/QPluginLoader>
#include <QtCore/QDir>
#include <QtCore/QMetaType>
#include <QtCore/QTime>
#include <QtNetwork/QNetworkReply>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>

#include "Artist.h"
#include "Album.h"
#include "Collection.h"
#include "infosystem/InfoSystem.h"
#include "accounts/AccountManager.h"
#include "accounts/spotify/SpotifyAccount.h"
#include "accounts/lastfm/LastFmAccount.h"
#include "database/Database.h"
#include "database/DatabaseCollection.h"
#include "database/DatabaseCommand_CollectionStats.h"
#include "database/DatabaseResolver.h"
#include "playlist/dynamic/GeneratorFactory.h"
#include "playlist/dynamic/echonest/EchonestGenerator.h"
#include "playlist/dynamic/database/DatabaseGenerator.h"
#include "playlist/XspfUpdater.h"
#include "network/Servent.h"
#include "web/Api_v1.h"
#include "SourceList.h"
#include "ShortcutHandler.h"
#include "ScanManager.h"
#include "TomahawkSettings.h"
#include "GlobalActionManager.h"
#include "database/LocalCollection.h"
#include "MusicScanner.h"
#include "Pipeline.h"
#include "DropJob.h"
#include "EchonestCatalogSynchronizer.h"

#include "audio/AudioEngine.h"
#include "utils/XspfLoader.h"
#include "utils/JspfLoader.h"
#include "utils/Logger.h"
#include "utils/TomahawkUtilsGui.h"
#include "accounts/lastfm/LastFmAccount.h"
#include "accounts/spotify/SpotifyAccount.h"
#include "accounts/spotify/SpotifyPlaylistUpdater.h"
#include "utils/TomahawkCache.h"

#include "config.h"

#ifndef ENABLE_HEADLESS
    #include "resolvers/QtScriptResolver.h"
    #include "resolvers/ScriptResolver.h"
    #include "utils/SpotifyParser.h"
    #include "AtticaManager.h"
    #include "TomahawkWindow.h"
    #include "SettingsDialog.h"
    #include "ActionCollection.h"
    #include <QtGui/QMessageBox>
    #include "widgets/HeaderLabel.h"
    #include <TomahawkSettingsGui.h>
#endif

// should go to a plugin actually
#ifdef GLOOX_FOUND
    #include "xmppbot/XmppBot.h"
#endif

#ifdef Q_WS_MAC
#include "mac/MacShortcutHandler.h"

#include <sys/resource.h>
#include <sys/sysctl.h>
#endif

const char* enApiSecret = "BNvTzfthHr/d1eNhHLvL1Jo=";

void
increaseMaxFileDescriptors()
{
#ifdef Q_WS_MAC
    /// Following code taken from Clementine project, main.cpp. Thanks!
    // Bump the soft limit for the number of file descriptors from the default of 256 to
    // the maximum (usually 1024).
    struct rlimit limit;
    getrlimit( RLIMIT_NOFILE, &limit );

    // getrlimit() lies about the hard limit so we have to check sysctl.
    int max_fd = 0;
    size_t len = sizeof( max_fd );
    sysctlbyname( "kern.maxfilesperproc", &max_fd, &len, NULL, 0 );

    limit.rlim_cur = max_fd;
    int ret = setrlimit( RLIMIT_NOFILE, &limit );

    if ( ret == 0 )
      qDebug() << "Max fd:" << max_fd;
#endif
}


using namespace Tomahawk;

TomahawkApp::TomahawkApp( int& argc, char *argv[] )
    : TOMAHAWK_APPLICATION( argc, argv )
    , m_headless( false )
    , m_loaded( false )
{
    setOrganizationName( QLatin1String( TOMAHAWK_ORGANIZATION_NAME ) );
    setOrganizationDomain( QLatin1String( TOMAHAWK_ORGANIZATION_DOMAIN ) );
    setApplicationName( QLatin1String( TOMAHAWK_APPLICATION_NAME ) );
    setApplicationVersion( QLatin1String( TOMAHAWK_VERSION ) );

    registerMetaTypes();
}


void
TomahawkApp::init()
{
    if ( arguments().contains( "--help" ) || arguments().contains( "-h" ) )
    {
        printHelp();
        ::exit( 0 );
    }

    qDebug() << "TomahawkApp thread:" << thread();
    Logger::setupLogfile();
    qsrand( QTime( 0, 0, 0 ).secsTo( QTime::currentTime() ) );

    tLog() << "Starting Tomahawk...";

#ifdef ENABLE_HEADLESS
    m_headless = true;
#else
    m_mainwindow = 0;
    m_headless = arguments().contains( "--headless" );
    setWindowIcon( QIcon( RESPATH "icons/tomahawk-icon-128x128.png" ) );
    setQuitOnLastWindowClosed( false );

    QFont f = APP->font();
    f.setPixelSize( HeaderLabel::defaultFontSize() );
    QFontMetrics fm( f );
    TomahawkUtils::setHeaderHeight( fm.height() + 8 );
#endif

    TomahawkSettings* s = TomahawkSettings::instance();

    tDebug( LOGINFO ) << "Setting NAM.";
    // Cause the creation of the nam, but don't need to address it directly, so prevent warning
    Q_UNUSED( TomahawkUtils::nam() );

    m_audioEngine = QWeakPointer<AudioEngine>( new AudioEngine );
    m_scanManager = QWeakPointer<ScanManager>( new ScanManager( this ) );

    // init pipeline and resolver factories
    new Pipeline();

#ifndef ENABLE_HEADLESS
    Pipeline::instance()->addExternalResolverFactory( boost::bind( &QtScriptResolver::factory, _1 ) );
    Pipeline::instance()->addExternalResolverFactory( boost::bind( &ScriptResolver::factory, _1 ) );

    new ActionCollection( this );
    connect( ActionCollection::instance()->getAction( "quit" ), SIGNAL( triggered() ), SLOT( quit() ), Qt::UniqueConnection );
#endif

    m_servent = QWeakPointer<Servent>( new Servent( this ) );
    connect( m_servent.data(), SIGNAL( ready() ), SLOT( initSIP() ) );

    tDebug() << "Init Database.";
    initDatabase();

    QByteArray magic = QByteArray::fromBase64( enApiSecret );
    QByteArray wand = QByteArray::fromBase64( QCoreApplication::applicationName().toLatin1() );
    int length = magic.length(), n2 = wand.length();
    for ( int i=0; i<length; i++ ) magic[i] = magic[i] ^ wand[i%n2];
    Echonest::Config::instance()->setAPIKey( magic );

#ifndef ENABLE_HEADLESS
    tDebug() << "Init Echonest Factory.";
    GeneratorFactory::registerFactory( "echonest", new EchonestFactory );
#endif
    tDebug() << "Init Database Factory.";
    GeneratorFactory::registerFactory( "database", new DatabaseFactory );

    // Register shortcut handler for this platform
#ifdef Q_WS_MAC
    m_shortcutHandler = QWeakPointer<Tomahawk::ShortcutHandler>( new MacShortcutHandler( this ) );
    Tomahawk::setShortcutHandler( static_cast<MacShortcutHandler*>( m_shortcutHandler.data() ) );

    Tomahawk::setApplicationHandler( this );
    increaseMaxFileDescriptors();
#endif

    // Connect up shortcuts
    if ( !m_shortcutHandler.isNull() )
    {
        connect( m_shortcutHandler.data(), SIGNAL( playPause() ), m_audioEngine.data(), SLOT( playPause() ) );
        connect( m_shortcutHandler.data(), SIGNAL( pause() ), m_audioEngine.data(), SLOT( pause() ) );
        connect( m_shortcutHandler.data(), SIGNAL( stop() ), m_audioEngine.data(), SLOT( stop() ) );
        connect( m_shortcutHandler.data(), SIGNAL( previous() ), m_audioEngine.data(), SLOT( previous() ) );
        connect( m_shortcutHandler.data(), SIGNAL( next() ), m_audioEngine.data(), SLOT( next() ) );
        connect( m_shortcutHandler.data(), SIGNAL( volumeUp() ), m_audioEngine.data(), SLOT( raiseVolume() ) );
        connect( m_shortcutHandler.data(), SIGNAL( volumeDown() ), m_audioEngine.data(), SLOT( lowerVolume() ) );
        connect( m_shortcutHandler.data(), SIGNAL( mute() ), m_audioEngine.data(), SLOT( mute() ) );
    }

    tDebug() << "Init InfoSystem.";
    m_infoSystem = QWeakPointer<Tomahawk::InfoSystem::InfoSystem>( Tomahawk::InfoSystem::InfoSystem::instance() );

    tDebug() << "Init AccountManager.";
    m_accountManager = QWeakPointer< Tomahawk::Accounts::AccountManager >( new Tomahawk::Accounts::AccountManager( this ) );
    connect( m_accountManager.data(), SIGNAL( ready() ), SLOT( accountManagerReady() ) );

    Echonest::Config::instance()->setNetworkAccessManager( TomahawkUtils::nam() );
#ifndef ENABLE_HEADLESS
    EchonestGenerator::setupCatalogs();

    if ( !m_headless )
    {
        tDebug() << "Init MainWindow.";
        m_mainwindow = new TomahawkWindow();
        m_mainwindow->setWindowTitle( "Tomahawk" );
        m_mainwindow->setObjectName( "TH_Main_Window" );
        if ( !arguments().contains( "--hide" ) )
        {
            m_mainwindow->show();
        }
    }
#endif

    tDebug() << "Init Local Collection.";
    initLocalCollection();
    tDebug() << "Init Pipeline.";
    initPipeline();

#ifndef ENABLE_HEADLESS
    // load remote list of resolvers able to be installed
    AtticaManager::instance();
#endif

    if ( arguments().contains( "--http" ) || TomahawkSettings::instance()->value( "network/http", true ).toBool() )
    {
        initHTTP();
    }
    connect( TomahawkSettings::instance(), SIGNAL( changed() ), SLOT( initHTTP() ) );

#ifndef ENABLE_HEADLESS
    if ( !s->hasScannerPaths() )
    {
        m_mainwindow->showSettingsDialog();
    }
#endif

#ifdef LIBLASTFM_FOUND
    tDebug() << "Init Scrobbler.";
    m_scrobbler = new Scrobbler( this );
#endif

    if ( arguments().contains( "--filescan" ) )
    {
        m_scanManager.data()->runScan( true );
    }

    // Set up echonest catalog synchronizer
    Tomahawk::EchonestCatalogSynchronizer::instance();

    PlaylistUpdaterInterface::registerUpdaterFactory( new XspfUpdaterFactory );
    PlaylistUpdaterInterface::registerUpdaterFactory( new SpotifyUpdaterFactory );

    // Following work-around/fix taken from Clementine rev. 13e13ccd9a95 and courtesy of David Sansome
    // A bug in Qt means the wheel_scroll_lines setting gets ignored and replaced
    // with the default value of 3 in QApplicationPrivate::initialize.
    {
        QSettings qt_settings(QSettings::UserScope, "Trolltech");
        qt_settings.beginGroup("Qt");
        QApplication::setWheelScrollLines(
            qt_settings.value("wheelScrollLines", QApplication::wheelScrollLines()).toInt());
    }

#ifndef ENABLE_HEADLESS
    // Make sure to init GAM in the gui thread
    GlobalActionManager::instance();

    // check if our spotify playlist api server is up and running, and enable spotify playlist drops if so
    QNetworkReply* r = TomahawkUtils::nam()->get( QNetworkRequest( QUrl( SPOTIFY_PLAYLIST_API_URL "/pong" ) ) );
    connect( r, SIGNAL( finished() ), this, SLOT( spotifyApiCheckFinished() ) );
#endif

#ifdef Q_WS_MAC
    // Make sure to do this after main window is inited
    Tomahawk::enableFullscreen();
#endif
}


TomahawkApp::~TomahawkApp()
{
    tLog() << "Shutting down Tomahawk...";

    if ( !m_session.isNull() )
        delete m_session.data();
    if ( !m_connector.isNull() )
        delete m_connector.data();

    Pipeline::instance()->stop();

    if ( !m_servent.isNull() )
        delete m_servent.data();
    if ( !m_scanManager.isNull() )
        delete m_scanManager.data();

    if ( !m_audioEngine.isNull() )
        delete m_audioEngine.data();

    delete Tomahawk::Accounts::AccountManager::instance();
    delete TomahawkUtils::Cache::instance();

#ifndef ENABLE_HEADLESS
    delete m_mainwindow;
    delete AtticaManager::instance();
#endif

    if ( !m_database.isNull() )
        delete m_database.data();

    delete Pipeline::instance();

    if ( !m_infoSystem.isNull() )
        delete m_infoSystem.data();

    tLog() << "Finished shutdown.";
}


TomahawkApp*
TomahawkApp::instance()
{
    return (TomahawkApp*)TOMAHAWK_APPLICATION::instance();
}


void
TomahawkApp::printHelp()
{
    #define echo( X ) std::cout << QString( X ).toAscii().data()

    echo( "Usage: " + arguments().at( 0 ) + " [options] [url]\n" );
    echo( "Options are:\n" );
    echo( "  --help         Show this help\n" );
    echo( "  --http         Initialize HTTP server\n" );
    echo( "  --filescan     Scan files on startup\n" );
    echo( "  --hide         Hide main window on startup\n" );
    echo( "  --testdb       Use a test database instead of real collection\n" );
    echo( "  --noupnp       Disable UPnP\n" );
    echo( "  --nosip        Disable SIP\n" );
    echo( "\nPlayback Controls:\n" );
    echo( "  --playpause    Toggle playing/paused state\n" );
    echo( "  --play         Start/resume playback\n" );
    echo( "  --pause        Pause playback\n" );
    echo( "  --stop         Stop playback\n" );
    echo( "  --next         Advances to the next track (if available)\n" );
    echo( "  --prev         Returns to the previous track (if available)\n" );
    echo( "\nurl is a tomahawk:// command or alternatively a url that Tomahawk can recognize.\n" );
    echo( "For more documentation, see http://wiki.tomahawk-player.org/mediawiki/index.php/Tomahawk://_Links\n" );
}


#ifndef ENABLE_HEADLESS
AudioControls*
TomahawkApp::audioControls()
{
    return m_mainwindow->audioControls();
}
#endif


void
TomahawkApp::registerMetaTypes()
{
    qRegisterMetaType< QSharedPointer<DatabaseCommand> >("QSharedPointer<DatabaseCommand>");
    qRegisterMetaType< DBSyncConnection::State >("DBSyncConnection::State");
    qRegisterMetaType< msg_ptr >("msg_ptr");
    qRegisterMetaType< QList<dbop_ptr> >("QList<dbop_ptr>");
    qRegisterMetaType< QList<QVariantMap> >("QList<QVariantMap>");
    qRegisterMetaType< QList<QString> >("QList<QString>");
    qRegisterMetaType< QList<uint> >("QList<uint>");
    qRegisterMetaType< Connection* >("Connection*");
    qRegisterMetaType< QAbstractSocket::SocketError >("QAbstractSocket::SocketError");
    qRegisterMetaType< QTcpSocket* >("QTcpSocket*");
    qRegisterMetaType< QSharedPointer<QIODevice> >("QSharedPointer<QIODevice>");
    qRegisterMetaType< QFileInfo >("QFileInfo");
    qRegisterMetaType< QDir >("QDir");
    qRegisterMetaType< QHostAddress >("QHostAddress");
    qRegisterMetaType< QMap<QString, unsigned int> >("QMap<QString, unsigned int>");
    qRegisterMetaType< QMap< QString, plentry_ptr > >("QMap< QString, plentry_ptr >");
    qRegisterMetaType< QHash< QString, QMap<quint32, quint16> > >("QHash< QString, QMap<quint32, quint16> >");
    qRegisterMetaType< QMap< QString, QMap< unsigned int, unsigned int > > >("QMap< QString, QMap< unsigned int, unsigned int > >");
    qRegisterMetaType< PairList >("PairList");

    qRegisterMetaType<GeneratorMode>("GeneratorMode");
    qRegisterMetaType<Tomahawk::GeneratorMode>("Tomahawk::GeneratorMode");
    qRegisterMetaType<ModelMode>("Tomahawk::ModelMode");
    qRegisterMetaType<Tomahawk::ModelMode>("Tomahawk::ModelMode");

    // Extra definition for namespaced-versions of signals/slots required
    qRegisterMetaType< Tomahawk::source_ptr >("Tomahawk::source_ptr");
    qRegisterMetaType< Tomahawk::collection_ptr >("Tomahawk::collection_ptr");
    qRegisterMetaType< Tomahawk::result_ptr >("Tomahawk::result_ptr");
    qRegisterMetaType< Tomahawk::query_ptr >("Tomahawk::query_ptr");
    qRegisterMetaType< Tomahawk::album_ptr >("Tomahawk::album_ptr");
    qRegisterMetaType< Tomahawk::artist_ptr >("Tomahawk::artist_ptr");
    qRegisterMetaType< Tomahawk::source_ptr >("Tomahawk::source_ptr");
    qRegisterMetaType< Tomahawk::dyncontrol_ptr >("Tomahawk::dyncontrol_ptr");
    qRegisterMetaType< Tomahawk::playlist_ptr >("Tomahawk::playlist_ptr");
    qRegisterMetaType< Tomahawk::playlistinterface_ptr >("Tomahawk::playlistinterface_ptr");
    qRegisterMetaType< Tomahawk::dynplaylist_ptr >("Tomahawk::dynplaylist_ptr");
    qRegisterMetaType< Tomahawk::geninterface_ptr >("Tomahawk::geninterface_ptr");
    qRegisterMetaType< QList<Tomahawk::playlist_ptr> >("QList<Tomahawk::playlist_ptr>");
    qRegisterMetaType< QList<Tomahawk::dynplaylist_ptr> >("QList<Tomahawk::dynplaylist_ptr>");
    qRegisterMetaType< QList<Tomahawk::dyncontrol_ptr> >("QList<Tomahawk::dyncontrol_ptr>");
    qRegisterMetaType< QList<Tomahawk::geninterface_ptr> >("QList<Tomahawk::geninterface_ptr>");
    qRegisterMetaType< QList<Tomahawk::plentry_ptr> >("QList<Tomahawk::plentry_ptr>");
    qRegisterMetaType< QList<Tomahawk::query_ptr> >("QList<Tomahawk::query_ptr>");
    qRegisterMetaType< QList<Tomahawk::result_ptr> >("QList<Tomahawk::result_ptr>");
    qRegisterMetaType< QList<Tomahawk::artist_ptr> >("QList<Tomahawk::artist_ptr>");
    qRegisterMetaType< QList<Tomahawk::album_ptr> >("QList<Tomahawk::album_ptr>");
    qRegisterMetaType< QList<Tomahawk::source_ptr> >("QList<Tomahawk::source_ptr>");
    qRegisterMetaType< QMap< QString, Tomahawk::plentry_ptr > >("QMap< QString, Tomahawk::plentry_ptr >");
    qRegisterMetaType< Tomahawk::PlaylistRevision >("Tomahawk::PlaylistRevision");
    qRegisterMetaType< Tomahawk::DynamicPlaylistRevision >("Tomahawk::DynamicPlaylistRevision");
    qRegisterMetaType< Tomahawk::QID >("Tomahawk::QID");

    qRegisterMetaType< AudioErrorCode >("AudioErrorCode");

    qRegisterMetaType< Tomahawk::InfoSystem::InfoStringHash >( "Tomahawk::InfoSystem::InfoStringHash" );
    qRegisterMetaType< Tomahawk::InfoSystem::InfoType >( "Tomahawk::InfoSystem::InfoType" );
    qRegisterMetaType< Tomahawk::InfoSystem::PushInfoFlags >( "Tomahawk::InfoSystem::PushInfoFlags" );
    qRegisterMetaType< Tomahawk::InfoSystem::PushInfoPair >( "Tomahawk::InfoSystem::PushInfoPair" );
    qRegisterMetaType< Tomahawk::InfoSystem::InfoRequestData >( "Tomahawk::InfoSystem::InfoRequestData" );
    qRegisterMetaType< Tomahawk::InfoSystem::InfoPushData >( "Tomahawk::InfoSystem::InfoPushData" );
    qRegisterMetaType< Tomahawk::InfoSystem::InfoSystemCache* >( "Tomahawk::InfoSystem::InfoSystemCache*" );
    qRegisterMetaType< Tomahawk::InfoSystem::InfoPluginPtr >( "Tomahawk::InfoSystem::InfoPluginPtr" );
    qRegisterMetaType< Tomahawk::InfoSystem::InfoPlugin* >( "Tomahawk::InfoSystem::InfoPlugin*" );
    qRegisterMetaType< QList< Tomahawk::InfoSystem::InfoStringHash > >("QList< Tomahawk::InfoSystem::InfoStringHash > ");

    qRegisterMetaType< TomahawkSettings::PrivateListeningMode >( "TomahawkSettings::PrivateListeningMode" );

    qRegisterMetaTypeStreamOperators< QList< Tomahawk::InfoSystem::InfoStringHash > >("QList< Tomahawk::InfoSystem::InfoStringHash > ");
    qRegisterMetaType< QPersistentModelIndex >( "QPersistentModelIndex" );

    qRegisterMetaType< Tomahawk::PlaylistInterface::LatchMode >( "Tomahawk::PlaylistInterface::LatchMode" );

    qRegisterMetaType< TomahawkUtils::CacheData >( "TomahawkUtils::CacheData" );
    qRegisterMetaTypeStreamOperators< TomahawkUtils::CacheData >( "TomahawkUtils::CacheData" );
}


void
TomahawkApp::initDatabase()
{
    QString dbpath;
    if ( arguments().contains( "--testdb" ) )
    {
        dbpath = QDir::currentPath() + "/test.db";
    }
    else
    {
        dbpath = TomahawkUtils::appDataDir().absoluteFilePath( "tomahawk.db" );
    }

    tDebug( LOGEXTRA ) << "Using database:" << dbpath;
    m_database = QWeakPointer<Database>( new Database( dbpath, this ) );
    Pipeline::instance()->databaseReady();
}


void
TomahawkApp::initHTTP()
{
    if ( !TomahawkSettings::instance()->httpEnabled() )
    {
        tLog() << "Stopping HTTPd, not enabled";
        if ( !m_session.isNull() )
            delete m_session.data();
        if ( !m_connector.isNull() )
            delete m_connector.data();
        return;
    }

    if ( m_session )
    {
        tLog() << "HTTPd session already exists, returning";
        return;
    }

    m_session = QWeakPointer< QxtHttpSessionManager >( new QxtHttpSessionManager() );
    m_connector = QWeakPointer< QxtHttpServerConnector >( new QxtHttpServerConnector );
    if ( m_session.isNull() || m_connector.isNull() )
    {
        if ( !m_session.isNull() )
            delete m_session.data();
        if ( !m_connector.isNull() )
            delete m_connector.data();
        tLog() << "Failed to start HTTPd, could not create object";
        return;
    }

    m_session.data()->setPort( 60210 ); //TODO config
    m_session.data()->setListenInterface( QHostAddress::LocalHost );
    m_session.data()->setConnector( m_connector.data() );

    Api_v1* api = new Api_v1( m_session.data() );
    m_session.data()->setStaticContentService( api );

    tLog() << "Starting HTTPd on" << m_session.data()->listenInterface().toString() << m_session.data()->port();
    m_session.data()->start();
}


void
TomahawkApp::initPipeline()
{
    // setup resolvers for local content, and (cached) remote collection content
    Pipeline::instance()->addResolver( new DatabaseResolver( 100 ) );
}


void
TomahawkApp::initLocalCollection()
{
    connect( SourceList::instance(), SIGNAL( ready() ), SLOT( initServent() ) );

    source_ptr src( new Source( 0, tr( "My Collection" ) ) );
    collection_ptr coll( new LocalCollection( src ) );

    src->addCollection( coll );
    SourceList::instance()->setLocal( src );
    SourceList::instance()->loadSources();

    // to make the stats signal be emitted by our local source
    // this will update the sidebar, etc.
    DatabaseCommand_CollectionStats* cmd = new DatabaseCommand_CollectionStats( src );
    connect( cmd,       SIGNAL( done( const QVariantMap& ) ),
             src.data(),  SLOT( setStats( const QVariantMap& ) ), Qt::QueuedConnection );
    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
}


void
TomahawkApp::initServent()
{
    tDebug() << "Init Servent.";

    bool upnp = !arguments().contains( "--noupnp" );
    int port = TomahawkSettings::instance()->externalPort();
    if ( !Servent::instance()->startListening( QHostAddress( QHostAddress::Any ), upnp, port ) )
    {
        tLog() << "Failed to start listening with servent";
        exit( 1 );
    }
}


// Called after Servent emits ready()
void
TomahawkApp::initSIP()
{
    tDebug() << Q_FUNC_INFO;
    //FIXME: jabber autoconnect is really more, now that there is sip -- should be renamed and/or split out of jabber-specific settings
    if ( !arguments().contains( "--nosip" ) )
    {
#ifdef GLOOX_FOUND
        m_xmppBot = QWeakPointer<XMPPBot>( new XMPPBot( this ) );
#endif

        tDebug( LOGINFO ) << "Connecting SIP classes";
        Accounts::AccountManager::instance()->initSIP();
    }

    m_loaded = true;
    emit tomahawkLoaded();
}


void
TomahawkApp::spotifyApiCheckFinished()
{
#ifndef ENABLE_HEADLESS
    QNetworkReply* reply = qobject_cast< QNetworkReply* >( sender() );
    Q_ASSERT( reply );

    DropJob::setCanParseSpotifyPlaylists( !reply->error() );
#endif
}


void
TomahawkApp::accountManagerReady()
{
    Tomahawk::Accounts::LastFmAccountFactory* lastfmFactory = new Tomahawk::Accounts::LastFmAccountFactory();
    m_accountManager.data()->addAccountFactory( lastfmFactory );

    Tomahawk::Accounts::SpotifyAccountFactory* spotifyFactory = new Tomahawk::Accounts::SpotifyAccountFactory;
    m_accountManager.data()->addAccountFactory( spotifyFactory );
    m_accountManager.data()->registerAccountFactoryForFilesystem( spotifyFactory );

    Tomahawk::Accounts::AccountManager::instance()->loadFromConfig();
}


void
TomahawkApp::activate()
{
#ifndef ENABLE_HEADLESS
    TomahawkUtils::bringToFront();
#endif
}


bool
TomahawkApp::loadUrl( const QString& url )
{
#ifndef ENABLE_HEADLESS
    if ( url.startsWith( "tomahawk://" ) )
        return GlobalActionManager::instance()->parseTomahawkLink( url );
    else if ( url.contains( "open.spotify.com" ) || url.contains( "spotify:track" ) )
        return GlobalActionManager::instance()->openSpotifyLink( url );
    else if ( url.contains( "www.rdio.com" ) )
        return GlobalActionManager::instance()->openRdioLink( url );
    else
    {
        QFile f( url );
        QFileInfo info( f );
        if ( info.suffix() == "xspf" )
        {
            XSPFLoader* l = new XSPFLoader( true, this );
            tDebug( LOGINFO ) << "Loading spiff:" << url;
            l->load( QUrl::fromUserInput( url ) );

            return true;
        }
        else if ( info.suffix() == "jspf" )
        {
            JSPFLoader* l = new JSPFLoader( true, this );
            tDebug( LOGINFO ) << "Loading j-spiff:" << url;
            l->load( QUrl::fromUserInput( url ) );

            return true;
        }
    }
#endif
    return false;
}


void
TomahawkApp::instanceStarted( KDSingleApplicationGuard::Instance instance )
{
    tDebug( LOGINFO ) << "Instance started!" << instance.pid << instance.arguments;

    if ( instance.arguments.size() < 2 )
        return;

    QString arg1 = instance.arguments[ 1 ];
    if ( loadUrl( arg1 ) )
    {
        activate();
        return;
    }

    if ( instance.arguments.contains( "--next" ) )
        AudioEngine::instance()->next();
    else if ( instance.arguments.contains( "--prev" ) )
        AudioEngine::instance()->previous();
    else if ( instance.arguments.contains( "--playpause" ) )
        AudioEngine::instance()->playPause();
    else if ( instance.arguments.contains( "--play" ) )
        AudioEngine::instance()->play();
    else if ( instance.arguments.contains( "--pause" ) )
        AudioEngine::instance()->pause();
    else if ( instance.arguments.contains( "--stop" ) )
        AudioEngine::instance()->stop();
}
