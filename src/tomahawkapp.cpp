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
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */

#include "tomahawkapp.h"

#include "config.h"

#include <iostream>

#include <QPluginLoader>
#include <QDir>
#include <QMetaType>
#include <QTime>
#include <QNetworkReply>
#include <QFile>
#include <QFileInfo>
#include <QNetworkProxy>

#include "artist.h"
#include "album.h"
#include "collection.h"
#include "infosystem/infosystem.h"
#include "database/database.h"
#include "database/databasecollection.h"
#include "database/databasecommand_collectionstats.h"
#include "database/databaseresolver.h"
#include "sip/SipHandler.h"
#include "playlist/dynamic/GeneratorFactory.h"
#include "playlist/dynamic/echonest/EchonestGenerator.h"
#include "playlist/dynamic/database/DatabaseGenerator.h"
#include "network/servent.h"
#include "web/api_v1.h"
#include "sourcelist.h"
#include "shortcuthandler.h"
#include "scanmanager.h"
#include "tomahawksettings.h"
#include "globalactionmanager.h"
#include "database/localcollection.h"
#include "musicscanner.h"
#include "AtticaManager.h"
#include "pipeline.h"
#include "utils/spotifyparser.h"
#include "dropjob.h"
#include "EchonestCatalogSynchronizer.h"
#include "widgets/HeaderLabel.h"

#include "audio/audioengine.h"
#include "utils/xspfloader.h"
#include "utils/jspfloader.h"
#include "utils/logger.h"
#include "utils/tomahawkutils.h"

#include <lastfm/ws.h>
#include "config.h"

#ifndef TOMAHAWK_HEADLESS
    #include "tomahawkwindow.h"
    #include "settingsdialog.h"
    #include <QMessageBox>
#endif

// should go to a plugin actually
#ifdef GLOOX_FOUND
    #include "xmppbot/xmppbot.h"
#endif

#ifdef Q_WS_MAC
#include "mac/macshortcuthandler.h"

#include <sys/resource.h>
#include <sys/sysctl.h>
#endif


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
        ::exit(0);
    }

    qDebug() << "TomahawkApp thread:" << thread();
    Logger::setupLogfile();
    qsrand( QTime( 0, 0, 0 ).secsTo( QTime::currentTime() ) );

    tLog() << "Starting Tomahawk...";

#ifdef TOMAHAWK_HEADLESS
    m_headless = true;
#else
    m_mainwindow = 0;
    m_headless = arguments().contains( "--headless" );
    setWindowIcon( QIcon( RESPATH "icons/tomahawk-icon-128x128.png" ) );
    setQuitOnLastWindowClosed( false );
#endif

    QFont f = APP->font();
    f.setPixelSize( HeaderLabel::defaultFontSize() );
    QFontMetrics fm( f );
    TomahawkUtils::setHeaderHeight( fm.height() + 8 );

    new TomahawkSettings( this );
    TomahawkSettings* s = TomahawkSettings::instance();

    tDebug( LOGINFO ) << "Setting NAM.";
#ifndef LIBLASTFM_FOUND
    TomahawkUtils::setNam( new QNetworkAccessManager() );
#else
    TomahawkUtils::setNam( lastfm::nam() );
    //Ensure that liblastfm2 won't delete the nam out from under us, even though they created it
    lastfm::setNetworkAccessManager( TomahawkUtils::nam() );
#endif

    TomahawkUtils::NetworkProxyFactory* proxyFactory = new TomahawkUtils::NetworkProxyFactory();
    if ( s->proxyType() != QNetworkProxy::NoProxy && !s->proxyHost().isEmpty() )
    {
        tDebug( LOGEXTRA ) << "Setting proxy to saved values";
        QNetworkProxy proxy( static_cast<QNetworkProxy::ProxyType>( s->proxyType() ), s->proxyHost(), s->proxyPort(), s->proxyUsername(), s->proxyPassword() );
        proxyFactory->setProxy( proxy );
        //TODO: On Windows and Mac because liblastfm sets an application level proxy it may override our factory, so may need to explicitly do
        //a QNetworkProxy::setApplicationProxy with our own proxy (but then also overriding our own factory :-( )
    }
    if ( !s->proxyNoProxyHosts().isEmpty() )
        proxyFactory->setNoProxyHosts( s->proxyNoProxyHosts().split( ',', QString::SkipEmptyParts ) );

    TomahawkUtils::setProxyFactory( proxyFactory );
    

    m_audioEngine = QWeakPointer<AudioEngine>( new AudioEngine );
    m_scanManager = QWeakPointer<ScanManager>( new ScanManager( this ) );
    new Pipeline( this );

    m_servent = QWeakPointer<Servent>( new Servent( this ) );
    connect( m_servent.data(), SIGNAL( ready() ), SLOT( initSIP() ) );

    tDebug() << "Init Database.";
    initDatabase();

    Echonest::Config::instance()->setAPIKey( "JRIHWEP6GPOER2QQ6" );

    tDebug() << "Init Echonest Factory.";
    GeneratorFactory::registerFactory( "echonest", new EchonestFactory );
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
    m_infoSystem = QWeakPointer<Tomahawk::InfoSystem::InfoSystem>( new Tomahawk::InfoSystem::InfoSystem( this ) );

    Echonest::Config::instance()->setNetworkAccessManager( TomahawkUtils::nam() );
    EchonestGenerator::setupCatalogs();

#ifndef TOMAHAWK_HEADLESS
    if ( !m_headless )
    {
        tDebug() << "Init MainWindow.";
        m_mainwindow = new TomahawkWindow();
        m_mainwindow->setWindowTitle( "Tomahawk" );
        m_mainwindow->setObjectName( "TH_Main_Window" );
        m_mainwindow->show();
    }
#endif

    tDebug() << "Init Local Collection.";
    initLocalCollection();
    tDebug() << "Init Pipeline.";
    initPipeline();

#ifdef LIBATTICA_FOUND
    // load remote list of resolvers able to be installed
    AtticaManager::instance();
#endif

    if ( arguments().contains( "--http" ) || TomahawkSettings::instance()->value( "network/http", true ).toBool() )
    {
        initHTTP();
    }

#ifndef TOMAHAWK_HEADLESS
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

    // Make sure to init GAM in the gui thread
    GlobalActionManager::instance();

    // Set up echonest catalog synchronizer
    Tomahawk::EchonestCatalogSynchronizer::instance();
    // check if our spotify playlist api server is up and running, and enable spotify playlist drops if so
    QNetworkReply* r = TomahawkUtils::nam()->get( QNetworkRequest( QUrl( SPOTIFY_PLAYLIST_API_URL "/playlist/test" ) ) );
    connect( r, SIGNAL( finished() ), this, SLOT( spotifyApiCheckFinished() ) );

#ifdef Q_WS_MAC
    // Make sure to do this after main window is inited
    Tomahawk::enableFullscreen();
#endif
}


TomahawkApp::~TomahawkApp()
{
    tLog() << "Shutting down Tomahawk...";

    if ( !m_servent.isNull() )
        delete m_servent.data();
    if ( !m_scanManager.isNull() )
        delete m_scanManager.data();

    if ( !m_audioEngine.isNull() )
        delete m_audioEngine.data();
    if ( !m_infoSystem.isNull() )
        delete m_infoSystem.data();

    //FIXME: delete GeneratorFactory::registerFactory( "echonest", new EchonestFactory ); ?

    delete SipHandler::instance();

    Pipeline::instance()->stop();

    if ( !m_database.isNull() )
        delete m_database.data();

    delete Pipeline::instance();

#ifndef TOMAHAWK_HEADLESS
    delete m_mainwindow;
#endif
#ifdef LIBATTICA_FOUND
    delete AtticaManager::instance();
#endif

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
    echo( "  --filescan     Scan for files on startup\n" );
    echo( "  --testdb       Use a test database instead of real collection\n" );
    echo( "  --noupnp       Disable UPNP\n" );
    echo( "  --nosip        Disable SIP\n" );
    echo( "\nurl is a tomahawk:// command or alternatively a url that Tomahawk can recognize.\n" );
    echo( "For more documentation, see http://wiki.tomahawk-player.org/mediawiki/index.php/Tomahawk://_Links\n" );
}


#ifndef TOMAHAWK_HEADLESS
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

    qRegisterMetaType< GeneratorMode>("GeneratorMode");
    qRegisterMetaType<Tomahawk::GeneratorMode>("Tomahawk::GeneratorMode");

    // Extra definition for namespaced-versions of signals/slots required
    qRegisterMetaType< Tomahawk::source_ptr >("Tomahawk::source_ptr");
    qRegisterMetaType< Tomahawk::collection_ptr >("Tomahawk::collection_ptr");
    qRegisterMetaType< Tomahawk::result_ptr >("Tomahawk::result_ptr");
    qRegisterMetaType< Tomahawk::query_ptr >("Tomahawk::query_ptr");
    qRegisterMetaType< Tomahawk::source_ptr >("Tomahawk::source_ptr");
    qRegisterMetaType< Tomahawk::dyncontrol_ptr >("Tomahawk::dyncontrol_ptr");
    qRegisterMetaType< Tomahawk::playlist_ptr >("Tomahawk::playlist_ptr");
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

    qRegisterMetaType< QHash< QString, QString > >( "Tomahawk::InfoSystem::InfoStringHash" );
    qRegisterMetaType< Tomahawk::InfoSystem::InfoType >( "Tomahawk::InfoSystem::InfoType" );
    qRegisterMetaType< Tomahawk::InfoSystem::InfoRequestData >( "Tomahawk::InfoSystem::InfoRequestData" );
    qRegisterMetaType< QWeakPointer< Tomahawk::InfoSystem::InfoSystemCache > >( "QWeakPointer< Tomahawk::InfoSystem::InfoSystemCache >" );

    qRegisterMetaType< QList<Tomahawk::InfoSystem::InfoStringHash> >("QList<Tomahawk::InfoSystem::InfoStringHash>");
    qRegisterMetaType< QPersistentModelIndex >( "QPersistentModelIndex" );
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
    m_session.setPort( 60210 ); //TODO config
    m_session.setListenInterface( QHostAddress::LocalHost );
    m_session.setConnector( &m_connector );

    Api_v1* api = new Api_v1( &m_session );
    m_session.setStaticContentService( api );

    tLog() << "Starting HTTPd on" << m_session.listenInterface().toString() << m_session.port();
    m_session.start();

}


void
TomahawkApp::initPipeline()
{
    // setup resolvers for local content, and (cached) remote collection content
    Pipeline::instance()->addResolver( new DatabaseResolver( 100 ) );
    // load script resolvers
    QStringList enabled = TomahawkSettings::instance()->enabledScriptResolvers();
    foreach ( QString resolver, TomahawkSettings::instance()->allScriptResolvers() )
    {
        const bool enable = enabled.contains( resolver );
        Pipeline::instance()->addScriptResolver( resolver, enable );
    }
}


void
TomahawkApp::initLocalCollection()
{
    connect( SourceList::instance(), SIGNAL( ready() ), SLOT( initServent() ) );

    source_ptr src( new Source( 0, "My Collection" ) );
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

    bool upnp = !arguments().contains( "--noupnp" ) && TomahawkSettings::instance()->value( "network/upnp", true ).toBool() && !TomahawkSettings::instance()->preferStaticHostPort();
    int port = TomahawkSettings::instance()->externalPort();
    if ( !Servent::instance()->startListening( QHostAddress( QHostAddress::Any ), upnp, port ) )
    {
        tLog() << "Failed to start listening with servent";
        exit( 1 );
    }
}


void
TomahawkApp::initSIP()
{
    //FIXME: jabber autoconnect is really more, now that there is sip -- should be renamed and/or split out of jabber-specific settings
    if ( !arguments().contains( "--nosip" ) )
    {
#ifdef GLOOX_FOUND
        m_xmppBot = QWeakPointer<XMPPBot>( new XMPPBot( this ) );
#endif

        tDebug( LOGINFO ) << "Connecting SIP classes";
        //SipHandler::instance()->refreshProxy();
        SipHandler::instance()->loadFromConfig( true );
    }
}


void
TomahawkApp::spotifyApiCheckFinished()
{
    QNetworkReply* reply = qobject_cast< QNetworkReply* >( sender() );
    Q_ASSERT( reply );

    if ( reply->error() == QNetworkReply::ContentNotFoundError )
        DropJob::setCanParseSpotifyPlaylists( true );
    else
        DropJob::setCanParseSpotifyPlaylists( false );
}


void
TomahawkApp::activate()
{
#ifndef TOMAHAWK_HEADLESS
    TomahawkUtils::bringToFront();
#endif
}


bool
TomahawkApp::loadUrl( const QString& url )
{
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

    return false;
}


void
TomahawkApp::instanceStarted( KDSingleApplicationGuard::Instance instance )
{
    tDebug( LOGINFO ) << "Instance started!" << instance.pid << instance.arguments;

    activate();
    if ( instance.arguments.size() < 2 )
        return;

    QString arg1 = instance.arguments[ 1 ];
    loadUrl( arg1 );
}

