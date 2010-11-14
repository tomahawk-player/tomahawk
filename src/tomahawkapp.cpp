#include "tomahawk/tomahawkapp.h"

#include <QPluginLoader>
#include <QDir>
#include <QMetaType>
#include <QTime>
#include <QNetworkReply>

#include "tomahawk/collection.h"
#include "tomahawk/infosystem.h"
#include "database/database.h"
#include "database/databasecollection.h"
#include "database/databasecommand_collectionstats.h"
#include "database/databaseresolver.h"
#include "jabber/jabber.h"
#include "utils/tomahawkutils.h"
#include "xmppbot/xmppbot.h"
#include "web/api_v1.h"
#include "scriptresolver.h"

#include "audioengine.h"
#include "controlconnection.h"
#include "tomahawkzeroconf.h"

#ifndef TOMAHAWK_HEADLESS
    #include "tomahawkwindow.h"
    #include "settingsdialog.h"
    #include <QMessageBox>
#endif

#include <iostream>
#include <fstream>

#define LOGFILE TomahawkUtils::appDataDir().filePath( "tomahawk.log" ).toLocal8Bit()
#define LOGFILE_SIZE 1024 * 512
#include "tomahawksettings.h"

using namespace std;
ofstream logfile;

void TomahawkLogHandler( QtMsgType type, const char *msg )
{
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
    , m_audioEngine( 0 )
    , m_zeroconf( 0 )
    , m_settings( 0 )
    , m_nam( 0 )
    , m_proxy( 0 )
    , m_infoSystem( 0 )
{
    qsrand( QTime( 0, 0, 0 ).secsTo( QTime::currentTime() ) );

#ifdef TOMAHAWK_HEADLESS
    m_headless = true;
#else
    m_mainwindow = 0;
    m_headless = arguments().contains( "--headless" );
    setWindowIcon( QIcon( RESPATH "icons/tomahawk-icon-128x128.png" ) );
#endif

#ifndef NO_LIBLASTFM
    m_scrobbler = 0;
#endif

    qDebug() << "TomahawkApp thread:" << this->thread();
    setOrganizationName( "Tomahawk" );
    setOrganizationDomain( "tomahawk.org" );
    setApplicationName( "Player" );
    setApplicationVersion( "1.0" ); // FIXME: last.fm "tst" auth requires 1.0 version according to docs, will change when we get our own identifier
    registerMetaTypes();
    setupLogfile();

    m_settings = new TomahawkSettings( this );
    m_audioEngine = new AudioEngine;
    setupDatabase();

#ifndef NO_LIBLASTFM
        m_scrobbler = new Scrobbler( this );
        m_nam = new lastfm::NetworkAccessManager( this );

        connect( m_audioEngine, SIGNAL( started( const Tomahawk::result_ptr& ) ),
                 m_scrobbler,     SLOT( trackStarted( const Tomahawk::result_ptr& ) ), Qt::QueuedConnection );

        connect( m_audioEngine, SIGNAL( paused() ),
                 m_scrobbler,     SLOT( trackPaused() ), Qt::QueuedConnection );

        connect( m_audioEngine, SIGNAL( resumed() ),
                 m_scrobbler,     SLOT( trackResumed() ), Qt::QueuedConnection );

        connect( m_audioEngine, SIGNAL( stopped() ),
                 m_scrobbler,     SLOT( trackStopped() ), Qt::QueuedConnection );
#else
        m_nam = new QNetworkAccessManager;
#endif

#ifndef TOMAHAWK_HEADLESS
    if ( !m_headless )
    {

        m_mainwindow = new TomahawkWindow();
        m_mainwindow->show();
        connect( m_mainwindow, SIGNAL( settingsChanged() ), SIGNAL( settingsChanged() ) );
    }
#endif

    // Set up proxy
    if( m_settings->proxyType() != QNetworkProxy::NoProxy && !m_settings->proxyHost().isEmpty() )
    {
        qDebug() << "Setting proxy to saved values";
        m_proxy = new QNetworkProxy( static_cast<QNetworkProxy::ProxyType>(m_settings->proxyType()), m_settings->proxyHost(), m_settings->proxyPort(), m_settings->proxyUsername(), m_settings->proxyPassword() );
        qDebug() << "Proxy type = " << QString::number( static_cast<int>(m_proxy->type()) );
        qDebug() << "Proxy host = " << m_proxy->hostName();
        QNetworkAccessManager* nam = TomahawkApp::instance()->nam();
        nam->setProxy( *m_proxy );
    }
    else
        m_proxy = new QNetworkProxy( QNetworkProxy::NoProxy );

    QNetworkProxy::setApplicationProxy( *m_proxy );

    m_infoSystem = new Tomahawk::InfoSystem::InfoSystem( this );

    boost::function<QSharedPointer<QIODevice>(result_ptr)> fac =
        boost::bind( &TomahawkApp::httpIODeviceFactory, this, _1 );
    this->registerIODeviceFactory( "http", fac );

    setupPipeline();
    initLocalCollection();
    startServent();
    //loadPlugins();

    if( arguments().contains( "--http" ) || settings()->value( "network/http", true ).toBool() )
        startHTTP();

    if( !arguments().contains("--nojabber") ) setupJabber();
    m_xmppBot = new XMPPBot( this );

    if ( !arguments().contains( "--nozeroconf" ) )
    {
        // advertise our servent on the LAN
        m_zeroconf = new TomahawkZeroconf( m_servent.port(), this );
        connect( m_zeroconf, SIGNAL( tomahawkHostFound( const QString&, int, const QString&, const QString& ) ),
                               SLOT( lanHostFound( const QString&, int, const QString&, const QString& ) ) );
        m_zeroconf->advertise();
    }

    #ifndef TOMAHAWK_HEADLESS
    if ( !m_settings->hasScannerPath() )
    {
        m_mainwindow->showSettingsDialog();
    }
    #endif
}


TomahawkApp::~TomahawkApp()
{
    qDebug() << Q_FUNC_INFO;

    if ( !m_jabber.isNull() )
    {
        m_jabber.clear();
    }

#ifndef TOMAHAWK_HEADLESS
    delete m_mainwindow;
    delete m_audioEngine;
#endif

    delete m_zeroconf;
    delete m_db;

    // always last thing, incase other objects save state on exit:
    delete m_settings;
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


PlaylistManager*
TomahawkApp::playlistManager()
{
    return m_mainwindow->playlistManager();
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
    qRegisterMetaType< QSharedPointer<QIODevice> >("QSharedPointer<QIODevice>");
    qRegisterMetaType< QFileInfo >("QFileInfo");
    qRegisterMetaType< QMap<QString, unsigned int> >("QMap<QString, unsigned int>");
    qRegisterMetaType< QMap< QString, plentry_ptr > >("QMap< QString, plentry_ptr >");
    qRegisterMetaType< QHash< QString, QMap<quint32, quint16> > >("QHash< QString, QMap<quint32, quint16> >");

    // Extra definition for namespaced-versions of signals/slots required
    qRegisterMetaType< Tomahawk::collection_ptr >("Tomahawk::collection_ptr");
    qRegisterMetaType< Tomahawk::result_ptr >("Tomahawk::result_ptr");
    qRegisterMetaType< Tomahawk::source_ptr >("Tomahawk::source_ptr");
    qRegisterMetaType< QList<Tomahawk::playlist_ptr> >("QList<Tomahawk::playlist_ptr>");
    qRegisterMetaType< QList<Tomahawk::plentry_ptr> >("QList<Tomahawk::plentry_ptr>");
    qRegisterMetaType< QList<Tomahawk::query_ptr> >("QList<Tomahawk::query_ptr>");
    qRegisterMetaType< QList<Tomahawk::result_ptr> >("QList<Tomahawk::result_ptr>");
    qRegisterMetaType< QMap< QString, Tomahawk::plentry_ptr > >("QMap< QString, Tomahawk::plentry_ptr >");
    qRegisterMetaType< Tomahawk::PlaylistRevision >("Tomahawk::PlaylistRevision");
    qRegisterMetaType< Tomahawk::QID >("Tomahawk::QID");
    qRegisterMetaType< QTcpSocket* >("QTcpSocket*");

    #ifndef TOMAHAWK_HEADLESS
    qRegisterMetaType< AudioErrorCode >("AudioErrorCode");
    #endif
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
    m_db = new Database( dbpath, this );
    m_pipeline.databaseReady();
}


void
TomahawkApp::lanHostFound( const QString& host, int port, const QString& name, const QString& nodeid )
{
    qDebug() << "Found LAN host:" << host << port << nodeid;

    if ( !m_servent.connectedToSession( nodeid ) )
        m_servent.connectToPeer( host, port, "whitelist", name, nodeid );
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
    m_pipeline.addResolver( new DatabaseResolver( true,  100 ) );
    m_pipeline.addResolver( new DatabaseResolver( false, 90 ) );

//    new ScriptResolver("/home/rj/src/tomahawk-core/contrib/magnatune/magnatune-resolver.php");
}


void
TomahawkApp::initLocalCollection()
{
    source_ptr src( new Source( "My Collection" ) );
    collection_ptr coll( new DatabaseCollection( src ) );

    src->addCollection( coll );
    this->sourcelist().add( src );

    boost::function<QSharedPointer<QIODevice>(result_ptr)> fac =
        boost::bind( &TomahawkApp::localFileIODeviceFactory, this, _1 );
    this->registerIODeviceFactory( "file", fac );

    // to make the stats signal be emitted by our local source
    // this will update the sidebar, etc.
    DatabaseCommand_CollectionStats* cmd = new DatabaseCommand_CollectionStats( src );
    connect( cmd,       SIGNAL( done( const QVariantMap& ) ),
             src.data(),  SLOT( setStats( const QVariantMap& ) ), Qt::QueuedConnection );
    database()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
}


void
TomahawkApp::startServent()
{
    bool upnp = arguments().contains( "--upnp" ) || settings()->value( "network/upnp", true ).toBool();
    if ( !m_servent.startListening( QHostAddress( QHostAddress::Any ), upnp ) )
    {
        qDebug() << "Failed to start listening with servent";
        exit( 1 );
    }

    //QString key = m_servent.createConnectionKey();
    //qDebug() << "Generated an offer key: " << key;

    boost::function<QSharedPointer<QIODevice>(result_ptr)> fac =
        boost::bind( &Servent::remoteIODeviceFactory, &m_servent, _1 );

    this->registerIODeviceFactory( "servent", fac );
}


void
TomahawkApp::loadPlugins()
{
    // look in same dir as executable for plugins
    QDir dir( TomahawkApp::instance()->applicationDirPath() );
    QStringList filters;
    filters << "*.so" << "*.dll" << "*.dylib";

    QStringList files = dir.entryList( filters );
    foreach( const QString& filename, files )
    {
        qDebug() << "Attempting to load" << QString( "%1/%2" ).arg( dir.absolutePath() ).arg( filename );

        QPluginLoader loader( dir.absoluteFilePath( filename ) );
        if ( QObject* inst = loader.instance() )
        {
            TomahawkPlugin* pluginst = qobject_cast<TomahawkPlugin *>(inst);
            if ( !pluginst )
                continue;

            PluginAPI* api = new PluginAPI( this->pipeline() );
            TomahawkPlugin* plugin = pluginst->factory( api );
            qDebug() << "Loaded Plugin:" << plugin->name();
            qDebug() << plugin->description();
            m_plugins.append( plugin );

            // plugins responsibility to register itself as a resolver/collection
            // all we need to do is create an instance of it.
        }
        else
        {
            qDebug() << "PluginLoader failed to create instance:" << filename << " Err:" << loader.errorString();
        }
    }
}


void
TomahawkApp::setupJabber()
{
    qDebug() << Q_FUNC_INFO;
    if ( !m_jabber.isNull() )
        return;
    if ( !m_settings->value( "jabber/autoconnect", true ).toBool() )
        return;

    QString jid       = m_settings->value( "jabber/username"   ).toString();
    QString server    = m_settings->value( "jabber/server"     ).toString();
    QString password  = m_settings->value( "jabber/password"   ).toString();
    unsigned int port = m_settings->value( "jabber/port", 5222 ).toUInt();

    // gtalk check
    if( server.isEmpty() && ( jid.contains("@gmail.com") || jid.contains("@googlemail.com") ) )
    {
        qDebug() << "Setting jabber server to talk.google.com";
        server = "talk.google.com";
    }

    if ( port < 1 || port > 65535 || jid.isEmpty() || password.isEmpty() )
    {
        qDebug() << "Jabber credentials look wrong, not connecting";
        return;
    }

    m_jabber = QSharedPointer<Jabber>( new Jabber( jid, password, server, port ) );

    connect( m_jabber.data(), SIGNAL( peerOnline( QString ) ), SLOT( jabberPeerOnline( QString ) ) );
    connect( m_jabber.data(), SIGNAL( peerOffline( QString ) ), SLOT( jabberPeerOffline( QString ) ) );
    connect( m_jabber.data(), SIGNAL( msgReceived( QString, QString ) ), SLOT( jabberMessage( QString, QString ) ) );

    connect( m_jabber.data(), SIGNAL( connected() ), SLOT( jabberConnected() ) );
    connect( m_jabber.data(), SIGNAL( disconnected() ),  SLOT( jabberDisconnected() ) );
    connect( m_jabber.data(), SIGNAL( authError( int, QString ) ), SLOT( jabberAuthError( int, QString ) ) );

    m_jabber->setProxy( m_proxy );
    m_jabber->start();
}


void
TomahawkApp::reconnectJabber()
{
    m_jabber.clear();
    setupJabber();
}


void
TomahawkApp::jabberAuthError( int code, const QString& msg )
{
    qWarning() << "Failed to connect to jabber" << code << msg;

#ifndef TOMAHAWK_HEADLESS
    if( m_mainwindow )
    {
        m_mainwindow->setWindowTitle( QString("Tomahawk [jabber: %1, portfwd: %2]")
                                      .arg( "AUTH_ERROR" )
                                      .arg( (servent().externalPort() > 0) ? QString( "YES:%1" ).arg(servent().externalPort()) :"NO" ) );

        if ( code == gloox::ConnAuthenticationFailed )
        {
            QMessageBox::warning( m_mainwindow,
                                  "Jabber Auth Error",
                                  QString("Error connecting to Jabber (%1) %2").arg(code).arg(msg),
                                  QMessageBox::Ok );
        }
    }
#endif

    if ( code != gloox::ConnAuthenticationFailed )
        QTimer::singleShot( 10000, this, SLOT( reconnectJabber() ) );
}


void
TomahawkApp::jabberConnected()
{
    qDebug() << Q_FUNC_INFO;

#ifndef TOMAHAWK_HEADLESS
    if( m_mainwindow )
    {
        m_mainwindow->setWindowTitle( QString("Tomahawk [jabber: %1, portfwd: %2]")
                                      .arg( "CONNECTED" )
                                      .arg( (servent().externalPort() > 0) ? QString( "YES:%1" ).arg(servent().externalPort()):"NO" ) );
    }
#endif
}


void
TomahawkApp::jabberDisconnected()
{
    qDebug() << Q_FUNC_INFO;

#ifndef TOMAHAWK_HEADLESS
    if( m_mainwindow )
    {
        m_mainwindow->setWindowTitle( QString("Tomahawk [jabber: %1, portfwd: %2]")
                                      .arg( "DISCONNECTED" )
                                      .arg( (servent().externalPort() > 0) ? QString( "YES:%1" ).arg(servent().externalPort()):"NO" ) );
    }
#endif
}


void
TomahawkApp::jabberPeerOnline( const QString& jid )
{
//    qDebug() << Q_FUNC_INFO;
//    qDebug() << "Jabber Peer online:" << jid;

    QVariantMap m;
    if( m_servent.visibleExternally() )
    {
        QString key = uuid();
        ControlConnection* conn = new ControlConnection( &m_servent );

        const QString& nodeid = APP->nodeID();
        conn->setName( jid.left( jid.indexOf( "/" ) ) );
        conn->setId( nodeid );

        // FIXME strip /resource, but we should use a UID per database install
        //QString uniqname = jid.left( jid.indexOf("/") );
        //conn->setName( uniqname ); //FIXME

        // FIXME:
        //QString ouruniqname = m_settings->value( "jabber/username" ).toString()
        //                      .left( m_settings->value( "jabber/username" ).toString().indexOf("/") );

        m_servent.registerOffer( key, conn );
        m["visible"] = true;
        m["ip"] = m_servent.externalAddress().toString();
        m["port"] = m_servent.externalPort();
        m["key"] = key;
        m["uniqname"] = nodeid;

        qDebug() << "Asking them to connect to us:" << m;
    }
    else
    {
        m["visible"] = false;
        qDebug() << "We are not visible externally:" << m;
    }

    QJson::Serializer ser;
    QByteArray ba = ser.serialize( m );
    m_jabber->sendMsg( jid, QString::fromAscii( ba ) );
}


void
TomahawkApp::jabberPeerOffline( const QString& jid )
{
//    qDebug() << Q_FUNC_INFO;
//    qDebug() << "Jabber Peer offline:" << jid;
}


void
TomahawkApp::jabberMessage( const QString& from, const QString& msg )
{
    qDebug() << Q_FUNC_INFO;
    qDebug() << "Jabber Message:" << from << msg;

    QJson::Parser parser;
    bool ok;
    QVariant v = parser.parse( msg.toAscii(), &ok );
    if ( !ok  || v.type() != QVariant::Map )
    {
        qDebug() << "Invalid JSON in XMPP msg";
        return;
    }

    QVariantMap m = v.toMap();
    /*
      If only one party is externally visible, connection is obvious
      If both are, peer with lowest IP address initiates the connection.
      This avoids dupe connections.
     */
    if ( m.value( "visible" ).toBool() )
    {
        if( !m_servent.visibleExternally() ||
            m_servent.externalAddress().toString() <= m.value( "ip" ).toString() )
        {
            qDebug() << "Initiate connection to" << from;
            m_servent.connectToPeer( m.value( "ip"   ).toString(),
                                     m.value( "port" ).toInt(),
                                     m.value( "key"  ).toString(),
                                     from,
                                     m.value( "uniqname" ).toString() );
        }
        else
        {
            qDebug() << Q_FUNC_INFO << "They should be conecting to us...";
        }
    }
    else
    {
        qDebug() << Q_FUNC_INFO << "They are not visible, doing nothing atm";
        if ( m_servent.visibleExternally() )
            jabberPeerOnline( from ); // HACK FIXME
    }
}


void
TomahawkApp::registerIODeviceFactory( const QString &proto, boost::function<QSharedPointer<QIODevice>(Tomahawk::result_ptr)> fac )
{
    m_iofactories.insert( proto, fac );
    qDebug() << "Registered IODevice Factory for" << proto;
}


QSharedPointer<QIODevice>
TomahawkApp::getIODeviceForUrl( const Tomahawk::result_ptr& result )
{
    qDebug() << Q_FUNC_INFO << thread();
    QSharedPointer<QIODevice> sp;

    QRegExp rx( "^([a-zA-Z0-9]+)://(.+)$" );
    if ( rx.indexIn( result->url() ) == -1 )
        return sp;

    const QString proto = rx.cap( 1 );
    //const QString urlpart = rx.cap( 2 );
    if ( !m_iofactories.contains( proto ) )
        return sp;

    return m_iofactories.value( proto )( result );
}


QSharedPointer<QIODevice>
TomahawkApp::localFileIODeviceFactory( const Tomahawk::result_ptr& result )
{
    // ignore "file://" at front of url
    QFile * io = new QFile( result->url().mid( QString( "file://" ).length() ) );
    if ( io )
        io->open( QIODevice::ReadOnly );

    return QSharedPointer<QIODevice>( io );
}


QSharedPointer<QIODevice>
TomahawkApp::httpIODeviceFactory( const Tomahawk::result_ptr& result )
{
    qDebug() << Q_FUNC_INFO << result->url();
    QNetworkRequest req( result->url() );
    QNetworkReply* reply = APP->nam()->get( req );
    return QSharedPointer<QIODevice>( reply );
}


const QString&
TomahawkApp::nodeID() const
{
    return m_db->dbid();
}
