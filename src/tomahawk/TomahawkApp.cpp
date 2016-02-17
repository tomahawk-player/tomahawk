/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2014, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright 2013-2014, Teo Mrnjavac <teo@kde.org>
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

#include "TomahawkVersion.h"
#include "AclRegistryImpl.h"
#include "Album.h"
#include "Artist.h"
#include "Track.h"
#include "collection/Collection.h"
#include "infosystem/InfoSystem.h"
#include "infosystem/InfoSystemCache.h"
#include "playlist/dynamic/GeneratorFactory.h"
#include "playlist/dynamic/echonest/EchonestGenerator.h"
#include "playlist/dynamic/database/DatabaseGenerator.h"
#include "playlist/XspfUpdater.h"
#include "network/Servent.h"
#include "network/DbSyncConnection.h"
#include "SourceList.h"
#include "ViewManager.h"
#include "ShortcutHandler.h"
#include "filemetadata/ScanManager.h"
#include "TomahawkSettings.h"
#include "GlobalActionManager.h"
#include "database/LocalCollection.h"
#include "Pipeline.h"
#include "DropJob.h"
#include "DownloadManager.h"
#include "EchonestCatalogSynchronizer.h"
#include "database/DatabaseImpl.h"
#include "network/Msg.h"
#include "utils/NetworkAccessManager.h"

#include "accounts/lastfm/LastFmAccount.h"
#include "accounts/spotify/SpotifyAccount.h"
//#include "accounts/spotify/SpotifyPlaylistUpdater.h"
#include "accounts/AccountManager.h"
#include "audio/AudioEngine.h"
#include "database/Database.h"
#include "database/DatabaseCollection.h"
#include "database/DatabaseCommand_CollectionStats.h"
#include "database/DatabaseResolver.h"
#include "playlist/PlaylistTemplate.h"
#include "jobview/ErrorStatusMessage.h"
#include "jobview/JobStatusModel.h"
#include "jobview/JobStatusView.h"
#include "utils/XspfLoader.h"
#include "utils/JspfLoader.h"
#include "utils/Logger.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/TomahawkCache.h"
#include "widgets/SplashWidget.h"

#include "resolvers/JSResolver.h"
#include "resolvers/ScriptResolver.h"
#include "utils/SpotifyParser.h"
#include "AtticaManager.h"
#include "TomahawkWindow.h"
#include "dialogs/SettingsDialog.h"
#include "ActionCollection.h"
#include "widgets/HeaderLabel.h"
#include "utils/TomahawkStyle.h"

#include "config.h"

#ifdef WITH_UPOWER
    #include "linux/UPowerHandler.h"
#endif

#ifdef WITH_GNOMESHORTCUTHANDLER
    #include "linux/GnomeShortcutHandler.h"
#endif

#ifdef Q_OS_MAC
    #include "mac/MacShortcutHandler.h"

    #include <sys/resource.h>
    #include <sys/sysctl.h>
#endif

#include <QDir>
#include <QMetaType>
#include <QTime>
#include <QMessageBox>
#include <QNetworkReply>
#include <QProgressDialog>
#include <QFile>
#include <QFileInfo>
#include <QTranslator>

#include <iostream>


const char* enApiSecret = "BNvTzfthHr/d1eNhHLvL1Jo=";

void
increaseMaxFileDescriptors()
{
#ifdef Q_OS_MAC
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
    , m_mainwindow( nullptr )
    , m_splashWidget( nullptr )
    , m_headless( false )
{
    if ( arguments().contains( "--help" ) || arguments().contains( "-h" ) )
    {
        printHelp();
        ::exit( 0 );
    }

    setOrganizationName( QLatin1String( TOMAHAWK_ORGANIZATION_NAME ) );
    setOrganizationDomain( QLatin1String( TOMAHAWK_ORGANIZATION_DOMAIN ) );
    setApplicationName( QLatin1String( TOMAHAWK_APPLICATION_NAME ) );
    setApplicationVersion( QLatin1String( TOMAHAWK_VERSION ) );

    registerMetaTypes();
    TomahawkUtils::installTranslator( this );
}


void
TomahawkApp::init()
{
    qDebug() << "TomahawkApp thread:" << thread();
    m_logFile.setFileName( TomahawkUtils::logFilePath() );
    Logger::setupLogfile( m_logFile );

    qsrand( QTime( 0, 0, 0 ).secsTo( QTime::currentTime() ) );

    tLog() << "Starting Tomahawk...";

    QWebSettings::globalSettings()->setAttribute(QWebSettings::OfflineStorageDatabaseEnabled, true);
    //QWebSettings::setOfflineStoragePath(QLatin1String("/tmp"));

    m_headless = arguments().contains( "--headless" );

#ifndef Q_OS_MAC
    setWindowIcon( QIcon( RESPATH "icons/tomahawk-icon-128x128.png" ) );
    setQuitOnLastWindowClosed( false );
#endif

    if ( arguments().contains( "--splash" ) )
    {
        startSplashWidget( "Splash screen test\n" );
        updateSplashWidgetMessage( "Splash screen test\n2/7" );
    }

    TomahawkStyle::loadFonts();
    QFont f = font();
    TomahawkUtils::setSystemFont( f );
    f.setFamily( "Roboto" );
#ifdef Q_OS_WIN
    f.setPointSize( 10 );
#endif
#ifdef Q_OS_MAC
    f.setPointSize( f.pointSize() - 2 );
#endif

    setFont( f );
    tDebug() << "Default font:" << f.pixelSize() << f.pointSizeF() << f.family() << QFontMetrics( f ).height();
    TomahawkUtils::setDefaultFontSize( f.pointSize() );

    TomahawkUtils::setHeadless( m_headless );
    new ACLRegistryImpl( this );

    TomahawkSettings *s = TomahawkSettings::instance();
    Tomahawk::Utils::setProxyDns( s->proxyDns() );
    Tomahawk::Utils::setProxyType( s->proxyType() );
    Tomahawk::Utils::setProxyHost( s->proxyHost() );
    Tomahawk::Utils::setProxyPort( s->proxyPort() );
    Tomahawk::Utils::setProxyUsername( s->proxyUsername() );
    Tomahawk::Utils::setProxyPassword( s->proxyPassword() );
    Tomahawk::Utils::setProxyNoProxyHosts( s->proxyNoProxyHosts() );

    // Cause the creation of the nam, but don't need to address it directly, so prevent warning
    tDebug() << "Setting NAM:" << Tomahawk::Utils::nam();

    DownloadManager::instance();
    m_audioEngine = QPointer<AudioEngine>( new AudioEngine );

    // init pipeline and resolver factories
    new Pipeline();

    m_servent = QPointer<Servent>( new Servent( this ) );
    connect( m_servent.data(), SIGNAL( ready() ), SLOT( initSIP() ) );

    tDebug() << "Init Database.";
    initDatabase();

    Pipeline::instance()->addExternalResolverFactory(
                std::bind( &JSResolver::factory, std::placeholders::_1,
                           std::placeholders::_2, std::placeholders::_3 ) );
    Pipeline::instance()->addExternalResolverFactory(
                std::bind( &ScriptResolver::factory, std::placeholders::_1,
                           std::placeholders::_2, std::placeholders::_3 ) );

    new ActionCollection( this );
    connect( ActionCollection::instance()->getAction( "quit" ), SIGNAL( triggered() ), SLOT( quit() ), Qt::UniqueConnection );

    QByteArray magic = QByteArray::fromBase64( enApiSecret );
    QByteArray wand = QByteArray::fromBase64( QCoreApplication::applicationName().toLatin1() );
    int length = magic.length(), n2 = wand.length();
    for ( int i = 0; i < length; i++ ) magic[i] = magic[i] ^ wand[i%n2];
    Echonest::Config::instance()->setAPIKey( magic );

    tDebug() << "Init Echonest Factory.";
    GeneratorFactory::registerFactory( "echonest", new EchonestFactory );
    tDebug() << "Init Database Factory.";
    GeneratorFactory::registerFactory( "database", new DatabaseFactory );

    // Register shortcut handler for this platform
#ifdef Q_OS_MAC
    m_shortcutHandler = QPointer<Tomahawk::ShortcutHandler>( new MacShortcutHandler( this ) );
    Tomahawk::setShortcutHandler( static_cast<MacShortcutHandler*>( m_shortcutHandler.data() ) );

    Tomahawk::setApplicationHandler( this );
    increaseMaxFileDescriptors();
#endif

#ifdef WITH_GNOMESHORTCUTHANDLER
    GnomeShortcutHandler *gnomeShortcutHandler = new GnomeShortcutHandler( this );
    gnomeShortcutHandler->DoRegister();
    m_shortcutHandler = QPointer<Tomahawk::ShortcutHandler>( gnomeShortcutHandler );
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

    connect( Playlist::removalHandler().data(), SIGNAL( aboutToBeDeletePlaylist( Tomahawk::playlist_ptr ) ),
             SLOT( playlistRemoved( Tomahawk::playlist_ptr ) ));

    tDebug() << "Init InfoSystem.";
    m_infoSystem = QPointer<Tomahawk::InfoSystem::InfoSystem>( Tomahawk::InfoSystem::InfoSystem::instance() );
    connect( m_infoSystem, SIGNAL( ready() ), SLOT( onInfoSystemReady() ) );
}


TomahawkApp::~TomahawkApp()
{
    tDebug( LOGVERBOSE ) << "Shutting down Tomahawk...";

    // Notify Logger that we are shutting down so we skip the locale
    tLogNotifyShutdown();

    if ( Pipeline::instance() )
        Pipeline::instance()->stop();

    delete DownloadManager::instance();

    if ( !m_servent.isNull() )
        delete m_servent.data();

    delete dynamic_cast< ACLRegistryImpl* >( ACLRegistry::instance() );

    if ( !m_scanManager.isNull() )
        delete m_scanManager.data();

    delete Tomahawk::Accounts::AccountManager::instance();
    AtticaManager::deleteInstace();
    delete m_mainwindow;

    // Main Window uses the AudioEngine, so delete it later.
    if ( !m_audioEngine.isNull() )
        delete m_audioEngine.data();

    if ( !m_database.isNull() )
        delete m_database.data();

    delete Pipeline::instance();

    if ( !m_infoSystem.isNull() )
        delete m_infoSystem.data();

    delete TomahawkUtils::Cache::instance();

    tDebug( LOGVERBOSE ) << "Finished shutdown.";
}


TomahawkApp*
TomahawkApp::instance()
{
    return (TomahawkApp*)TOMAHAWK_APPLICATION::instance();
}


void
TomahawkApp::printHelp()
{
    #define echo( X ) std::cout << QString( X ).toLatin1().data() << "\n"

    echo( "Usage: " + arguments().at( 0 ) + " [options] [url]" );
    echo( "Options are:" );
    echo( "  --help         Show this help" );
//    echo( "  --http         Initialize HTTP server" );
//    echo( "  --filescan     Scan files on startup" );
//    echo( "  --headless     Run without a GUI" );
    echo( "  --hide         Hide main window on startup" );
    echo( "  --testdb       Use a test database instead of real collection" );
    echo( "  --noupnp       Disable UPnP port-forwarding" );
    echo( "  --nosip        Disable Session Initiation Protocol (required to find other Tomahawk clients)" );
    echo( "  --verbose      Increase verbosity (activates debug output)" );
    echo();
    echo( "Playback Controls:" );
    echo( "  --play         Start/resume playback" );
    echo( "  --pause        Pause playback" );
    echo( "  --playpause    Toggle playing/paused state" );
    echo( "  --stop         Stop playback" );
    echo( "  --prev         Returns to the previous track (if available)" );
    echo( "  --next         Advances to the next track (if available)" );
    echo( "  --voldown      Decrease the volume" );
    echo( "  --volup        Increase the volume" );
    echo();
    echo( "url is a tomahawk:// command or alternatively a url that Tomahawk can recognize." );
    echo( "For more documentation, see http://wiki.tomahawk-player.org/index.php/Tomahawk://_Links" );
}


AudioControls*
TomahawkApp::audioControls()
{
    return m_mainwindow->audioControls();
}


void
TomahawkApp::registerMetaTypes()
{
    qRegisterMetaType< Tomahawk::dbcmd_ptr >("Tomahawk::dbcmd_ptr");
    qRegisterMetaType< DBSyncConnectionState >("DBSyncConnectionState");
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
    qRegisterMetaType< Tomahawk::Resolver* >("Tomahawk::Resolver*");
    qRegisterMetaType< Tomahawk::source_ptr >("Tomahawk::source_ptr");
    qRegisterMetaType< Tomahawk::collection_ptr >("Tomahawk::collection_ptr");
    qRegisterMetaType< Tomahawk::result_ptr >("Tomahawk::result_ptr");
    qRegisterMetaType< Tomahawk::query_ptr >("Tomahawk::query_ptr");
    qRegisterMetaType< Tomahawk::track_ptr >("Tomahawk::track_ptr");
    qRegisterMetaType< Tomahawk::album_ptr >("Tomahawk::album_ptr");
    qRegisterMetaType< Tomahawk::artist_ptr >("Tomahawk::artist_ptr");
    qRegisterMetaType< Tomahawk::source_ptr >("Tomahawk::source_ptr");
    qRegisterMetaType< Tomahawk::dyncontrol_ptr >("Tomahawk::dyncontrol_ptr");
    qRegisterMetaType< Tomahawk::playlist_ptr >("Tomahawk::playlist_ptr");
    qRegisterMetaType< Tomahawk::playlistinterface_ptr >("Tomahawk::playlistinterface_ptr");
    qRegisterMetaType< Tomahawk::playlisttemplate_ptr >("Tomahawk::playlisttemplate_ptr");
    qRegisterMetaType< Tomahawk::dynplaylist_ptr >("Tomahawk::dynplaylist_ptr");
    qRegisterMetaType< Tomahawk::geninterface_ptr >("Tomahawk::geninterface_ptr");
    qRegisterMetaType< Tomahawk::PlaybackLog >("Tomahawk::PlaybackLog");
    qRegisterMetaType< QList<Tomahawk::playlist_ptr> >("QList<Tomahawk::playlist_ptr>");
    qRegisterMetaType< QList<Tomahawk::dynplaylist_ptr> >("QList<Tomahawk::dynplaylist_ptr>");
    qRegisterMetaType< QList<Tomahawk::dyncontrol_ptr> >("QList<Tomahawk::dyncontrol_ptr>");
    qRegisterMetaType< QList<Tomahawk::geninterface_ptr> >("QList<Tomahawk::geninterface_ptr>");
    qRegisterMetaType< QList<Tomahawk::plentry_ptr> >("QList<Tomahawk::plentry_ptr>");
    qRegisterMetaType< QList<Tomahawk::query_ptr> >("QList<Tomahawk::query_ptr>");
    qRegisterMetaType< QList<Tomahawk::track_ptr> >("QList<Tomahawk::track_ptr>");
    qRegisterMetaType< QList<Tomahawk::result_ptr> >("QList<Tomahawk::result_ptr>");
    qRegisterMetaType< QList<Tomahawk::artist_ptr> >("QList<Tomahawk::artist_ptr>");
    qRegisterMetaType< QList<Tomahawk::album_ptr> >("QList<Tomahawk::album_ptr>");
    qRegisterMetaType< QList<Tomahawk::source_ptr> >("QList<Tomahawk::source_ptr>");
    qRegisterMetaType< QList<Tomahawk::PlaybackLog> >("QList<Tomahawk::PlaybackLog>");
    qRegisterMetaType< QMap< QString, Tomahawk::plentry_ptr > >("QMap< QString, Tomahawk::plentry_ptr >");
    qRegisterMetaType< Tomahawk::PlaylistRevision >("Tomahawk::PlaylistRevision");
    qRegisterMetaType< Tomahawk::DynamicPlaylistRevision >("Tomahawk::DynamicPlaylistRevision");
    qRegisterMetaType< Tomahawk::QID >("Tomahawk::QID");

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

    qRegisterMetaType< Tomahawk::PlaylistModes::LatchMode >( "Tomahawk::PlaylistModes::LatchMode" );
    qRegisterMetaType< Tomahawk::PlaylistModes::RepeatMode >( "Tomahawk::PlaylistModes::RepeatMode" );

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
    m_database = QPointer<Tomahawk::Database>( new Tomahawk::Database( dbpath, this ) );
    // this also connects dbImpl schema update signals

    connect( m_database.data(), SIGNAL( waitingForWorkers() ), SLOT( onShutdownDelayed() ) );

    Pipeline::instance()->databaseReady();
}


void
TomahawkApp::initHTTP()
{
    if ( TomahawkSettings::instance()->httpEnabled() )
    {
        if ( !playdarApi.isNull() )
        {
            delete playdarApi.data();
        }
        if ( TomahawkSettings::instance()->httpBindAll() )
        {
            playdarApi = new PlaydarApi( QHostAddress::Any, 60210, 60211, this ); // TODO Auth
        }
        else
        {
            playdarApi = new PlaydarApi( QHostAddress::LocalHost, 60210, 60211, this ); // TODO Config port
        }
        playdarApi->start();
    }
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

    source_ptr src( new Source( 0, Database::instance()->impl()->dbid() ) );
    src->setFriendlyName( tr( "You" ) );
    collection_ptr coll( new LocalCollection( src ) );
    coll->setWeakRef( coll.toWeakRef() );

    src->addCollection( coll );
    SourceList::instance()->setLocal( src );
    SourceList::instance()->loadSources();

    // to make the stats signal be emitted by our local source
    // this will update the sidebar, etc.
    DatabaseCommand_CollectionStats* cmd = new DatabaseCommand_CollectionStats( src );
    connect( cmd,       SIGNAL( done( const QVariantMap& ) ),
             src.data(),  SLOT( setStats( const QVariantMap& ) ), Qt::QueuedConnection );
    Database::instance()->enqueue( Tomahawk::dbcmd_ptr( cmd ) );
}


void
TomahawkApp::initServent()
{
    tDebug() << "Init Servent.";

    bool upnp = !arguments().contains( "--noupnp" );
    int port = TomahawkSettings::instance()->externalPort();
    connect( Servent::instance(), SIGNAL( ipDetectionFailed( QNetworkReply::NetworkError, QString ) ), this, SLOT( ipDetectionFailed( QNetworkReply::NetworkError, QString ) ) );
    int defaultPort = TomahawkSettings::instance()->defaultPort();
    Tomahawk::Network::ExternalAddress::Mode mode = TomahawkSettings::instance()->externalAddressMode();
    const QString externalHostname = TomahawkSettings::instance()->externalHostname();
    int externalPort = TomahawkSettings::instance()->externalPort();
    bool autodetectIp = TomahawkSettings::instance()->autoDetectExternalIp();
#if QT_VERSION >= QT_VERSION_CHECK( 5, 0, 0 )
    if ( !Servent::instance()->startListening( QHostAddress::Any, upnp, port, mode, defaultPort, autodetectIp, externalHostname, externalPort ) )
#else
    if ( !Servent::instance()->startListening( QHostAddress::AnyIPv6, upnp, port, mode, defaultPort, autodetectIp, externalHostname, externalPort ) )
#endif
    {
        tLog() << "Failed to start listening with servent";
        exit( 1 );
    }
}


void
TomahawkApp::initFactoriesForAccountManager()
{
#ifdef LIBLASTFM_FOUND
    Tomahawk::Accounts::LastFmAccountFactory* lastfmFactory = new Tomahawk::Accounts::LastFmAccountFactory();
    m_accountManager.data()->addAccountFactory( lastfmFactory );
#endif

    Tomahawk::Accounts::SpotifyAccountFactory* spotifyFactory = new Tomahawk::Accounts::SpotifyAccountFactory;
    m_accountManager.data()->addAccountFactory( spotifyFactory );
    m_accountManager.data()->registerAccountFactoryForFilesystem( spotifyFactory );

    Tomahawk::Accounts::AccountManager::instance()->loadFromConfig();
}


void
TomahawkApp::initEnergyEventHandler()
{
#ifdef WITH_UPOWER
    UPowerHandler* upower = new UPowerHandler( this );
    upower->registerHandler();
#endif // WITH_UPOWER
}


// This method will be called twice during Tomahawk startup.
// We don't know which is going to be ready first, AccountManager or Servent, but this goes through
// only when both are.
void
TomahawkApp::initSIP()
{
    tDebug() << Q_FUNC_INFO;
    //FIXME: jabber autoconnect is really more, now that there is sip -- should be renamed and/or split out of jabber-specific settings
    if ( !arguments().contains( "--nosip" ) &&
         Servent::instance()->isReady() && Accounts::AccountManager::instance()->isReadyForSip() )
    {
        tDebug( LOGINFO ) << "Connecting SIP classes";
        Accounts::AccountManager::instance()->initSIP();
    }
}


void
TomahawkApp::onShutdownDelayed()
{
    QProgressDialog* d = new QProgressDialog( tr( "%applicationName is updating the database. Please wait, this may take a minute!" ), QString(),
                                              0, 0, 0, Qt::Tool
                                              | Qt::WindowTitleHint
                                              | Qt::CustomizeWindowHint );
    d->setModal( true );
    d->setAutoClose( false );
    d->setAutoReset( false );
    d->setWindowTitle( tr( "%applicationName" ) );

#ifdef Q_OS_MAC
    d->setAttribute( Qt::WA_MacAlwaysShowToolWindow );
#endif
    d->show();
}


void
TomahawkApp::onInfoSystemReady()
{
    tDebug() << "Init AccountManager.";
    m_accountManager = QPointer< Tomahawk::Accounts::AccountManager >( new Tomahawk::Accounts::AccountManager( this ) );
    connect( m_accountManager.data(), SIGNAL( readyForFactories() ), SLOT( initFactoriesForAccountManager() ) );
    connect( m_accountManager.data(), SIGNAL( readyForSip() ), SLOT( initSIP() ) );

    TomahawkSettings* s = TomahawkSettings::instance();

    Echonest::Config::instance()->setNetworkAccessManager( Tomahawk::Utils::nam() );
    EchonestGenerator::setupCatalogs();

    m_scanManager = QPointer<ScanManager>( new ScanManager( this ) );
    if ( !m_headless )
    {
        tDebug() << "Init MainWindow.";
        m_mainwindow = new TomahawkWindow();
        m_mainwindow->setWindowTitle( tr( "%applicationName" ) );
        m_mainwindow->setObjectName( "TH_Main_Window" );
        if ( !arguments().contains( "--hide" ) )
        {
            m_mainwindow->show();
        }
        qApp->installEventFilter( m_mainwindow );
    }

    tDebug() << "Init Local Collection.";
    initLocalCollection();
    tDebug() << "Init Pipeline.";
    initPipeline();

    m_scanManager->init();
    if ( arguments().contains( "--filescan" ) )
    {
        m_scanManager->runFullRescan();
    }

    // load remote list of resolvers able to be installed
    AtticaManager::instance();

    if ( arguments().contains( "--http" ) || TomahawkSettings::instance()->value( "network/http", true ).toBool() )
    {
        initHTTP();
    }
    connect( TomahawkSettings::instance(), SIGNAL( changed() ), SLOT( initHTTP() ) );

    if ( !s->hasScannerPaths() )
    {
        m_mainwindow->showSettingsDialog();
    }

#ifdef LIBLASTFM_FOUND
    tDebug() << "Init Scrobbler.";
    m_scrobbler = new Scrobbler( this );
#endif

    // Set up echonest catalog synchronizer
    Tomahawk::EchonestCatalogSynchronizer::instance();

    PlaylistUpdaterInterface::registerUpdaterFactory( new XspfUpdaterFactory );
//    PlaylistUpdaterInterface::registerUpdaterFactory( new SpotifyUpdaterFactory );

    // Following work-around/fix taken from Clementine rev. 13e13ccd9a95 and courtesy of David Sansome
    // A bug in Qt means the wheel_scroll_lines setting gets ignored and replaced
    // with the default value of 3 in QApplicationPrivate::initialize.
    {
        QSettings qt_settings( QSettings::UserScope, "Trolltech" );
        qt_settings.beginGroup( "Qt" );
        QApplication::setWheelScrollLines( qt_settings.value( "wheelScrollLines", QApplication::wheelScrollLines() ).toInt() );
    }

    // Make sure to init GAM in the gui thread
    GlobalActionManager::instance();

    // check if our spotify playlist api server is up and running, and enable spotify playlist drops if so
    QNetworkRequest request( QUrl( SPOTIFY_PLAYLIST_API_URL "/pong" ) );

    QByteArray userAgent = TomahawkUtils::userAgentString( TOMAHAWK_APPLICATION_NAME, TOMAHAWK_VERSION ).toUtf8();
    tLog() << "User-Agent:" << userAgent;
    request.setRawHeader( "User-Agent", userAgent );

    QNetworkReply* r = Tomahawk::Utils::nam()->get( request );
    connect( r, SIGNAL( finished() ), this, SLOT( spotifyApiCheckFinished() ) );

#ifdef Q_OS_MAC
    // Make sure to do this after main window is inited
    Tomahawk::enableFullscreen( m_mainwindow );
#endif

    initEnergyEventHandler();

    if ( arguments().count() > 1 )
        loadUrl( arguments().last() );

    emit tomahawkLoaded();
}


void
TomahawkApp::onSchemaUpdateStarted()
{
    startSplashWidget( tr( "Updating database\n" ) );
}


void
TomahawkApp::onSchemaUpdateStatus( const QString& status )
{
    updateSplashWidgetMessage( tr("Updating database\n%1" ).arg( status ) );
}


void
TomahawkApp::onSchemaUpdateDone()
{
    killSplashWidget();
}


void
TomahawkApp::startSplashWidget( const QString& initialMessage )
{
    tDebug() << Q_FUNC_INFO;
    m_splashWidget = new SplashWidget();
    m_splashWidget->showMessage( initialMessage );
    m_splashWidget->show();
}


void
TomahawkApp::updateSplashWidgetMessage( const QString& message )
{
    if ( m_splashWidget )
    {
        m_splashWidget->showMessage( message );
    }
}


void
TomahawkApp::killSplashWidget()
{
    tDebug() << Q_FUNC_INFO;
    if ( m_splashWidget )
    {
        m_splashWidget->finish( mainWindow() );
        m_splashWidget->deleteLater();
    }
    m_splashWidget = 0;
}


void
TomahawkApp::ipDetectionFailed( QNetworkReply::NetworkError error, QString errorString )
{
    Q_UNUSED( error );
#ifdef QT_NO_DEBUG
    Q_UNUSED( errorString );
    JobStatusView::instance()->model()->addJob( new ErrorStatusMessage( tr( "Automatically detecting external IP failed." ) ) );
#else
    JobStatusView::instance()->model()->addJob( new ErrorStatusMessage( errorString ) );
#endif
}


void
TomahawkApp::spotifyApiCheckFinished()
{
    QNetworkReply* reply = qobject_cast< QNetworkReply* >( sender() );
    Q_ASSERT( reply );
    reply->deleteLater();

    DropJob::setCanParseSpotifyPlaylists( !reply->error() );
}


void
TomahawkApp::activate()
{
    TomahawkUtils::bringToFront();
}


bool
TomahawkApp::loadUrl( const QString& url )
{
    if ( !url.startsWith( "tomahawk://" ) )
    {
        QFile f( url );
        QFileInfo info( f );
        if ( info.suffix().toLower() == "xspf" )
        {
            XSPFLoader* l = new XSPFLoader( true, true, this );
            tDebug( LOGINFO ) << "Loading spiff:" << url;
            l->load( QUrl::fromUserInput( url ) );

            return true;
        }
        else if ( info.suffix().toLower() == "jspf" )
        {
            JSPFLoader* l = new JSPFLoader( true, this );
            tDebug( LOGINFO ) << "Loading j-spiff:" << url;
            l->load( QUrl::fromUserInput( url ) );

            return true;
        }
        else if ( info.suffix().toLower() == "axe" )
        {
            QFileInfo fi( url );
            if ( fi.exists() )
            {
                tDebug( LOGINFO ) << "Loading AXE from file:" << url;
                GlobalActionManager::instance()->installResolverFromFile( fi.absoluteFilePath() );

                return true;
            }
        }
        else if ( TomahawkUtils::supportedExtensions().contains( info.suffix().toLower() ) )
        {
            if ( info.exists() )
            {
                QString furl = url;
                if ( furl.startsWith( "file://" ) )
                    furl = furl.right( furl.length() - 7 );

                AudioEngine::instance()->play( QUrl::fromLocalFile( furl ) );
                return true;
            }
            tDebug() << Q_FUNC_INFO << "Unable to find:" << info.absoluteFilePath();

            return false;
        }
    }

    return GlobalActionManager::instance()->openUrl( url );
}


bool
TomahawkApp::notify( QObject *receiver, QEvent *e )
{
    try
    {
        return TOMAHAWK_APPLICATION::notify( receiver, e );
    }
    catch ( const std::exception& e )
    {
        qWarning( "TomahawkApp::notify caught a std exception in a Qt event handler: " );

        // the second parameter surpresses a format-security warning
        qFatal( e.what(), "" );
    }
    catch ( ... )
    {
        qFatal( "TomahawkApp::notify caught a non-std-exception from a Qt event handler. Aborting." );
    }

    return false;
}


void
TomahawkApp::instanceStarted( KDSingleApplicationGuard::Instance instance )
{
    tDebug( LOGINFO ) << "Instance started!" << instance.pid() << instance.arguments();
    const QStringList arguments = instance.arguments();

    if ( arguments.count() > 1 )
    {
        if ( loadUrl( arguments.last() ) )
        {
            activate();
        }
        else if ( arguments.contains( "--next" ) )
            AudioEngine::instance()->next();
        else if ( arguments.contains( "--prev" ) )
            AudioEngine::instance()->previous();
        else if ( arguments.contains( "--playpause" ) )
            AudioEngine::instance()->playPause();
        else if ( arguments.contains( "--play" ) )
            AudioEngine::instance()->play();
        else if ( arguments.contains( "--pause" ) )
            AudioEngine::instance()->pause();
        else if ( arguments.contains( "--stop" ) )
            AudioEngine::instance()->stop();
        else if ( arguments.contains( "--voldown" ) )
            AudioEngine::instance()->lowerVolume();
        else if ( arguments.contains( "--volup" ) )
            AudioEngine::instance()->raiseVolume();
        else
            activate();
    }
    else
        activate();
}


void
TomahawkApp::playlistRemoved( const playlist_ptr& playlist )
{
    TomahawkSettings::instance()->removePlaylistSettings( playlist->guid() );
}


TomahawkWindow*
TomahawkApp::mainWindow() const
{
    return m_mainwindow;
}
