#include "tomahawk/tomahawkapp.h"

#include <QPluginLoader>
#include <QDir>
#include <QMetaType>
#include <QTime>
#include <QNetworkReply>

#include "tomahawk/artist.h"
#include "tomahawk/album.h"
#include "tomahawk/collection.h"
#include "tomahawk/infosystem.h"
#include "database/database.h"
#include "database/databasecollection.h"
#include "database/databasecommand_collectionstats.h"
#include "database/databaseresolver.h"
#include "sip/SipHandler.h"
#include "utils/tomahawkutils.h"
#include "xmppbot/xmppbot.h"
#include "web/api_v1.h"
#include "scriptresolver.h"

#include "audioengine.h"

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

    if( !arguments().contains("--nojabber") )
    {
        setupSIP();
        m_xmppBot = new XMPPBot( this );
    }

#ifndef TOMAHAWK_HEADLESS
    if ( !m_headless )
    {
        m_mainwindow = new TomahawkWindow();
        m_mainwindow->setWindowTitle( "Tomahawk" );
        m_mainwindow->show();
        connect( m_mainwindow, SIGNAL( settingsChanged() ), SIGNAL( settingsChanged() ) );
    }
#endif

    setupPipeline();
    initLocalCollection();
    startServent();
    //loadPlugins();

    if( arguments().contains( "--http" ) || settings()->value( "network/http", true ).toBool() )
        startHTTP();

    m_sipHandler->connect();

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

    delete m_sipHandler;

#ifndef TOMAHAWK_HEADLESS
    delete m_mainwindow;
    delete m_audioEngine;
#endif

    delete m_db;
    m_db = 0;

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
    qRegisterMetaType< QTcpSocket* >("QTcpSocket*");
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
    qRegisterMetaType< QList<Tomahawk::artist_ptr> >("QList<Tomahawk::artist_ptr>");
    qRegisterMetaType< QList<Tomahawk::album_ptr> >("QList<Tomahawk::album_ptr>");
    qRegisterMetaType< QMap< QString, Tomahawk::plentry_ptr > >("QMap< QString, Tomahawk::plentry_ptr >");
    qRegisterMetaType< Tomahawk::PlaylistRevision >("Tomahawk::PlaylistRevision");
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
    m_db = new Database( dbpath, this );
    m_pipeline.databaseReady();
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
    bool upnp = !arguments().contains( "--noupnp" ) && settings()->value( "network/upnp", true ).toBool();
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
TomahawkApp::setupSIP()
{
    qDebug() << Q_FUNC_INFO;

    m_sipHandler = new SipHandler( this );

//    m_sipHandler->setProxy( m_proxy );
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
