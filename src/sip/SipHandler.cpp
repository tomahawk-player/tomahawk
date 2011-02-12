#include "SipHandler.h"
#include "sip/SipPlugin.h"

#include <QCoreApplication>
#include <QDir>
#include <QPluginLoader>
#include <QMessageBox>

#include "database/database.h"
#include "network/controlconnection.h"
#include "sourcelist.h"


SipHandler::SipHandler( QObject* parent )
    : QObject( parent )
{
    m_connected = false;
    loadPlugins( findPlugins() );
}


SipHandler::~SipHandler()
{
}


QStringList
SipHandler::findPlugins()
{
    QStringList paths;
    QList< QDir > pluginDirs;

    QDir appDir( qApp->applicationDirPath() );
    #ifdef Q_OS_MAC
    if ( appDir.dirName() == "MacOS" )
    {
        // Development convenience-hack
        appDir.cdUp();
        appDir.cdUp();
        appDir.cdUp();
    }
    #endif

    QDir libDir( appDir );
    libDir.cdUp();
    libDir.cd( "lib" );

    QDir lib64Dir( appDir );
    lib64Dir.cdUp();
    lib64Dir.cd( "lib64" );
    
    pluginDirs << appDir << libDir << lib64Dir << QDir( qApp->applicationDirPath() );
    foreach ( const QDir& pluginDir, pluginDirs )
    {
        qDebug() << "Checking directory for plugins:" << pluginDir;
        foreach ( QString fileName, pluginDir.entryList( QDir::Files ) )
        {
            if ( fileName.startsWith( "libsip_" ) )
            {
                const QString path = pluginDir.absoluteFilePath( fileName );
                if ( !paths.contains( path ) )
                    paths << path;
            }
        }
    }

    return paths;
}


void
SipHandler::loadPlugins( const QStringList& paths )
{
    foreach ( QString fileName, paths )
    {
        if ( !QLibrary::isLibrary( fileName ) )
            continue;

        qDebug() << "Trying to load plugin:" << fileName;

        QPluginLoader loader( fileName );
        QObject* plugin = loader.instance();
        if ( plugin )
        {
            // Connect via that plugin
            qDebug() << "Loaded plugin:" << loader.fileName();
            loadPlugin( plugin );
        }
        else
        {
            qDebug() << "Error loading library:" << loader.errorString();
        }
    }
}


void
SipHandler::loadPlugin( QObject* plugin )
{
    SipPlugin* sip = qobject_cast<SipPlugin*>(plugin);
    if ( sip )
    {
        QObject::connect( sip, SIGNAL( peerOnline( QString ) ), SLOT( onPeerOnline( QString ) ) );
        QObject::connect( sip, SIGNAL( peerOffline( QString ) ), SLOT( onPeerOffline( QString ) ) );
        QObject::connect( sip, SIGNAL( msgReceived( QString, QString ) ), SLOT( onMessage( QString, QString ) ) );

        QObject::connect( sip, SIGNAL( connected() ), SIGNAL( connected() ) );
        QObject::connect( sip, SIGNAL( disconnected() ), SIGNAL( disconnected() ) );
        QObject::connect( sip, SIGNAL( error( int, QString ) ), SLOT( onError( int, QString ) ) );

        m_plugins << sip;
    }
}


void
SipHandler::connectPlugins( bool startup, const QString &pluginName )
{
    foreach( SipPlugin* sip, m_plugins )
    {
        if ( pluginName.isEmpty() || ( !pluginName.isEmpty() && sip->name() == pluginName ) )
            sip->connectPlugin( startup );
    }
    m_connected = true;
}


void
SipHandler::disconnectPlugins( const QString &pluginName )
{
    foreach( SipPlugin* sip, m_plugins )
    {
        if ( pluginName.isEmpty() || ( !pluginName.isEmpty() && sip->name() == pluginName ) )
            sip->disconnectPlugin();
    }
    if( pluginName.isEmpty() )
    {
        SourceList::instance()->removeAllRemote();
        m_connected = false;
    }
}

void
SipHandler::toggleConnect()
{
    if( m_connected )
        disconnectPlugins();
    else
        connectPlugins();
}


void
SipHandler::onPeerOnline( const QString& jid )
{
//    qDebug() << Q_FUNC_INFO;
    qDebug() << "SIP online:" << jid;

    SipPlugin* sip = qobject_cast<SipPlugin*>(sender());

    QVariantMap m;
    if( Servent::instance()->visibleExternally() )
    {
        QString key = uuid();
        ControlConnection* conn = new ControlConnection( Servent::instance() );

        const QString& nodeid = Database::instance()->dbid();
        conn->setName( jid.left( jid.indexOf( "/" ) ) );
        conn->setId( nodeid );

        Servent::instance()->registerOffer( key, conn );
        m["visible"] = true;
        m["ip"] = Servent::instance()->externalAddress();
        m["port"] = Servent::instance()->externalPort();
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

    sip->sendMsg( jid, QString::fromAscii( ba ) );
}


void
SipHandler::onPeerOffline( const QString& jid )
{
//    qDebug() << Q_FUNC_INFO;
    qDebug() << "SIP offline:" << jid;
}


void
SipHandler::onMessage( const QString& from, const QString& msg )
{
    qDebug() << Q_FUNC_INFO;
    qDebug() << "SIP Message:" << from << msg;

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
        if( !Servent::instance()->visibleExternally() ||
            Servent::instance()->externalAddress() <= m.value( "ip" ).toString() )
        {
            qDebug() << "Initiate connection to" << from;
            Servent::instance()->connectToPeer( m.value( "ip" ).toString(),
                                          m.value( "port" ).toInt(),
                                          m.value( "key" ).toString(),
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
    }
}


void
SipHandler::onError( int code, const QString& msg )
{
    qWarning() << "Failed to connect to SIP:" << code << msg;

    if ( code == SipPlugin::AuthError )
    {
        emit authError();
    }
    else
    {
        SipPlugin* sip = qobject_cast<SipPlugin*>(sender());
        QTimer::singleShot( 10000, sip, SLOT( connect() ) );
    }
}
