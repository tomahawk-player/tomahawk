#include "tomahawk/tomahawkapp.h"

#include <QPluginLoader>
#include <QDir>
#include <QMetaType>
#include <QTime>
#include <QNetworkReply>

#include "artist.h"
#include "album.h"
#include "collection.h"
#include "tomahawk/infosystem.h"
#include "database/database.h"
#include "database/databasecollection.h"
#include "database/databasecommand_collectionstats.h"
#include "database/databaseresolver.h"
#include "sip/SipHandler.h"
#include "dynamic/GeneratorFactory.h"
#include "dynamic/echonest/EchonestGenerator.h"
#include "utils/tomahawkutils.h"
#include "xmppbot/xmppbot.h"
#include "web/api_v1.h"
#include "scriptresolver.h"
#include "sourcelist.h"

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
    , m_nam( 0 )
    , m_proxy( 0 )
    , m_infoSystem( 0 )
{
    qsrand( QTime( 0, 0, 0 ).secsTo( QTime::currentTime() ) );

    new Pipeline( this );
    new SourceList( this );
    m_servent = new Servent( this );

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

    new TomahawkSettings( this );
    m_audioEngine = new AudioEngine;
    setupDatabase();
    
    GeneratorFactory::registerFactory( "echonest", new EchonestFactory );
    
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
    if( TomahawkSettings::instance()->proxyType() != QNetworkProxy::NoProxy && !TomahawkSettings::instance()->proxyHost().isEmpty() )
    {
        qDebug() << "Setting proxy to saved values";
        m_proxy = new QNetworkProxy( static_cast<QNetworkProxy::ProxyType>(TomahawkSettings::instance()->proxyType()), TomahawkSettings::instance()->proxyHost(), TomahawkSettings::instance()->proxyPort(), TomahawkSettings::instance()->proxyUsername(), TomahawkSettings::instance()->proxyPassword() );
        qDebug() << "Proxy type = " << QString::number( static_cast<int>(m_proxy->type()) );
        qDebug() << "Proxy host = " << m_proxy->hostName();
        QNetworkAccessManager* nam = TomahawkApp::instance()->nam();
        nam->setProxy( *m_proxy );
    }
    else
        m_proxy = new QNetworkProxy( QNetworkProxy::NoProxy );

    QNetworkProxy::setApplicationProxy( *m_proxy );

    m_infoSystem = new Tomahawk::InfoSystem::InfoSystem( this );

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

    if( arguments().contains( "--http" ) || TomahawkSettings::instance()->value( "network/http", true ).toBool() )
        startHTTP();

    m_sipHandler->connect();

#ifndef TOMAHAWK_HEADLESS
    if ( !TomahawkSettings::instance()->hasScannerPath() )
    {
        m_mainwindow->showSettingsDialog();
    }
#endif
}


TomahawkApp::~TomahawkApp()
{
    qDebug() << Q_FUNC_INFO;

    delete m_sipHandler;
    delete m_servent;

#ifndef TOMAHAWK_HEADLESS
    delete m_mainwindow;
    delete m_audioEngine;
#endif
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
    
    qRegisterMetaType< GeneratorMode>("GeneratorMode");
    qRegisterMetaType<Tomahawk::GeneratorMode>("Tomahawk::GeneratorMode");
    // Extra definition for namespaced-versions of signals/slots required
    qRegisterMetaType< Tomahawk::collection_ptr >("Tomahawk::collection_ptr");
    qRegisterMetaType< Tomahawk::result_ptr >("Tomahawk::result_ptr");
    qRegisterMetaType< Tomahawk::query_ptr >("Tomahawk::query_ptr");
    qRegisterMetaType< Tomahawk::source_ptr >("Tomahawk::source_ptr");
    qRegisterMetaType< Tomahawk::dyncontrol_ptr >("Tomahawk::dyncontrol_ptr");
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
    new Database( dbpath, this );
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
    Pipeline::instance()->addResolver( new DatabaseResolver( true,  100 ) );
    Pipeline::instance()->addResolver( new DatabaseResolver( false, 90 ) );

//    new ScriptResolver("/home/rj/src/tomahawk-core/contrib/magnatune/magnatune-resolver.php");
}


void
TomahawkApp::initLocalCollection()
{
    source_ptr src( new Source( "My Collection" ) );
    collection_ptr coll( new DatabaseCollection( src ) );

    src->addCollection( coll );
    SourceList::instance()->add( src );

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
    bool upnp = !arguments().contains( "--noupnp" ) && TomahawkSettings::instance()->value( "network/upnp", true ).toBool();
    if ( !Servent::instance()->startListening( QHostAddress( QHostAddress::Any ), upnp ) )
    {
        qDebug() << "Failed to start listening with servent";
        exit( 1 );
    }

    //QString key = m_servent.createConnectionKey();
    //qDebug() << "Generated an offer key: " << key;
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

            PluginAPI* api = new PluginAPI( Pipeline::instance() );
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

