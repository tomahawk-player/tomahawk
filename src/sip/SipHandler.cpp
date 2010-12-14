#include "SipHandler.h"
#include "sip/SipPlugin.h"

#include <QDir>
#include <QPluginLoader>
#include <QMessageBox>

#include "tomahawk/tomahawkapp.h"
#include "controlconnection.h"


SipHandler::SipHandler( QObject* parent )
    : QObject( parent )
{
    m_connected = false;
    loadPlugins();
}


SipHandler::~SipHandler()
{
}


void
SipHandler::loadPlugins()
{
    qDebug() << TomahawkApp::instance();
    qDebug() << TomahawkApp::instance()->applicationDirPath();
    QDir pluginsDir( TomahawkApp::instance()->applicationDirPath() );

    #if defined(Q_OS_WIN)
    if (pluginsDir.dirName().toLower() == "debug" || pluginsDir.dirName().toLower() == "release")
      pluginsDir.cdUp();
    #elif defined(Q_OS_MAC)
    if (pluginsDir.dirName() == "MacOS") {
      pluginsDir.cdUp();
      pluginsDir.cdUp();
      pluginsDir.cdUp();
    }
    #endif
    pluginsDir.cd("plugins");

    foreach ( QString fileName, pluginsDir.entryList( QDir::Files ) )
    {
        QPluginLoader loader( pluginsDir.absoluteFilePath( fileName ) );
        QObject* plugin = loader.instance();
        if ( plugin )
        {
            // Connect via that plugin
            qDebug() << "Trying to load plugin:" << loader.fileName();
            loadPlugin( plugin );
        }
    }
}


void
SipHandler::loadPlugin( QObject* plugin )
{
    SipPlugin* sip = qobject_cast<SipPlugin*>(plugin);
    if ( sip )
    {
        qDebug() << "Loaded plugin!";

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
SipHandler::connect()
{
    foreach( SipPlugin* sip, m_plugins )
        sip->connect();
    m_connected = true;
}


void
SipHandler::disconnect()
{
    foreach( SipPlugin* sip, m_plugins )
        sip->disconnect();
    APP->sourcelist().removeAllRemote();
    m_connected = false;
}

void
SipHandler::toggleConnect()
{
    if( m_connected )
        disconnect();
    else
        connect();
}


void
SipHandler::onPeerOnline( const QString& jid )
{
//    qDebug() << Q_FUNC_INFO;
    qDebug() << "SIP online:" << jid;

    SipPlugin* sip = qobject_cast<SipPlugin*>(sender());

    QVariantMap m;
    if( APP->servent().visibleExternally() )
    {
        QString key = uuid();
        ControlConnection* conn = new ControlConnection( &APP->servent() );

        const QString& nodeid = APP->nodeID();
        conn->setName( jid.left( jid.indexOf( "/" ) ) );
        conn->setId( nodeid );

        // FIXME strip /resource, but we should use a UID per database install
        //QString uniqname = jid.left( jid.indexOf("/") );
        //conn->setName( uniqname ); //FIXME

        // FIXME:
        //QString ouruniqname = m_settings->value( "jabber/username" ).toString()
        //                      .left( m_settings->value( "jabber/username" ).toString().indexOf("/") );

        APP->servent().registerOffer( key, conn );
        m["visible"] = true;
        m["ip"] = APP->servent().externalAddress().toString();
        m["port"] = APP->servent().externalPort();
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
        if( !APP->servent().visibleExternally() ||
            APP->servent().externalAddress().toString() <= m.value( "ip" ).toString() )
        {
            qDebug() << "Initiate connection to" << from;
            APP->servent().connectToPeer( m.value( "ip"   ).toString(),
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
//        if ( m_servent.visibleExternally() )
//            jabberPeerOnline( from ); // HACK FIXME
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
