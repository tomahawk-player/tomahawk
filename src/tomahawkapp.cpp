/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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
#include "web/api_v1.h"
#include "resolvers/scriptresolver.h"
#include "resolvers/qtscriptresolver.h"
#include "sourcelist.h"
#include "shortcuthandler.h"
#include "scanmanager.h"
#include "tomahawksettings.h"
#include "globalactionmanager.h"
#include "webcollection.h"
#include "database/localcollection.h"
#include "musicscanner.h"

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
    qDebug() << "TomahawkApp thread:" << thread();

    setOrganizationName( QLatin1String( TOMAHAWK_ORGANIZATION_NAME ) );
    setOrganizationDomain( QLatin1String( TOMAHAWK_ORGANIZATION_DOMAIN ) );
    setApplicationName( QLatin1String( TOMAHAWK_APPLICATION_NAME ) );
    setApplicationVersion( QLatin1String( TOMAHAWK_VERSION ) );

    registerMetaTypes();
}


void
TomahawkApp::init()
{
    Logger::setupLogfile();
    qsrand( QTime( 0, 0, 0 ).secsTo( QTime::currentTime() ) );

    tLog() << "Starting Tomahawk...";

#ifdef TOMAHAWK_HEADLESS
    m_headless = true;
#else
    m_mainwindow = 0;
    m_headless = arguments().contains( "--headless" );
    setWindowIcon( QIcon( RESPATH "icons/tomahawk-icon-128x128.png" ) );
#endif

    registerMetaTypes();

    new TomahawkSettings( this );
    TomahawkSettings* s = TomahawkSettings::instance();

    tDebug( LOGINFO ) << "Setting NAM.";
#ifdef LIBLASTFM_FOUND
    TomahawkUtils::setNam( lastfm::nam() );
#else
    TomahawkUtils::setNam( new QNetworkAccessManager() );
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

    Echonest::Config::instance()->setAPIKey( "JRIHWEP6GPOER2QQ6" );

    m_audioEngine = QWeakPointer<AudioEngine>( new AudioEngine );
    m_scanManager = QWeakPointer<ScanManager>( new ScanManager( this ) );
    new Pipeline( this );

    m_servent = QWeakPointer<Servent>( new Servent( this ) );
    connect( m_servent.data(), SIGNAL( ready() ), SLOT( initSIP() ) );

    tDebug() << "Init Database.";
    initDatabase();

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

    setQuitOnLastWindowClosed( false );
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

    Echonest::Config::instance()->setAPIKey( "JRIHWEP6GPOER2QQ6" );
    Echonest::Config::instance()->setNetworkAccessManager( TomahawkUtils::nam() );

#ifndef TOMAHAWK_HEADLESS
    if ( !m_headless )
    {
        tDebug() << "Init MainWindow.";
        m_mainwindow = new TomahawkWindow();
        m_mainwindow->setWindowTitle( "Tomahawk" );
        m_mainwindow->show();
    }
#endif

    tDebug() << "Init Local Collection.";
    initLocalCollection();
    tDebug() << "Init Pipeline.";
    initPipeline();

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
}


TomahawkApp::~TomahawkApp()
{
    tLog() << "Shutting down Tomahawk...";

    // stop script resolvers
    foreach( Tomahawk::ExternalResolver* r, m_scriptResolvers.values() )
    {
        delete r;
    }
    m_scriptResolvers.clear();

    if ( !m_servent.isNull() )
        delete m_servent.data();
    if ( !m_scanManager.isNull() )
        delete m_scanManager.data();

#ifndef TOMAHAWK_HEADLESS
    delete m_mainwindow;
#endif

    if ( !m_audioEngine.isNull() )
        delete m_audioEngine.data();

    if ( !m_infoSystem.isNull() )
        delete m_infoSystem.data();

    //FIXME: delete GeneratorFactory::registerFactory( "echonest", new EchonestFactory ); ?

    delete SipHandler::instance();

    if ( !m_scanManager.isNull() )
        delete m_scanManager.data();
    if ( !m_database.isNull() )
        delete m_database.data();

    Pipeline::instance()->stop();
    delete Pipeline::instance();

    tLog() << "Finished shutdown.";
}


TomahawkApp*
TomahawkApp::instance()
{
    return (TomahawkApp*)TOMAHAWK_APPLICATION::instance();
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
    qRegisterMetaType< QList<QVariantMap> >("QList<QVariantMap>");
    qRegisterMetaType< DBSyncConnection::State >("DBSyncConnection::State");
    qRegisterMetaType< msg_ptr >("msg_ptr");
    qRegisterMetaType< QList<dbop_ptr> >("QList<dbop_ptr>");
    qRegisterMetaType< QList<QString> >("QList<QString>");
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

    qRegisterMetaType< QMap< QString, QMap< QString, QString > > >( "Tomahawk::InfoSystem::InfoGenericMap" );
    qRegisterMetaType< QHash< QString, QString > >( "Tomahawk::InfoSystem::InfoCriteriaHash" );
    qRegisterMetaType< Tomahawk::InfoSystem::InfoType >( "Tomahawk::InfoSystem::InfoType" );
    qRegisterMetaType< Tomahawk::InfoSystem::InfoRequestData >( "Tomahawk::InfoSystem::InfoRequestData" );
    qRegisterMetaType< QWeakPointer< Tomahawk::InfoSystem::InfoSystemCache > >( "QWeakPointer< Tomahawk::InfoSystem::InfoSystemCache >" );

    qRegisterMetaType< DirLister::Mode >("DirLister::Mode");
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
    foreach( QString resolver, TomahawkSettings::instance()->enabledScriptResolvers() )
        enableScriptResolver( resolver );
}


void
TomahawkApp::enableScriptResolver( const QString& path )
{
    const QFileInfo fi( path );
    if ( fi.suffix() == "js" || fi.suffix() == "script" )
        m_scriptResolvers.insert( path, new QtScriptResolver( path ) );
    else
        m_scriptResolvers.insert( path, new ScriptResolver( path ) );
}


void
TomahawkApp::disableScriptResolver( const QString& path )
{
    if ( m_scriptResolvers.contains( path ) )
    {
        Tomahawk::ExternalResolver* r = m_scriptResolvers.take( path );

        connect( r, SIGNAL( finished() ), r, SLOT( deleteLater() ) );
        r->stop();
        return;
    }
}


Tomahawk::ExternalResolver*
TomahawkApp::resolverForPath( const QString& scriptPath )
{
    return m_scriptResolvers.value( scriptPath, 0 );
}


void
TomahawkApp::initLocalCollection()
{
    connect( SourceList::instance(), SIGNAL( ready() ), SLOT( initServent() ) );

    source_ptr src( new Source( 0, "My Collection" ) );
    collection_ptr coll( new LocalCollection( src ) );

    src->addCollection( coll );
    SourceList::instance()->setLocal( src );
//    src->collection()->tracks();

    // dummy source/collection for web-based result-hints.
    source_ptr dummy( new Source( -1, "" ) );
    dummy->setOnline();
    collection_ptr dummycol( new WebCollection( dummy ) );
    dummy->addCollection( dummycol );
    SourceList::instance()->setWebSource( dummy );
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
TomahawkApp::activate()
{
#ifndef TOMAHAWK_HEADLESS
    TomahawkUtils::bringToFront();
#endif
}


bool
TomahawkApp::loadUrl( const QString& url )
{
    activate();
    if ( url.startsWith( "tomahawk://" ) )
        return GlobalActionManager::instance()->parseTomahawkLink( url );
    else if ( url.contains( "open.spotify.com" ) || url.contains( "spotify:track" ) )
        return GlobalActionManager::instance()->openSpotifyLink( url );
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
        } else if ( info.suffix() == "jspf" )
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

    if ( instance.arguments.size() < 2 )
    {
        return;
    }

    QString arg1 = instance.arguments[ 1 ];
    loadUrl( arg1 );
}

