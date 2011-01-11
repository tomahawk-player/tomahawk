#include "servent.h"

#include <QCoreApplication>
#include <QMutexLocker>
#include <QNetworkInterface>
#include <QFile>
#include <QTime>
#include <QThread>
#include <QNetworkRequest>
#include <QNetworkReply>

#include "result.h"
#include "source.h"
#include "bufferiodevice.h"
#include "connection.h"
#include "controlconnection.h"
#include "database/database.h"
#include "filetransferconnection.h"
#include "sourcelist.h"

#include "portfwd/portfwd.h"

using namespace Tomahawk;

Servent* Servent::s_instance = 0;


Servent*
Servent::instance()
{
    return s_instance;
}


Servent::Servent( QObject* parent )
    : QTcpServer( parent )
    , m_port( 0 )
    , m_externalPort( 0 )
    , pf( new Portfwd() )
{
    s_instance = this;

    qsrand( QTime( 0, 0, 0 ).secsTo( QTime::currentTime() ) );

    {
    boost::function<QSharedPointer<QIODevice>(result_ptr)> fac =
        boost::bind( &Servent::localFileIODeviceFactory, this, _1 );
    this->registerIODeviceFactory( "file", fac );
    }

    {
    boost::function<QSharedPointer<QIODevice>(result_ptr)> fac =
        boost::bind( &Servent::remoteIODeviceFactory, this, _1 );
    this->registerIODeviceFactory( "servent", fac );
    }

    {
    boost::function<QSharedPointer<QIODevice>(result_ptr)> fac =
        boost::bind( &Servent::httpIODeviceFactory, this, _1 );
    this->registerIODeviceFactory( "http", fac );
    }
}


Servent::~Servent()
{
    if( m_externalPort )
    {
        qDebug() << "Unregistering port fwd";
        pf->remove( m_externalPort );
    }
}


bool
Servent::startListening( QHostAddress ha, bool upnp, int port )
{
    m_port = port;
    // try listening on one port higher as well, to aid debugging
    // and let you run 2 instances easily
    if( !listen( ha, m_port ) && !listen( ha, ++m_port ) )
    {
        qDebug() << "Failed to listen on port" << m_port;
        return false;
    }
    else
    {
        qDebug() << "Servent listening on port" << m_port << " servent thread:" << thread();
    }

    // TODO check if we have a public/internet IP on this machine directly
    // FIXME the portfwd stuff is blocking, so we hang here for 2 secs atm
    if( upnp )
    {
        // try and pick an available port:
        if( pf->init( 2000 ) )
        {
            int tryport = m_port;

            // last.fm office firewall policy hack
            // (corp. firewall allows outgoing connections to this port,
            //  so listen on this if you want lastfmers to connect to you)
            if( qApp->arguments().contains( "--porthack" ) )
            {
                tryport = 3389;
                pf->remove( tryport );
            }

            for( int r=0; r<5; ++r )
            {
                qDebug() << "Trying to setup portfwd on" << tryport;
                if( pf->add( tryport, m_port ) )
                {
                    QString pubip = QString( pf->external_ip().c_str() );
                    m_externalAddress = QHostAddress( pubip );
                    m_externalPort = tryport;
                    qDebug() << "External servent address detected as" << pubip << ":" << m_externalPort;
                    qDebug() << "Max upstream  " << pf->max_upstream_bps() << "bps";
                    qDebug() << "Max downstream" << pf->max_downstream_bps() << "bps";
                    break;
                }
                tryport = 10000 + 50000 * (float)qrand()/RAND_MAX;
            }
            if( !m_externalPort )
            {
                qDebug() << "Could not setup fwd for port:" << m_port;
            }
        }
        else qDebug() << "No UPNP Gateway device found?";
    }

    if( m_externalPort == 0 )
    {
        qDebug() << "No external access, LAN and outbound connections only!";
    }

    // --lanhack means to advertise your LAN IP over jabber as if it were externallyVisible
    if( qApp->arguments().contains( "--lanhack" ) )
    {
        QList<QHostAddress> ifs = QNetworkInterface::allAddresses();
        foreach( QHostAddress ha, ifs )
        {
            if( ha.toString() == "127.0.0.1" ) continue;
            if( ha.toString().contains( ":" ) ) continue; //ipv6

            m_externalAddress = ha;
            m_externalPort = m_port;
            qDebug() << "LANHACK: set external address to lan address" << ha.toString();
            break;
        }
    }

    return true;
}


QString
Servent::createConnectionKey( const QString& name )
{
    Q_ASSERT( this->thread() == QThread::currentThread() );

    QString key = uuid();
    ControlConnection * cc = new ControlConnection( this );
    cc->setName( name.isEmpty() ? QString( "KEY(%1)" ).arg( key ) : name );
    registerOffer( key, cc );
    return key;
}


void
Servent::setExternalAddress( QHostAddress ha, int port )
{
    m_externalAddress = ha;
    m_externalPort = port;
}


void
Servent::registerOffer( const QString& key, Connection* conn )
{
    m_offers[key] = QPointer<Connection>(conn);
}


void
Servent::registerControlConnection( ControlConnection* conn )
{
    qDebug() << Q_FUNC_INFO << conn->id();
    m_controlconnections.append( conn );
}


void
Servent::unregisterControlConnection( ControlConnection* conn )
{
    QList<ControlConnection*> n;
    foreach( ControlConnection* c, m_controlconnections )
        if( c!=conn )
            n.append( c );

    m_controlconnections = n;
}


ControlConnection*
Servent::lookupControlConnection( const QString& name )
{
    foreach( ControlConnection* c, m_controlconnections )
        if( c->name() == name )
            return c;

    return NULL;
}


void
Servent::incomingConnection( int sd )
{
    Q_ASSERT( this->thread() == QThread::currentThread() );
    QTcpSocketExtra* sock = new QTcpSocketExtra;
    qDebug() << Q_FUNC_INFO << "Accepting connection, sock" << sock;
    sock->moveToThread( thread() );
    sock->_disowned = false;
    sock->_outbound = false;
    if( !sock->setSocketDescriptor( sd ) )
    {
        qDebug() << "Out of system resources for new ports?";
        Q_ASSERT( false );
        return;
    }

    connect( sock, SIGNAL( readyRead() ), SLOT( readyRead() ), Qt::QueuedConnection );
    connect( sock, SIGNAL( disconnected() ), sock, SLOT( deleteLater() ), Qt::QueuedConnection );
    qDebug() << "connection accepted.";
}


void
Servent::readyRead()
{
    Q_ASSERT( this->thread() == QThread::currentThread() );
    QTcpSocketExtra* sock = (QTcpSocketExtra*)sender();
    //qDebug() << Q_FUNC_INFO  << thread() << sock;

    if( sock->_disowned )
    {
        //qDebug() << "Socket already disowned";
        return ;
    }

    if( sock->_msg.isNull() )
    {
        char msgheader[ Msg::headerSize() ];
        if( sock->bytesAvailable() < Msg::headerSize() )
            return;

        sock->read( (char*) &msgheader, Msg::headerSize() );
        sock->_msg = Msg::begin( (char*) &msgheader );
    }

    if( sock->bytesAvailable() < sock->_msg->length() )
        return;

    QByteArray ba = sock->read( sock->_msg->length() );
    sock->_msg->fill( ba );
    Q_ASSERT( sock->_msg->is( Msg::JSON ) );

    ControlConnection* cc = 0;
    bool ok;
    int pport = 0;
    QString key, conntype, nodeid, controlid;
    QVariantMap m = parser.parse( sock->_msg->payload(), &ok ).toMap();
    if( !ok )
    {
        qDebug() << "Invalid JSON on new conection, aborting";
        goto closeconnection;
    }
    conntype  = m.value( "conntype" ).toString();
    key       = m.value( "key" ).toString();
    pport     = m.value( "port" ).toInt();
    nodeid    = m.value( "nodeid" ).toString();
    controlid = m.value( "controlid" ).toString();

    qDebug() << m;

    if( !nodeid.isEmpty() && cc != 0 ) // only control connections send nodeid
    {
        foreach( ControlConnection* con, m_controlconnections )
        {
            qDebug() << con->socket() << sock;
            if( con->id() == nodeid )
            {
                qDebug() << "Duplicate control connection detected, dropping:" << nodeid << conntype;
                goto closeconnection;
            }
        }
    }

    foreach( ControlConnection* con, m_controlconnections )
    {
        qDebug() << "conid:" << con->id();

        if ( con->id() == controlid )
        {
            cc = con;
            break;
        }
    }

    // they connected to us and want something we are offering
    if( conntype == "accept-offer" || "push-offer" )
    {
        sock->_msg.clear();
        Connection* conn = claimOffer( cc, key, sock->peerAddress() );
        if( !conn )
        {
            qDebug() << "claimOffer FAILED, key:" << key;
            goto closeconnection;
        }
        qDebug() << "claimOffer OK:" << key;

        if( !nodeid.isEmpty() )
            conn->setId( nodeid );

        handoverSocket( conn, sock );
        return;
    }
    else
    {
        qDebug() << "Invalid or unhandled conntype";
    }
    
    // fallthru to cleanup:
closeconnection:
    qDebug() << "Closing incoming connection, something was wrong.";
    sock->_msg.clear();
    sock->disconnectFromHost();
}


// creates a new tcp connection to peer from conn, handled by given connector
// new_conn is responsible for sending the first msg, if needed
void
Servent::createParallelConnection( Connection* orig_conn, Connection* new_conn, const QString& key )
{
    qDebug() << "Servent::createParallelConnection, key:" << key << thread();
    // if we can connect to them directly:
    if( orig_conn->outbound() )
    {
        qDebug() << "Connecting directly";
        connectToPeer( orig_conn->socket()->peerAddress().toString(),
                       orig_conn->peerPort(),
                       key,
                       new_conn );
    }
    else // ask them to connect to us:
    {
        QString tmpkey = uuid();
        qDebug() << "Asking them to connect to us using" << tmpkey ;
        registerOffer( tmpkey, new_conn );

        QVariantMap m;
        m.insert( "conntype", "request-offer" );
        m.insert( "key", tmpkey );
        m.insert( "offer", key );
        m.insert( "port", externalPort() );
        m.insert( "controlid", Database::instance()->dbid() );

        QJson::Serializer ser;
        orig_conn->sendMsg( Msg::factory( ser.serialize(m), Msg::JSON ) );
    }
}


/// for outbound connections. DRY out the socket handover code from readyread too?
void
Servent::socketConnected()
{
    QTcpSocketExtra* sock = (QTcpSocketExtra*)sender();

    qDebug() << "Servent::SocketConnected" << thread() << "socket:" << sock;

    Connection* conn = sock->_conn;

    handoverSocket( conn, sock );
}


// transfers ownership of socket to the connection and inits the connection
void Servent::handoverSocket( Connection* conn, QTcpSocketExtra* sock )
{
    Q_ASSERT( conn );
    Q_ASSERT( sock );
    Q_ASSERT( conn->socket().isNull() );
    Q_ASSERT( sock->isValid() );

    disconnect( sock, SIGNAL( readyRead() ),    this, SLOT( readyRead() ) );
    disconnect( sock, SIGNAL( disconnected() ), sock, SLOT( deleteLater() ) );
    disconnect( sock, SIGNAL( error( QAbstractSocket::SocketError ) ),
                this, SLOT( socketError( QAbstractSocket::SocketError ) ) );


    sock->_disowned = true;
    conn->setOutbound( sock->_outbound );
    conn->setPeerPort( sock->peerPort() );

    conn->start( sock );
}


void
Servent::socketError( QAbstractSocket::SocketError e )
{
    qDebug() << Q_FUNC_INFO;
    QTcpSocketExtra* sock = (QTcpSocketExtra*)sender();
    if( !sock )
    {
        qDebug() << "SocketError, sock is null";
        return;
    }

    Connection* conn = sock->_conn;
    qDebug() << "Servent::SocketError:" << e << conn->id() << conn->name();
    if(!sock->_disowned)
    {
        // connection will delete if we already transferred ownership, otherwise:
        sock->deleteLater();
    }
    conn->markAsFailed(); // will emit failed, then finished
}


void
Servent::connectToPeer( const QString& ha, int port, const QString &key, const QString& name, const QString& id )
{
    qDebug() << Q_FUNC_INFO << ha << port << key << name << id;
    Q_ASSERT( this->thread() == QThread::currentThread() );

    ControlConnection* conn = new ControlConnection( this );
    QVariantMap m;
    m["conntype"]  = "accept-offer";
    m["key"]       = key;
    m["port"]      = externalPort();
    m["nodeid"]    = Database::instance()->dbid();

    conn->setFirstMessage( m );
    if( name.length() )
        conn->setName( name );
    if( id.length() )
        conn->setId( id );

    connectToPeer( ha, port, key, conn );
}


void
Servent::connectToPeer( const QString& ha, int port, const QString &key, Connection* conn )
{
    Q_ASSERT( port > 0 );
    Q_ASSERT( conn );
//    Q_ASSERT( this->thread() == QThread::currentThread() );

    qDebug() << "Servent::connectToPeer:" << ha << ":" << port
             << thread() << QThread::currentThread();

    if( key.length() && conn->firstMessage().isNull() )
    {
        QVariantMap m;
        m["conntype"]  = "accept-offer";
        m["key"]       = key;
        m["port"]      = externalPort();
        m["controlid"] = Database::instance()->dbid();
        conn->setFirstMessage( m );
    }

    QTcpSocketExtra* sock = new QTcpSocketExtra();
    sock->_disowned = false;
    sock->_conn = conn;
    sock->_outbound = true;
    //qDebug() << "connectToPeer, sock:" << sock->thread();

    connect( sock, SIGNAL( connected() ), SLOT( socketConnected() ), Qt::QueuedConnection );
    connect( sock, SIGNAL( error( QAbstractSocket::SocketError ) ),
                     SLOT( socketError( QAbstractSocket::SocketError ) ), Qt::QueuedConnection );

    //qDebug() << "About to connectToHost...";
    sock->connectToHost( ha, port, QTcpSocket::ReadWrite );
    sock->moveToThread( thread() );
    //qDebug() << "tried to connectToHost (waiting on a connected signal)";
}


void
Servent::reverseOfferRequest( ControlConnection* orig_conn, const QString& key, const QString& theirkey )
{
    Q_ASSERT( this->thread() == QThread::currentThread() );

    qDebug() << "Servent::reverseOfferRequest received for" << key;
    Connection* new_conn = claimOffer( orig_conn, key );
    if ( !new_conn )
    {
        qDebug() << "claimOffer failed, killing requesting connection out of spite";
        orig_conn->shutdown();
        return;
    }

    QVariantMap m;
    m["conntype"]  = "push-offer";
    m["key"]       = theirkey;
    m["port"]      = externalPort();
    m["controlid"] = Database::instance()->dbid();
    new_conn->setFirstMessage( m );
    createParallelConnection( orig_conn, new_conn, QString() );
}


// return the appropriate connection for a given offer key, or NULL if invalid
Connection*
Servent::claimOffer( ControlConnection* cc, const QString &key, const QHostAddress peer )
{
    bool noauth = qApp->arguments().contains( "--noauth" );

    // magic key for file transfers:
    if( key.startsWith( "FILE_REQUEST_KEY:" ) )
    {
        // check if the source IP matches an existing, authenticated connection
        if ( !noauth && peer != QHostAddress::Any && !isIPWhitelisted( peer ) )
        {
            bool authed = false;
            foreach( ControlConnection* cc, m_controlconnections )
            {
                if( cc->socket()->peerAddress() == peer )
                {
                    authed = true;
                    break;
                }
            }
            if( !authed )
            {
                qDebug() << "File transfer request rejected, invalid source IP";
                return NULL;
            }
        }

        QString fid = key.right( key.length() - 17 );
        FileTransferConnection* ftc = new FileTransferConnection( this, cc, fid );
        return ftc;
    }

    if( key == "whitelist" ) // LAN IP address, check source IP
    {
        if( isIPWhitelisted( peer ) )
        {
            qDebug() << "Connection is from whitelisted IP range (LAN)";
            Connection* conn = new ControlConnection( this );
            conn->setName( peer.toString() );
            return conn;
        }
        else
        {
            qDebug() << "Connection claimed to be whitelisted, but wasn't.";
            return NULL;
        }
    }

    if( m_offers.contains( key ) )
    {
        QPointer<Connection> conn = m_offers.value( key );
        if( conn.isNull() )
        {
            // This can happen if it's a filetransferconnection, but the audioengine has
            // already closed the iodevice, causing the connection to be deleted before
            // the peer connects and provides the first byte
            qDebug() << Q_FUNC_INFO << "invalid/expired offer:" << key;
            return NULL;
        }

        if( conn->onceOnly() )
        {
            m_offers.remove( key );
            return conn.data();
        }
        else
        {
            return conn->clone();
        }
    }
    else if ( noauth )
    {
        Connection* conn;
        conn = new ControlConnection( this );
        conn->setName( key );
        return conn;
    }
    else
    {
        qDebug() << "Invalid offer key:" << key;
        return NULL;
    }
}


QSharedPointer<QIODevice>
Servent::remoteIODeviceFactory( const result_ptr& result )
{
    qDebug() << Q_FUNC_INFO << thread();
    QSharedPointer<QIODevice> sp;

    QStringList parts = result->url().mid( QString( "servent://" ).length() ).split( "\t" );
    const QString sourceName = parts.at( 0 );
    const QString fileId = parts.at( 1 );
    source_ptr s = SourceList::instance()->get( sourceName );
    if ( s.isNull() )
        return sp;

    ControlConnection* cc = s->controlConnection();
    FileTransferConnection* ftc = new FileTransferConnection( this, cc, fileId, result );
    createParallelConnection( cc, ftc, QString( "FILE_REQUEST_KEY:%1" ).arg( fileId ) );
    return ftc->iodevice();
}


void
Servent::registerFileTransferConnection( FileTransferConnection* ftc )
{
    Q_ASSERT( !m_ftsessions.contains( ftc ) );
    qDebug() << "Registering FileTransfer" << m_ftsessions.length() + 1;

    QMutexLocker lock( &m_ftsession_mut );
    m_ftsessions.append( ftc );

    printCurrentTransfers();
    emit fileTransferStarted( ftc );
}


void
Servent::onFileTransferFinished( FileTransferConnection* ftc )
{
    Q_ASSERT( ftc );
    qDebug() << "FileTransfer Finished, unregistering" << ftc->id();

    QMutexLocker lock( &m_ftsession_mut );
    m_ftsessions.removeAll( ftc );

    printCurrentTransfers();
    emit fileTransferFinished( ftc );
}


// used for debug output:
void
Servent::printCurrentTransfers()
{
    int k = 1;
    qDebug() << "~~~ Active file transfer connections:" << m_ftsessions.length();
    foreach( FileTransferConnection* i, m_ftsessions )
    {
        qDebug() << k << ") " << i->id();
    }
    qDebug() << endl;
}


bool
Servent::isIPWhitelisted( QHostAddress ip )
{
    typedef QPair<QHostAddress, int> range;
    static QList<range> whitelist;
    if( whitelist.isEmpty() )
    {
        whitelist   << range( QHostAddress( "10.0.0.0" ), 8 )
                    << range( QHostAddress( "172.16.0.0" ), 12 )
                    << range( QHostAddress( "192.168.0.0" ), 16 )
                    << range( QHostAddress( "169.254.0.0" ), 16 )
                    << range( QHostAddress( "127.0.0.0" ), 24 );

//        qDebug() << "Loaded whitelist IP range:" << whitelist;
    }

    foreach( const range& r, whitelist )
        if( ip.isInSubnet( r ) )
            return true;

    return false;
}


bool
Servent::connectedToSession( const QString& session )
{
    foreach( ControlConnection* cc, m_controlconnections )
    {
        if( cc->id() == session )
            return true;
    }

    return false;
}


void
Servent::triggerDBSync()
{
    qDebug() << Q_FUNC_INFO;

    // tell peers we have new stuff they should sync
    QList<source_ptr> sources = SourceList::instance()->sources();
    foreach( const source_ptr& src, sources )
    {
        // local src doesnt have a control connection, skip it:
        if( src.isNull() || src->isLocal() )
            continue;

        src->controlConnection()->dbSyncConnection()->trigger();
    }
}


void
Servent::registerIODeviceFactory( const QString &proto, boost::function<QSharedPointer<QIODevice>(Tomahawk::result_ptr)> fac )
{
    m_iofactories.insert( proto, fac );
    qDebug() << "Registered IODevice Factory for" << proto;
}



QSharedPointer<QIODevice>
Servent::getIODeviceForUrl( const Tomahawk::result_ptr& result )
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
Servent::localFileIODeviceFactory( const Tomahawk::result_ptr& result )
{
    // ignore "file://" at front of url
    QFile * io = new QFile( result->url().mid( QString( "file://" ).length() ) );
    if ( io )
        io->open( QIODevice::ReadOnly );

    return QSharedPointer<QIODevice>( io );
}


QSharedPointer<QIODevice>
Servent::httpIODeviceFactory( const Tomahawk::result_ptr& result )
{
/*    qDebug() << Q_FUNC_INFO << result->url();
    QNetworkRequest req( result->url() );
    QNetworkReply* reply = APP->nam()->get( req );
    return QSharedPointer<QIODevice>( reply );*/
}
