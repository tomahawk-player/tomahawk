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

#include "tomahawk/tomahawkapp.h"

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
#include "utils/tomahawkutils.h"
#include "web/api_v1.h"
#include "resolvers/scriptresolver.h"
#include "resolvers/qtscriptresolver.h"
#include "sourcelist.h"
#include "shortcuthandler.h"
#include "scanmanager.h"
#include "tomahawksettings.h"

#include "audio/audioengine.h"
#include "utils/xspfloader.h"

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
#endif

#include <iostream>
#include <fstream>

#define LOGFILE TomahawkUtils::appLogDir().filePath( "Tomahawk.log" ).toLocal8Bit()
#define LOGFILE_SIZE 1024 * 512

using namespace std;
ofstream logfile;

void TomahawkLogHandler( QtMsgType type, const char *msg )
{
    static QMutex s_mutex;

    QMutexLocker locker( &s_mutex );
    switch( type )
    {
        case QtDebugMsg:
            logfile << QTime::currentTime().toString().toAscii().data() << " Debug: " << msg << "\n";
            break;

        case QtCriticalMsg:
            logfile << QTime::currentTime().toString().toAscii().data() << " Critical: " << msg << "\n";
            break;

        case QtWarningMsg:
            logfile << QTime::currentTime().toString().toAscii().data() << " Warning: " << msg << "\n";
            break;

        case QtFatalMsg:
            logfile << QTime::currentTime().toString().toAscii().data() << " Fatal: " << msg << "\n";
            logfile.flush();

            cout << msg << "\n";
            cout.flush();
            abort();
            break;
    }

    cout << msg << "\n";
    cout.flush();
    logfile.flush();
}

void setupLogfile()
{
    if ( QFileInfo( LOGFILE ).size() > LOGFILE_SIZE )
    {
        QByteArray lc;
        {
            QFile f( LOGFILE );
            f.open( QIODevice::ReadOnly | QIODevice::Text );
            lc = f.readAll();
            f.close();
        }

        QFile::remove( LOGFILE );

        {
            QFile f( LOGFILE );
            f.open( QIODevice::WriteOnly | QIODevice::Text );
            f.write( lc.right( LOGFILE_SIZE - (LOGFILE_SIZE / 4) ) );
            f.close();
        }
    }

    logfile.open( LOGFILE, ios::app );
    qInstallMsgHandler( TomahawkLogHandler );
}


using namespace Tomahawk;

TomahawkApp::TomahawkApp( int& argc, char *argv[] )
    : TOMAHAWK_APPLICATION( argc, argv )
    , m_database( 0 )
    , m_scanManager( 0 )
    , m_audioEngine( 0 )
    , m_sipHandler( 0 )
    , m_servent( 0 )
    , m_shortcutHandler( 0 )
    , m_scrubFriendlyName( false )
    , m_mainwindow( 0 )
{
    qDebug() << "TomahawkApp thread:" << this->thread();
    setOrganizationName( QLatin1String( ORGANIZATION_NAME ) );
    setOrganizationDomain( QLatin1String( ORGANIZATION_DOMAIN ) );
    setApplicationName( QLatin1String( APPLICATION_NAME ) );
    setApplicationVersion( QLatin1String( VERSION ) );
    setupLogfile();
}

void
TomahawkApp::init()
{
    qsrand( QTime( 0, 0, 0 ).secsTo( QTime::currentTime() ) );

    #ifdef TOMAHAWK_HEADLESS
    m_headless = true;
    #else
    m_mainwindow = 0;
    m_headless = arguments().contains( "--headless" );
    setWindowIcon( QIcon( RESPATH "icons/tomahawk-icon-128x128.png" ) );
    #endif

    registerMetaTypes();

    Echonest::Config::instance()->setAPIKey( "JRIHWEP6GPOER2QQ6" );

    new TomahawkSettings( this );
    m_audioEngine = new AudioEngine;
    m_scanManager = new ScanManager( this );
    new Pipeline( this );

    m_servent = new Servent( this );
    connect( m_servent, SIGNAL( ready() ), SLOT( setupSIP() ) );

    qDebug() << "Init Database.";
    setupDatabase();

    qDebug() << "Init Echonest Factory.";
    GeneratorFactory::registerFactory( "echonest", new EchonestFactory );

    m_scrubFriendlyName = arguments().contains( "--demo" );
    // Register shortcut handler for this platform
    #ifdef Q_WS_MAC
    m_shortcutHandler = new MacShortcutHandler( this );
    Tomahawk::setShortcutHandler( static_cast<MacShortcutHandler*>( m_shortcutHandler) );

    Tomahawk::setApplicationHandler( this );
    #endif

    // Connect up shortcuts
    if ( m_shortcutHandler )
    {
        connect( m_shortcutHandler, SIGNAL( playPause() ), m_audioEngine, SLOT( playPause() ) );
        connect( m_shortcutHandler, SIGNAL( pause() ), m_audioEngine, SLOT( pause() ) );
        connect( m_shortcutHandler, SIGNAL( stop() ), m_audioEngine, SLOT( stop() ) );
        connect( m_shortcutHandler, SIGNAL( previous() ), m_audioEngine, SLOT( previous() ) );
        connect( m_shortcutHandler, SIGNAL( next() ), m_audioEngine, SLOT( next() ) );
        connect( m_shortcutHandler, SIGNAL( volumeUp() ), m_audioEngine, SLOT( raiseVolume() ) );
        connect( m_shortcutHandler, SIGNAL( volumeDown() ), m_audioEngine, SLOT( lowerVolume() ) );
        connect( m_shortcutHandler, SIGNAL( mute() ), m_audioEngine, SLOT( mute() ) );
    }

    qDebug() << "Init InfoSystem.";
    m_infoSystem = new Tomahawk::InfoSystem::InfoSystem( this );

#ifdef LIBLASTFM_FOUND
    qDebug() << "Init Scrobbler.";
    m_scrobbler = new Scrobbler( this );
    qDebug() << "Setting NAM.";
    TomahawkUtils::setNam( lastfm::nam() );

    #else
    qDebug() << "Setting NAM.";
    TomahawkUtils::setNam( new QNetworkAccessManager );
    #endif

    // Set up proxy
    //FIXME: This overrides the lastfm proxy above?
    if( TomahawkSettings::instance()->proxyType() != QNetworkProxy::NoProxy &&
        !TomahawkSettings::instance()->proxyHost().isEmpty() )
    {
        qDebug() << "Setting proxy to saved values";
        TomahawkUtils::setProxy( new QNetworkProxy( static_cast<QNetworkProxy::ProxyType>(TomahawkSettings::instance()->proxyType()), TomahawkSettings::instance()->proxyHost(), TomahawkSettings::instance()->proxyPort(), TomahawkSettings::instance()->proxyUsername(), TomahawkSettings::instance()->proxyPassword() ) );
        qDebug() << "Proxy type =" << QString::number( static_cast<int>(TomahawkUtils::proxy()->type()) );
        qDebug() << "Proxy host =" << TomahawkUtils::proxy()->hostName();
        TomahawkUtils::nam()->setProxy( *TomahawkUtils::proxy() );
        lastfm::nam()->setProxy( *TomahawkUtils::proxy() );
    }
    else
        TomahawkUtils::setProxy( new QNetworkProxy( QNetworkProxy::NoProxy ) );


    Echonest::Config::instance()->setAPIKey( "JRIHWEP6GPOER2QQ6" );
    Echonest::Config::instance()->setNetworkAccessManager( TomahawkUtils::nam() );

    QNetworkProxy::setApplicationProxy( *TomahawkUtils::proxy() );

    qDebug() << "Init SIP system.";
    m_sipHandler = new SipHandler( this );

    #ifndef TOMAHAWK_HEADLESS
    if ( !m_headless )
    {
        qDebug() << "Init MainWindow.";
        m_mainwindow = new TomahawkWindow();
        m_mainwindow->setWindowTitle( "Tomahawk" );
        m_mainwindow->show();
    }
    #endif

    qDebug() << "Init Local Collection.";
    initLocalCollection();
    qDebug() << "Init Pipeline.";
    setupPipeline();
    qDebug() << "Init Servent.";
    startServent();

    if( arguments().contains( "--http" ) || TomahawkSettings::instance()->value( "network/http", true ).toBool() )
    {
        qDebug() << "Init HTTP Server.";
        startHTTP();
    }

#ifndef TOMAHAWK_HEADLESS
    if ( !TomahawkSettings::instance()->hasScannerPaths() )
    {
        m_mainwindow->showSettingsDialog();
    }
#endif
}


TomahawkApp::~TomahawkApp()
{
    qDebug() << Q_FUNC_INFO;

    // stop script resolvers
    foreach( Tomahawk::ExternalResolver* r, m_scriptResolvers )
    {
        delete r;
    }
    m_scriptResolvers.clear();

    delete m_sipHandler;
    delete m_servent;
    delete m_scanManager;
#ifndef TOMAHAWK_HEADLESS
    delete m_mainwindow;
    delete m_audioEngine;
#endif
    delete m_infoSystem;
    delete m_database;
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
    qRegisterMetaType< QHostAddress >("QHostAddress");
    qRegisterMetaType< QMap<QString, unsigned int> >("QMap<QString, unsigned int>");
    qRegisterMetaType< QMap< QString, plentry_ptr > >("QMap< QString, plentry_ptr >");
    qRegisterMetaType< QHash< QString, QMap<quint32, quint16> > >("QHash< QString, QMap<quint32, quint16> >");

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
}


void
TomahawkApp::setupDatabase()
{
    QString dbpath;
    if( arguments().contains( "--testdb" ) )
    {
        dbpath = QDir::currentPath() + "/test.db";
    }
    else
    {
        dbpath = TomahawkUtils::appDataDir().absoluteFilePath( "tomahawk.db" );
    }

    qDebug() << "Using database:" << dbpath;
    m_database = new Database( dbpath, this );
    Pipeline::instance()->databaseReady();
}


void
TomahawkApp::startHTTP()
{
    m_session.setPort( 60210 ); //TODO config
    m_session.setListenInterface( QHostAddress::LocalHost );
    m_session.setConnector( &m_connector );

    Api_v1* api = new Api_v1( &m_session );
    m_session.setStaticContentService( api );

    qDebug() << "Starting HTTPd on" << m_session.listenInterface().toString() << m_session.port();
    m_session.start();

}


void
TomahawkApp::setupPipeline()
{
    // setup resolvers for local content, and (cached) remote collection content
    Pipeline::instance()->addResolver( new DatabaseResolver( 100 ) );

    // load script resolvers
    foreach( QString resolver, TomahawkSettings::instance()->scriptResolvers() )
        addScriptResolver( resolver );
}


void
TomahawkApp::addScriptResolver( const QString& path )
{
    const QFileInfo fi( path );
    if ( fi.suffix() == "js" || fi.suffix() == "script" )
        m_scriptResolvers << new QtScriptResolver( path );
    else
        m_scriptResolvers << new ScriptResolver( path );
}


void
TomahawkApp::removeScriptResolver( const QString& path )
{
    foreach( Tomahawk::ExternalResolver* r, m_scriptResolvers )
    {
        if( r->filePath() == path )
        {
            m_scriptResolvers.removeAll( r );
            connect( r, SIGNAL( finished() ), r, SLOT( deleteLater() ) );
            r->stop();
            return;
        }
    }
}


void
TomahawkApp::initLocalCollection()
{
    source_ptr src( new Source( 0, "My Collection" ) );
    collection_ptr coll( new DatabaseCollection( src ) );

    src->addCollection( coll );
    SourceList::instance()->setLocal( src );
//    src->collection()->tracks();

    // to make the stats signal be emitted by our local source
    // this will update the sidebar, etc.
    DatabaseCommand_CollectionStats* cmd = new DatabaseCommand_CollectionStats( src );
    connect( cmd,       SIGNAL( done( const QVariantMap& ) ),
             src.data(),  SLOT( setStats( const QVariantMap& ) ), Qt::QueuedConnection );
    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
}


void
TomahawkApp::startServent()
{
    bool upnp = !arguments().contains( "--noupnp" ) && TomahawkSettings::instance()->value( "network/upnp", true ).toBool() && !TomahawkSettings::instance()->preferStaticHostPort();
    int port = TomahawkSettings::instance()->externalPort();
    if ( !Servent::instance()->startListening( QHostAddress( QHostAddress::Any ), upnp, port ) )
    {
        qDebug() << "Failed to start listening with servent";
        exit( 1 );
    }
}

void
TomahawkApp::setupSIP()
{
    qDebug() << Q_FUNC_INFO;

    //FIXME: jabber autoconnect is really more, now that there is sip -- should be renamed and/or split out of jabber-specific settings
    if( !arguments().contains( "--nosip" ) && TomahawkSettings::instance()->jabberAutoConnect() )
    {
        #ifdef GLOOX_FOUND
        m_xmppBot = new XMPPBot( this );
        #endif

        qDebug() << "Connecting SIP classes";
        m_sipHandler->connectPlugins( true );
//        m_sipHandler->setProxy( *TomahawkUtils::proxy() );
    }
}


void
TomahawkApp::activate()
{
#ifndef TOMAHAWK_HEADLESS
    mainWindow()->show();
#endif
}


bool
TomahawkApp::loadUrl( const QString& url )
{
    if( url.contains( "tomahawk://" ) ) {
        QString cmd = url.mid( 11 );
        qDebug() << "tomahawk!s" << cmd;
        if( cmd.startsWith( "load/?" ) ) {
            cmd = cmd.mid( 6 );
            qDebug() << "loading.." << cmd;
            if( cmd.startsWith( "xspf=" ) ) {
                XSPFLoader* l = new XSPFLoader( true, this );
                qDebug() << "Loading spiff:" << cmd.mid( 5 );
                l->load( QUrl( cmd.mid( 5 ) ) );
            }
        }
    } else {
        QFile f( url );
        QFileInfo info( f );
        if( f.exists() && info.suffix() == "xspf" ) {
            XSPFLoader* l = new XSPFLoader( true, this );
            qDebug() << "Loading spiff:" << url;
            l->load( QUrl::fromUserInput( url ) );
        }
    }
    return true;
}


void
TomahawkApp::instanceStarted( KDSingleApplicationGuard::Instance instance )
{
    qDebug() << "INSTANCE STARTED!" << instance.pid << instance.arguments;

    if( instance.arguments.size() < 2 )
    {
        return;
    }

    loadUrl( instance.arguments.at( 1 ) );
}

