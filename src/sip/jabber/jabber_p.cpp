#include "jabber_p.h"

#include <QDebug>
#include <QTime>
#include <QTimer>
#include <QString>
#include <QRegExp>
#include <QThread>
#include <utils/tomahawkutils.h>

using namespace gloox;
using namespace std;


Jabber_p::Jabber_p( const QString& jid, const QString& password, const QString& server, const int port )
    : QObject()
    , m_server()
{
    qDebug() << Q_FUNC_INFO;
    qsrand( QTime( 0, 0, 0 ).secsTo( QTime::currentTime() ) );

    m_presences[Presence::Available] = "available";
    m_presences[Presence::Chat] = "chat";
    m_presences[Presence::Away] = "away";
    m_presences[Presence::DND] = "dnd";
    m_presences[Presence::XA] = "xa";
    m_presences[Presence::Unavailable] = "unavailable";
    m_presences[Presence::Probe] = "probe";
    m_presences[Presence::Error] = "error";
    m_presences[Presence::Invalid] = "invalid";

    m_jid = JID( jid.toStdString() );

    if( m_jid.resource().find( "tomahawk" ) == std::string::npos )
    {
        qDebug() << "!!! Setting your resource to 'tomahawk' prior to logging in to jabber";
        m_jid.setResource( QString( "tomahawk%1" ).arg( qrand() ).toStdString() );
    }

    qDebug() << "Our JID set to:" << m_jid.full().c_str();

    // the google hack, because they filter disco features they don't know.
    if( m_jid.server().find( "googlemail." ) != string::npos
        || m_jid.server().find( "gmail." ) != string::npos
        || m_jid.server().find( "gtalk." ) != string::npos )
    {
        if( m_jid.resource().find( "tomahawk" ) == string::npos )
        {
            qDebug() << "Forcing your /resource to contain 'tomahawk' (the google workaround)";
            m_jid.setResource( "tomahawk-tomahawk" );
        }
    }

    m_client = QSharedPointer<gloox::Client>( new gloox::Client( m_jid, password.toStdString(), port ) );
    m_server = server;
}


Jabber_p::~Jabber_p()
{
   // qDebug() << Q_FUNC_INFO;
    if ( !m_client.isNull() )
    {
       // m_client->disco()->removeDiscoHandler( this );
        m_client->rosterManager()->removeRosterListener();
        m_client->removeConnectionListener( this );
    }
}

void
Jabber_p::resolveHostSRV()
{
    if( m_server.isEmpty() )
    {
        qDebug() << "No server found!";
        return;
    }
    TomahawkUtils::DNSResolver *resolver = TomahawkUtils::dnsResolver();
    connect( resolver, SIGNAL(result(QString &)), SLOT(resolveResult(QString &)) );
    qDebug() << "Resolving SRV record of " << m_server;
    resolver->resolve( m_server, "SRV" );
}

void
Jabber_p::setProxy( QNetworkProxy* proxy )
{
    qDebug() << Q_FUNC_INFO;

    if( !m_client.isNull() || !proxy )
    {
        qDebug() << "No client or no proxy";
        return;
    }

    QNetworkProxy appProx = QNetworkProxy::applicationProxy();
    QNetworkProxy* p = proxy->type() == QNetworkProxy::DefaultProxy ? &appProx : proxy;

    if( p->type() == QNetworkProxy::NoProxy )
    {
        qDebug() << "Setting proxy to none";
        m_client->setConnectionImpl( new gloox::ConnectionTCPClient( m_client.data(), m_client->logInstance(), m_client->server(), m_client->port() ) );
    }
    else if( proxy->type() == QNetworkProxy::Socks5Proxy )
    {
        qDebug() << "Setting proxy to SOCKS5";
        m_client->setConnectionImpl( new gloox::ConnectionSOCKS5Proxy( m_client.data(),
                                     new gloox::ConnectionTCPClient( m_client->logInstance(), proxy->hostName().toStdString(), proxy->port() ),
                                     m_client->logInstance(), m_client->server(), m_client->port() ) );
    }
    else
    {
        qDebug() << "Proxy type unknown";
    }
}

void
Jabber_p::resolveResult( QString& result )
{
    if ( result != "NONE" )
        m_server = result;
    qDebug() << "Final host name for XMPP server set to " << m_server;
    QMetaObject::invokeMethod( this, "go", Qt::QueuedConnection );
}

void
Jabber_p::go()
{
    if( !m_server.isEmpty() )
        m_client->setServer( m_server.toStdString() );
    else
    {
        qDebug() << "No server found!";
        return;
    }
    m_client->registerConnectionListener( this );
    m_client->rosterManager()->registerRosterListener( this );    
    m_client->logInstance().registerLogHandler( LogLevelWarning, LogAreaAll, this );
    m_client->registerMessageHandler( this );

    /*
    m_client->disco()->registerDiscoHandler( this );
    m_client->disco()->setVersion( "gloox_tomahawk", GLOOX_VERSION, "xplatform" );
    m_client->disco()->setIdentity( "client", "bot" );
    m_client->disco()->addFeature( "tomahawk:player" );
    */
    
    m_client->setPresence( Presence::Available, 1, "Tomahawk available" );

    // m_client->connect();
    // return;

    // Handle proxy
    
    qDebug() << "Connecting to the XMPP server...";
    //FIXME: This call blocks and locks up the whole GUI if the network is down
    if( m_client->connect( false ) )
    {
        qDebug() << "Connected to the XMPP server";
        emit connected();
        QTimer::singleShot( 0, this, SLOT( doJabberRecv() ) );
    }
    else
        qDebug() << "Could not connect to the XMPP server!";
}


void
Jabber_p::doJabberRecv()
{
    if ( m_client.isNull() )
        return;

    ConnectionError ce = m_client->recv( 100 );
    if ( ce != ConnNoError )
    {
        qDebug() << "Jabber_p::Recv failed, disconnected";
    }
    else
    {
        QTimer::singleShot( 100, this, SLOT( doJabberRecv() ) );
    }
}


void
Jabber_p::disconnect()
{
    if ( !m_client.isNull() )
    {
        m_client->disconnect();
    }
}


void
Jabber_p::sendMsg( const QString& to, const QString& msg )
{
    if ( QThread::currentThread() != thread() )
    {
        qDebug() << Q_FUNC_INFO << "invoking in correct thread, not"
                 << QThread::currentThread();

        QMetaObject::invokeMethod( this, "sendMsg",
                                   Qt::QueuedConnection,
                                   Q_ARG( const QString, to ),
                                   Q_ARG( const QString, msg )
                                 );
        return;
    }

    if ( m_client.isNull() )
        return;

    qDebug() << Q_FUNC_INFO << to << msg;
    Message m( Message::Chat, JID(to.toStdString()), msg.toStdString(), "" );

    m_client->send( m ); // assuming this is threadsafe
}


void
Jabber_p::broadcastMsg( const QString &msg )
{
    if ( QThread::currentThread() != thread() )
    {
        QMetaObject::invokeMethod( this, "broadcastMsg",
                                   Qt::QueuedConnection,
                                   Q_ARG(const QString, msg)
                                 );
        return;
    }

    if ( m_client.isNull() )
        return;

    std::string msg_s = msg.toStdString();
    foreach( const QString& jidstr, m_peers.keys() )
    {
        Message m(Message::Chat, JID(jidstr.toStdString()), msg_s, "");
        m_client->send( m );
    }
}


void
Jabber_p::addContact( const QString& jid, const QString& msg )
{
    if ( QThread::currentThread() != thread() )
    {
        QMetaObject::invokeMethod( this, "addContact",
                                   Qt::QueuedConnection,
                                   Q_ARG(const QString, jid),
                                   Q_ARG(const QString, msg)
                                 );
        return;
    }

    handleSubscription(JID(jid.toStdString()), msg.toStdString());
    return;
}

/// GLOOX IMPL STUFF FOLLOWS

void
Jabber_p::onConnect()
{
    // update jid resource, servers like gtalk use resource binding and may
    // have changed our requested /resource
    if ( m_client->resource() != m_jid.resource() )
    {
        m_jid.setResource( m_client->resource() );
        QString jidstr( m_jid.full().c_str() );
        emit jidChanged( jidstr );
    }

    qDebug() << "Connected as:" << m_jid.full().c_str();
}


void
Jabber_p::onDisconnect( ConnectionError e )
{
    qDebug() << "Jabber Disconnected";
    QString error;
    bool triggeredDisconnect = false;

    switch( e )
    {
    case AuthErrorUndefined:
        error = " No error occurred, or error condition is unknown";
        break;

    case SaslAborted:
        error = "The receiving entity acknowledges an &lt;abort/&gt; element sent "
                "by the initiating entity; sent in reply to the &lt;abort/&gt; element.";
        break;

    case SaslIncorrectEncoding:
        error = "The data provided by the initiating entity could not be processed "
                "because the [BASE64] encoding is incorrect (e.g., because the encoding "
                "does not adhere to the definition in Section 3 of [BASE64]); sent in "
                "reply to a &lt;response/&gt; element or an &lt;auth/&gt; element with "
                "initial response data.";
        break;

    case SaslInvalidAuthzid:
        error = "The authzid provided by the initiating entity is invalid, either "
                "because it is incorrectly formatted or because the initiating entity "
                "does not have permissions to authorize that ID; sent in reply to a "
                "&lt;response/&gt; element or an &lt;auth/&gt; element with initial "
                "response data.";
        break;

    case SaslInvalidMechanism:
        error = "The initiating entity did not provide a mechanism or requested a "
                "mechanism that is not supported by the receiving entity; sent in reply "
                "to an &lt;auth/&gt; element.";
        break;

    case SaslMalformedRequest:
        error = "The request is malformed (e.g., the &lt;auth/&gt; element includes "
                "an initial response but the mechanism does not allow that); sent in "
                "reply to an &lt;abort/&gt;, &lt;auth/&gt;, &lt;challenge/&gt;, or "
                "&lt;response/&gt; element.";
        break;

    case SaslMechanismTooWeak:
        error = "The mechanism requested by the initiating entity is weaker than "
                "server policy permits for that initiating entity; sent in reply to a "
                "&lt;response/&gt; element or an &lt;auth/&gt; element with initial "
                "response data.";
        break;

    case SaslNotAuthorized:
        error = "The authentication failed because the initiating entity did not "
                "provide valid credentials (this includes but is not limited to the "
                "case of an unknown username); sent in reply to a &lt;response/&gt; "
                "element or an &lt;auth/&gt; element with initial response data. ";
        break;

    case SaslTemporaryAuthFailure:
        error = "The authentication failed because of a temporary error condition "
                "within the receiving entity; sent in reply to an &lt;auth/&gt; element "
                "or &lt;response/&gt; element.";
        break;

    case NonSaslConflict:
        error = "XEP-0078: Resource Conflict";
        break;

    case NonSaslNotAcceptable:
        error = "XEP-0078: Required Information Not Provided";
        break;

    case NonSaslNotAuthorized:
        error = "XEP-0078: Incorrect Credentials";
        break;

    case ConnAuthenticationFailed:
        error = "Authentication failed";
        break;

    case ConnNoSupportedAuth:
        error = "No supported auth mechanism";
        break;

    default :
        error = "UNKNOWN ERROR";
        triggeredDisconnect = true;
    }

    qDebug() << "Connection error msg:" << error;

    // Assume that an unknown error is due to a disconnect triggered by the user
    if( !triggeredDisconnect )
        emit authError( e, error ); // trigger reconnect
    emit disconnected();


}


bool
Jabber_p::onTLSConnect( const CertInfo& info )
{
    qDebug()    << Q_FUNC_INFO
                << "Status" << info.status
                << "issuer" << info.issuer.c_str()
                << "peer"   << info.server.c_str()
                << "proto"  << info.protocol.c_str()
                << "mac"    << info.mac.c_str()
                << "cipher" << info.cipher.c_str()
                << "compression" << info.compression.c_str()
                << "from"   << ctime( (const time_t*)&info.date_from )
                << "to"     << ctime( (const time_t*)&info.date_to )
                ;

    //onConnect();
    return true;
}


void
Jabber_p::handleMessage( const Message& m, MessageSession * /*session*/ )
{
    QString from = QString::fromStdString( m.from().full() );
    QString msg = QString::fromStdString( m.body() );

    if ( !msg.length() )
        return;

    qDebug() << "Jabber_p::handleMessage" << from << msg;

    //printf( "from: %s, type: %d, subject: %s, message: %s, thread id: %s\n",
    //        msg.from().full().c_str(), msg.subtype(),
    //        msg.subject().c_str(), msg.body().c_str(), msg.thread().c_str() );

    //sendMsg( from, QString("You said %1").arg(msg) );

    emit msgReceived( from, msg );
}


void
Jabber_p::handleLog( LogLevel level, LogArea area, const std::string& message )
{
    qDebug() << Q_FUNC_INFO
             << "level:" << level
             << "area:" << area
             << "msg:" << message.c_str();
}


/// ROSTER STUFF
// {{{
void
Jabber_p::onResourceBindError( ResourceBindError error )
{
    qDebug() << Q_FUNC_INFO;
}


void
Jabber_p::onSessionCreateError( SessionCreateError error )
{
    qDebug() << Q_FUNC_INFO;
}


void
Jabber_p::handleItemSubscribed( const JID& jid )
{
    qDebug() << Q_FUNC_INFO << jid.full().c_str();
}


void
Jabber_p::handleItemAdded( const JID& jid )
{
    qDebug() << Q_FUNC_INFO << jid.full().c_str();
}


void
Jabber_p::handleItemUnsubscribed( const JID& jid )
{
    qDebug() << Q_FUNC_INFO << jid.full().c_str();
}


void
Jabber_p::handleItemRemoved( const JID& jid )
{
    qDebug() << Q_FUNC_INFO << jid.full().c_str();
}


void
Jabber_p::handleItemUpdated( const JID& jid )
{
    qDebug() << Q_FUNC_INFO << jid.full().c_str();
}
// }}}


void
Jabber_p::handleRoster( const Roster& roster )
{
//    qDebug() << Q_FUNC_INFO;

    Roster::const_iterator it = roster.begin();
    for ( ; it != roster.end(); ++it )
    {
        if ( (*it).second->subscription() != S10nBoth ) continue;
        qDebug() << (*it).second->jid().c_str() << (*it).second->name().c_str();
        //printf("JID: %s\n", (*it).second->jid().c_str());
    }

    // mark ourselves as "extended away" lowest priority:
    // there is no "invisible" in the spec. XA is the lowest?
    //m_client->setPresence( Presence::Available, 1, "Tomahawk App, not human" );
}


void
Jabber_p::handleRosterError( const IQ& /*iq*/ )
{
    qDebug() << Q_FUNC_INFO;
}


void
Jabber_p::handleRosterPresence( const RosterItem& item, const std::string& resource,
        Presence::PresenceType presence, const std::string& /*msg*/ )
{        
    JID jid( item.jid() );
    jid.setResource( resource );
    QString fulljid( jid.full().c_str() );

    qDebug() << "* handleRosterPresence" << fulljid << presence;

    if( jid == m_jid )
        return;

    // ignore anyone not running tomahawk:
    // convert to QString to get proper regex support
    QString res( jid.resource().c_str() );
    QRegExp regex( "tomahawk\\d+" );
    if( res != "tomahawk-tomahawk" && !res.contains( regex ) )
    {
        //qDebug() << "not considering resource of" << res;
        // Disco them to check if they are tomahawk-capable

        //qDebug() << "No tomahawk resource, DISCOing..." << jid.full().c_str();
        //m_client->disco()->getDiscoInfo( jid, "", this, 0 );
        return;
    }

    //qDebug() << "handling presence for resource of" << res;

    //qDebug() << Q_FUNC_INFO << "jid:" << QString::fromStdString(item.jid())
    //        << " resource:" << QString::fromStdString(resource)
    //        << " presencetype" << presence;

    // "going offline" event
    if ( !presenceMeansOnline( presence ) &&
         ( !m_peers.contains( fulljid ) ||
           presenceMeansOnline( m_peers.value( fulljid ) )
         )
       )
    {
        m_peers[ fulljid ] = presence;
        qDebug() << "* Peer goes offline:" << fulljid;
        emit peerOffline( fulljid );
        return;
    }

    // "coming online" event
    if( presenceMeansOnline( presence ) &&
        ( !m_peers.contains( fulljid ) ||
          !presenceMeansOnline( m_peers.value( fulljid ) )
        )
       )
    {
        m_peers[ fulljid ] = presence;
        qDebug() << "* Peer goes online:" << fulljid;
        emit peerOnline( fulljid );
        return;
    }

    //qDebug() << "Updating presence data for" << fulljid;
    m_peers[ fulljid ] = presence;
}


void
Jabber_p::handleSelfPresence( const RosterItem& item, const std::string& resource,
                            Presence::PresenceType presence, const std::string& msg )
{
    handleRosterPresence( item, resource, presence, msg );
}


bool
Jabber_p::handleSubscription( const JID& jid, const std::string& /*msg*/ )
{
    qDebug() << Q_FUNC_INFO << jid.bare().c_str();

    StringList groups;
    m_client->rosterManager()->subscribe( jid, "", groups, "" );
    return true;
}


bool
Jabber_p::handleSubscriptionRequest( const JID& jid, const std::string& /*msg*/ )
{
    qDebug() << Q_FUNC_INFO << jid.bare().c_str();
    StringList groups;
    m_client->rosterManager()->subscribe( jid, "", groups, "" );
    return true;
}


bool
Jabber_p::handleUnsubscriptionRequest( const JID& jid, const std::string& /*msg*/ )
{
    qDebug() << Q_FUNC_INFO << jid.bare().c_str();
    return true;
}


void
Jabber_p::handleNonrosterPresence( const Presence& presence )
{
    qDebug() << Q_FUNC_INFO << presence.from().full().c_str();
}
/// END ROSTER STUFF


void
Jabber_p::handleVCard( const JID& jid, const VCard* vcard )
{
    qDebug() << "VCARD RECEIVED!" << jid.bare().c_str();
}


void
Jabber_p::handleVCardResult( VCardContext context, const JID& jid, StanzaError se )
{
    qDebug() << "VCARD RESULT RECEIVED!" << jid.bare().c_str();
}


/// DISCO STUFF
void
Jabber_p::handleDiscoInfo( const JID& from, const Disco::Info& info, int context)
{
    QString jidstr( from.full().c_str() );
    //qDebug() << "DISCOinfo" << jidstr;
    if ( info.hasFeature("tomahawk:player") )
    {
        qDebug() << "Peer online and DISCOed ok:" << jidstr;
        m_peers.insert( jidstr, Presence::XA );
        emit peerOnline( jidstr );
    }
    else
    {
        //qDebug() << "Peer DISCO has no tomahawk:" << jidstr;
    }
}


void
Jabber_p::handleDiscoItems( const JID& /*iq*/, const Disco::Items&, int /*context*/ )
{
    qDebug() << Q_FUNC_INFO;
}


void
Jabber_p::handleDiscoError( const JID& j, const Error* e, int /*context*/ )
{
    qDebug() << Q_FUNC_INFO << j.full().c_str() << e->text().c_str() << e->type();
}
/// END DISCO STUFF


bool
Jabber_p::presenceMeansOnline( Presence::PresenceType p )
{
    switch(p)
    {
        case Presence::Invalid:
        case Presence::Unavailable:
        case Presence::Error:
            return false;
            break;
        default:
            return true;
    }
}
