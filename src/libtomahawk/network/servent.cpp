/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Tomahawk is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */

#include "servent.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QMutexLocker>
#include <QtNetwork/QNetworkInterface>
#include <QtCore/QFile>
#include <QtCore/QThread>
#include <QtNetwork/QNetworkProxy>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>

#include "result.h"
#include "Source.h"
#include "bufferiodevice.h"
#include "connection.h"
#include "controlconnection.h"
#include "database/database.h"
#include "streamconnection.h"
#include "sourcelist.h"

#include "portfwdthread.h"
#include "tomahawksettings.h"
#include "utils/tomahawkutils.h"
#include "utils/logger.h"

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
    , m_ready( false )
    , m_portfwd( 0 )
{
    s_instance = this;

    m_lanHack = qApp->arguments().contains( "--lanhack" );
    ACLRegistry::instance();
    setProxy( QNetworkProxy::NoProxy );

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
    delete ACLRegistry::instance();
    delete m_portfwd;
}


bool
Servent::startListening( QHostAddress ha, bool upnp, int port )
{
    m_port = port;
    int defPort = TomahawkSettings::instance()->defaultPort();

    // Listen on both the selected port and, if not the same, the default port -- the latter sometimes necessary for zeroconf
    // TODO: only listen on both when zeroconf sip is enabled
    // TODO: use a real zeroconf system instead of a simple UDP broadcast?
    if ( !listen( ha, m_port ) )
    {
        if ( m_port != defPort )
        {
            if ( !listen( ha, defPort ) )
            {
                tLog() << "Failed to listen on both port" << m_port << "and port" << defPort;
                tLog() << "Error string is:" << errorString();
                return false;
            }
            else
                m_port = defPort;
        }
    }

    tLog() << "Servent listening on port" << m_port << "- servent thread:" << thread()
           << "- address mode:" << (int)( TomahawkSettings::instance()->externalAddressMode() );

    // --lanhack means to advertise your LAN IP as if it were externally visible
    if ( TomahawkSettings::instance()->preferStaticHostPort() )
    {
        m_externalHostname = TomahawkSettings::instance()->externalHostname();
        m_externalPort = TomahawkSettings::instance()->externalPort();
        m_ready = true;
        emit ready();
        return true;
    }

    TomahawkSettings::ExternalAddressMode mode = TomahawkSettings::instance()->externalAddressMode();
    if ( mode == TomahawkSettings::Upnp && !upnp )
        mode = TomahawkSettings::Lan;

    switch ( mode )
    {
        case TomahawkSettings::Lan:
            setInternalAddress();
            break;

        case TomahawkSettings::Upnp:
            // TODO check if we have a public/internet IP on this machine directly
            tLog() << "External address mode set to upnp...";
            m_portfwd = new PortFwdThread( m_port );
            connect( m_portfwd, SIGNAL( externalAddressDetected( QHostAddress, unsigned int ) ),
                                  SLOT( setExternalAddress( QHostAddress, unsigned int ) ) );
            break;
    }

    return true;
}


QString
Servent::createConnectionKey( const QString& name, const QString &nodeid, const QString &key, bool onceOnly )
{
    Q_ASSERT( this->thread() == QThread::currentThread() );

    QString _key = ( key.isEmpty() ? uuid() : key );
    ControlConnection* cc = new ControlConnection( this, name );
    cc->setName( name.isEmpty() ? QString( "KEY(%1)" ).arg( key ) : name );
    if ( !nodeid.isEmpty() )
        cc->setId( nodeid );
    cc->setOnceOnly( onceOnly );

    tDebug( LOGVERBOSE ) << "Creating connection key with name of" << cc->name() << "and id of" << cc->id() << "and key of" << _key << "; key is once only? :" << (onceOnly ? "true" : "false");
    registerOffer( _key, cc );
    return _key;
}


bool
Servent::isValidExternalIP( const QHostAddress& addr ) const
{
    QString ip = addr.toString();
    if ( !m_lanHack && ( ip.startsWith( "10." ) || ip.startsWith( "172.16." ) || ip.startsWith( "192.168." ) ) )
    {
        return false;
    }

    return !addr.isNull();
}


void
Servent::setInternalAddress()
{
    foreach ( QHostAddress ha, QNetworkInterface::allAddresses() )
    {
        if ( ha.toString() == "127.0.0.1" )
            continue;
        if ( ha.toString().contains( ":" ) )
            continue; //ipv6

        if ( m_lanHack && isValidExternalIP( ha ) )
        {
            tLog() << "LANHACK: set external address to lan address" << ha.toString();
            setExternalAddress( ha, m_port );
        }
        else
        {
            m_ready = true;
            emit ready();
        }
        break;
    }
}


void
Servent::setExternalAddress( QHostAddress ha, unsigned int port )
{
    if ( isValidExternalIP( ha ) )
    {
        m_externalAddress = ha;
        m_externalPort = port;
    }

    if ( m_externalPort == 0 || !isValidExternalIP( ha ) )
    {
        if ( !TomahawkSettings::instance()->externalHostname().isEmpty() &&
             !TomahawkSettings::instance()->externalPort() == 0 )
        {
            m_externalHostname = TomahawkSettings::instance()->externalHostname();
            m_externalPort = TomahawkSettings::instance()->externalPort();
            tDebug() << "UPnP failed, have external address/port - falling back" << m_externalHostname << m_externalPort << m_externalAddress;
        }
        else
        {
            tLog() << "No external access, LAN and outbound connections only!";
            setInternalAddress();
            return;
        }
    }

    m_ready = true;
    emit ready();
}


void
Servent::registerOffer( const QString& key, Connection* conn )
{
    m_offers[key] = QWeakPointer<Connection>(conn);
}


void
Servent::registerControlConnection( ControlConnection* conn )
{
    m_controlconnections.append( conn );
}


void
Servent::unregisterControlConnection( ControlConnection* conn )
{
    QList<ControlConnection*> n;
    foreach( ControlConnection* c, m_controlconnections )
        if( c!=conn )
            n.append( c );

    m_connectedNodes.removeAll( conn->id() );
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
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Accepting connection, sock" << sock;

    sock->moveToThread( thread() );
    sock->_disowned = false;
    sock->_outbound = false;
    if( !sock->setSocketDescriptor( sd ) )
    {
        Q_ASSERT( false );
        return;
    }

    connect( sock, SIGNAL( readyRead() ), SLOT( readyRead() ) );
    connect( sock, SIGNAL( disconnected() ), sock, SLOT( deleteLater() ) );
}


void
Servent::readyRead()
{
    Q_ASSERT( this->thread() == QThread::currentThread() );
    QWeakPointer< QTcpSocketExtra > sock = (QTcpSocketExtra*)sender();

    if( sock.isNull() || sock.data()->_disowned )
    {
        return;
    }

    if( sock.data()->_msg.isNull() )
    {
        char msgheader[ Msg::headerSize() ];
        if( sock.data()->bytesAvailable() < Msg::headerSize() )
            return;

        sock.data()->read( (char*) &msgheader, Msg::headerSize() );
        sock.data()->_msg = Msg::begin( (char*) &msgheader );
    }

    if( sock.data()->bytesAvailable() < sock.data()->_msg->length() )
        return;

    QByteArray ba = sock.data()->read( sock.data()->_msg->length() );
    sock.data()->_msg->fill( ba );
    Q_ASSERT( sock.data()->_msg->is( Msg::JSON ) );

    ControlConnection* cc = 0;
    bool ok;
    QString key, conntype, nodeid, controlid;
    QVariantMap m = parser.parse( sock.data()->_msg->payload(), &ok ).toMap();
    if( !ok )
    {
        tDebug() << "Invalid JSON on new connection, aborting";
        goto closeconnection;
    }

    conntype  = m.value( "conntype" ).toString();
    key       = m.value( "key" ).toString();
    nodeid    = m.value( "nodeid" ).toString();
    controlid = m.value( "controlid" ).toString();

    tDebug( LOGVERBOSE ) << "Incoming connection details:" << m;

    if( !nodeid.isEmpty() ) // only control connections send nodeid
    {
        bool dupe = false;
        if ( m_connectedNodes.contains( nodeid ) )
            dupe = true;

        foreach( ControlConnection* con, m_controlconnections )
        {
            tLog( LOGVERBOSE ) << "known connection:" << con->id() << con->source()->friendlyName();
            if( con->id() == nodeid )
            {
                dupe = true;
                break;
            }
        }

        if ( dupe )
        {
            tLog() << "Duplicate control connection detected, dropping:" << nodeid << conntype;
            goto closeconnection;
        }
    }

    foreach( ControlConnection* con, m_controlconnections )
    {
        if ( con->id() == controlid )
        {
            cc = con;
            break;
        }
    }

    // they connected to us and want something we are offering
    if ( conntype == "accept-offer" || conntype == "push-offer" )
    {
        sock.data()->_msg.clear();
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << key << nodeid << "socket peer address = " << sock.data()->peerAddress() << "socket peer name = " << sock.data()->peerName();
        Connection* conn = claimOffer( cc, nodeid, key, sock.data()->peerAddress() );
        if ( !conn )
        {
            tLog() << "claimOffer FAILED, key:" << key << nodeid;
            goto closeconnection;
        }
        if ( sock.isNull() )
        {
            tLog() << "Socket has become null, possibly took too long to make an ACL decision, key:" << key << nodeid;
            return;
        }
        else if ( !sock.data()->isValid() )
        {
            tLog() << "Socket has become invalid, possibly took too long to make an ACL decision, key:" << key << nodeid;
            goto closeconnection;
        }
        tDebug( LOGVERBOSE ) << "claimOffer OK:" << key << nodeid;        
        
        m_connectedNodes << nodeid;
        if( !nodeid.isEmpty() )
            conn->setId( nodeid );

        handoverSocket( conn, sock.data() );
        return;
    }
    else
    {
        tLog() << "Invalid or unhandled conntype";
    }

    // fallthru to cleanup:
closeconnection:
    tLog() << "Closing incoming connection, something was wrong.";
    sock.data()->_msg.clear();
    sock.data()->disconnectFromHost();
}


// creates a new tcp connection to peer from conn, handled by given connector
// new_conn is responsible for sending the first msg, if needed
void
Servent::createParallelConnection( Connection* orig_conn, Connection* new_conn, const QString& key )
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << ", key:" << key << thread() << orig_conn;
    // if we can connect to them directly:
    if( orig_conn && orig_conn->outbound() )
    {
        connectToPeer( orig_conn->socket()->peerAddress().toString(),
                       orig_conn->peerPort(),
                       key,
                       new_conn );
    }
    else // ask them to connect to us:
    {
        QString tmpkey = uuid();
        tLog() << "Asking them to connect to us using" << tmpkey ;
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


void
Servent::socketConnected()
{
    QTcpSocketExtra* sock = (QTcpSocketExtra*)sender();

    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << thread() << "socket: " << sock << ", hostaddr: " << sock->peerAddress() << ", hostname: " << sock->peerName();

    Connection* conn = sock->_conn.data();
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
    QTcpSocketExtra* sock = (QTcpSocketExtra*)sender();
    if ( !sock )
    {
        tLog() << "SocketError, sock is null";
        return;
    }

    if ( !sock->_conn.isNull() )
    {
        Connection* conn = sock->_conn.data();
        tLog() << "Servent::SocketError:" << e << conn->id() << conn->name();

        if ( !sock->_disowned )
        {
            // connection will delete if we already transferred ownership, otherwise:
            sock->deleteLater();
        }

        conn->markAsFailed(); // will emit failed, then finished
    }
    else
    {
        tLog() << "SocketError, connection is null";
        sock->deleteLater();
    }
}


void
Servent::connectToPeer( const QString& ha, int port, const QString &key, const QString& name, const QString& id )
{
    Q_ASSERT( this->thread() == QThread::currentThread() );

    ControlConnection* conn = new ControlConnection( this, ha );
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
    tDebug( LOGVERBOSE ) << "Servent::connectToPeer:" << ha << ":" << port
                         << thread() << QThread::currentThread();

    Q_ASSERT( port > 0 );
    Q_ASSERT( conn );

    if ( ( ha == m_externalAddress.toString() || ha == m_externalHostname ) &&
         ( port == m_externalPort ) )
    {
        tDebug() << "ERROR: Tomahawk won't try to connect to" << ha << ":" << port << ": identified as ourselves.";
        return;
    }

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

    connect( sock, SIGNAL( connected() ), SLOT( socketConnected() ) );
    connect( sock, SIGNAL( error( QAbstractSocket::SocketError ) ),
                     SLOT( socketError( QAbstractSocket::SocketError ) ) );

    if ( !conn->peerIpAddress().isNull() )
        sock->connectToHost( conn->peerIpAddress(), port, QTcpSocket::ReadWrite );
    else
        sock->connectToHost( ha, port, QTcpSocket::ReadWrite );
    sock->moveToThread( thread() );
}


void
Servent::reverseOfferRequest( ControlConnection* orig_conn, const QString& theirdbid, const QString& key, const QString& theirkey )
{
    Q_ASSERT( this->thread() == QThread::currentThread() );

    tDebug( LOGVERBOSE ) << "Servent::reverseOfferRequest received for" << key;
    Connection* new_conn = claimOffer( orig_conn, theirdbid, key );
    if ( !new_conn )
    {
        tDebug() << "claimOffer failed, killing requesting connection out of spite";
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
Servent::claimOffer( ControlConnection* cc, const QString &nodeid, const QString &key, const QHostAddress peer )
{
    bool noauth = qApp->arguments().contains( "--noauth" );

    // magic key for stream connections:
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
                tLog() << "File transfer request rejected, invalid source IP";
                return NULL;
            }
        }

        QString fid = key.right( key.length() - 17 );
        StreamConnection* sc = new StreamConnection( this, cc, fid );
        return sc;
    }

    if( key == "whitelist" ) // LAN IP address, check source IP
    {
        if( isIPWhitelisted( peer ) )
        {
            tDebug() << "Connection is from whitelisted IP range (LAN)";
            Connection* conn = new ControlConnection( this, peer.toString() );
            conn->setName( peer.toString() );
            return conn;
        }
        else
        {
            tDebug() << "Connection claimed to be whitelisted, but wasn't.";
            return NULL;
        }
    }

    if( m_offers.contains( key ) )
    {
        QWeakPointer<Connection> conn = m_offers.value( key );
        if( conn.isNull() )
        {
            // This can happen if it's a streamconnection, but the audioengine has
            // already closed the iodevice, causing the connection to be deleted before
            // the peer connects and provides the first byte
            tLog() << Q_FUNC_INFO << "invalid/expired offer:" << key;
            return NULL;
        }

        if( !nodeid.isEmpty() )
        {
            // Used by the connection for the ACL check
            // If there isn't a nodeid it's not the first connection and will already have been stopped
            conn.data()->setProperty( "nodeid", nodeid );
        }

        if( conn.data()->onceOnly() )
        {
            m_offers.remove( key );
            return conn.data();
        }
        else
        {
            return conn.data()->clone();
        }
    }
    else if ( noauth )
    {
        Connection* conn;
        conn = new ControlConnection( this, peer );
        conn->setName( key );
        return conn;
    }
    else
    {
        tLog() << "Invalid offer key:" << key;
        return NULL;
    }
}


QSharedPointer<QIODevice>
Servent::remoteIODeviceFactory( const result_ptr& result )
{
    QSharedPointer<QIODevice> sp;

    QStringList parts = result->url().mid( QString( "servent://" ).length() ).split( "\t" );
    const QString sourceName = parts.at( 0 );
    const QString fileId = parts.at( 1 );
    source_ptr s = SourceList::instance()->get( sourceName );
    if ( s.isNull() || !s->controlConnection() )
        return sp;

    ControlConnection* cc = s->controlConnection();
    StreamConnection* sc = new StreamConnection( this, cc, fileId, result );
    createParallelConnection( cc, sc, QString( "FILE_REQUEST_KEY:%1" ).arg( fileId ) );
    return sc->iodevice();
}


void
Servent::registerStreamConnection( StreamConnection* sc )
{
    Q_ASSERT( !m_scsessions.contains( sc ) );
    tDebug( LOGVERBOSE ) << "Registering Stream" << m_scsessions.length() + 1;

    QMutexLocker lock( &m_ftsession_mut );
    m_scsessions.append( sc );

    printCurrentTransfers();
    emit streamStarted( sc );
}


void
Servent::onStreamFinished( StreamConnection* sc )
{
    Q_ASSERT( sc );
    tDebug( LOGVERBOSE ) << "Stream Finished, unregistering" << sc->id();

    QMutexLocker lock( &m_ftsession_mut );
    m_scsessions.removeAll( sc );

    printCurrentTransfers();
    emit streamFinished( sc );
}


// used for debug output:
void
Servent::printCurrentTransfers()
{
    int k = 1;
//    qDebug() << "~~~ Active file transfer connections:" << m_scsessions.length();
    foreach( StreamConnection* i, m_scsessions )
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
        whitelist << range( QHostAddress( "10.0.0.0" ), 8 )
                  << range( QHostAddress( "172.16.0.0" ), 12 )
                  << range( QHostAddress( "192.168.0.0" ), 16 )
                  << range( QHostAddress( "169.254.0.0" ), 16 )
                  << range( QHostAddress( "127.0.0.0" ), 24 );

//        tDebug( LOGVERBOSE ) << "Loaded whitelist IP range:" << whitelist;
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
    // tell peers we have new stuff they should sync
    QList<source_ptr> sources = SourceList::instance()->sources();
    foreach( const source_ptr& src, sources )
    {
        // skip local source
        if ( src.isNull() || src->isLocal() )
            continue;

        if ( src->controlConnection() && src->controlConnection()->dbSyncConnection() ) // source online?
            src->controlConnection()->dbSyncConnection()->trigger();
    }
}


void
Servent::registerIODeviceFactory( const QString &proto, boost::function<QSharedPointer<QIODevice>(Tomahawk::result_ptr)> fac )
{
    m_iofactories.insert( proto, fac );
}


QSharedPointer<QIODevice>
Servent::getIODeviceForUrl( const Tomahawk::result_ptr& result )
{
    QSharedPointer<QIODevice> sp;

    QRegExp rx( "^([a-zA-Z0-9]+)://(.+)$" );
    if ( rx.indexIn( result->url() ) == -1 )
        return sp;

    const QString proto = rx.cap( 1 );
    if ( !m_iofactories.contains( proto ) )
        return sp;

    return m_iofactories.value( proto )( result );
}


QSharedPointer<QIODevice>
Servent::localFileIODeviceFactory( const Tomahawk::result_ptr& result )
{
    // ignore "file://" at front of url
    QFile* io = new QFile( result->url().mid( QString( "file://" ).length() ) );
    if ( io )
        io->open( QIODevice::ReadOnly );

    return QSharedPointer<QIODevice>( io );
}


QSharedPointer<QIODevice>
Servent::httpIODeviceFactory( const Tomahawk::result_ptr& result )
{
    QNetworkRequest req( result->url() );
    QNetworkReply* reply = TomahawkUtils::nam()->get( req );
    return QSharedPointer<QIODevice>( reply, &QObject::deleteLater );
}
