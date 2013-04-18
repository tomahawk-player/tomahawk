/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright 2013,      Teo Mrnjavac <teo@kde.org>
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

#include "Servent.h"

#include "Result.h"
#include "Source.h"
#include "BufferIoDevice.h"
#include "Connection.h"
#include "ControlConnection.h"
#include "database/Database.h"
#include "database/DatabaseImpl.h"
#include "StreamConnection.h"
#include "SourceList.h"
#include "sip/SipInfo.h"
#include "sip/PeerInfo.h"
#include "sip/SipPlugin.h"
#include "PortFwdThread.h"
#include "TomahawkSettings.h"
#include "utils/TomahawkUtils.h"
#include "utils/Logger.h"
#include "accounts/AccountManager.h"


#include <QtCore/QCoreApplication>
#include <QtCore/QMutexLocker>
#include <QtNetwork/QNetworkInterface>
#include <QtCore/QFile>
#include <QtCore/QThread>
#include <QtNetwork/QNetworkProxy>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>

#include <boost/bind.hpp>

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
{
    s_instance = this;

    m_lanHack = qApp->arguments().contains( "--lanhack" );
    m_noAuth = qApp->arguments().contains( "--noauth" );

    setProxy( QNetworkProxy::NoProxy );

    {
        // _1 = result, _2 = callback function for IODevice
        IODeviceFactoryFunc fac = boost::bind( &Servent::localFileIODeviceFactory, this, _1, _2 );
        this->registerIODeviceFactory( "file", fac );
    }

    {
        IODeviceFactoryFunc fac = boost::bind( &Servent::remoteIODeviceFactory, this, _1, _2 );
        this->registerIODeviceFactory( "servent", fac );
    }

    {
        IODeviceFactoryFunc fac = boost::bind( &Servent::httpIODeviceFactory, this, _1, _2 );
        this->registerIODeviceFactory( "http", fac );
        this->registerIODeviceFactory( "https", fac );
    }
}


Servent::~Servent()
{
    tDebug() << Q_FUNC_INFO;

    foreach ( ControlConnection* cc, m_controlconnections )
        delete cc;

    if ( m_portfwd )
    {
        m_portfwd.data()->quit();
        m_portfwd.data()->wait( 60000 );
        delete m_portfwd.data();
    }
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

    TomahawkSettings::ExternalAddressMode mode = TomahawkSettings::instance()->externalAddressMode();

    tLog() << "Servent listening on port" << m_port << "- servent thread:" << thread()
           << "- address mode:" << (int)( mode );

    // --lanhack means to advertise your LAN IP as if it were externally visible
    switch ( mode )
    {
        case TomahawkSettings::Static:
            m_externalHostname = TomahawkSettings::instance()->externalHostname();
            m_externalPort = TomahawkSettings::instance()->externalPort();
            m_ready = true;
            emit ready();
            break;

        case TomahawkSettings::Lan:
            setInternalAddress();
            break;

        case TomahawkSettings::Upnp:
            if ( !upnp )
            {
                setInternalAddress();
                break;
            }
            // TODO check if we have a public/internet IP on this machine directly
            tLog() << "External address mode set to upnp...";
            m_portfwd = QPointer< PortFwdThread >( new PortFwdThread( m_port ) );
            Q_ASSERT( m_portfwd );
            connect( m_portfwd.data(), SIGNAL( externalAddressDetected( QHostAddress, unsigned int ) ),
                                  SLOT( setExternalAddress( QHostAddress, unsigned int ) ) );
            m_portfwd.data()->start();
            break;
    }

    return true;
}


QString
Servent::createConnectionKey( const QString& name, const QString &nodeid, const QString &key, bool onceOnly )
{
    Q_ASSERT( this->thread() == QThread::currentThread() );

    QString _key = ( key.isEmpty() ? uuid() : key );
    ControlConnection* cc = new ControlConnection( this );
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
        tLog() << "UPnP failed, LAN and outbound connections only!";
        setInternalAddress();
        return;
    }

    tLog() << "UPnP setup successful";
    m_ready = true;
    emit ready();
}


void
Servent::registerOffer( const QString& key, Connection* conn )
{
    m_offers[key] = QPointer<Connection>(conn);
}


void
Servent::registerControlConnection( ControlConnection* conn )
{
    Q_ASSERT( conn );
    tLog( LOGVERBOSE ) << Q_FUNC_INFO << conn->name();
    m_controlconnections << conn;
    m_connectedNodes << conn->id();
}


void
Servent::unregisterControlConnection( ControlConnection* conn )
{
    Q_ASSERT( conn );

    tLog( LOGVERBOSE ) << Q_FUNC_INFO << conn->name();
    m_connectedNodes.removeAll( conn->id() );
    m_controlconnections.removeAll( conn );
}


ControlConnection*
Servent::lookupControlConnection( const SipInfo& sipInfo )
{
    foreach ( ControlConnection* c, m_controlconnections )
    {
        tLog() << sipInfo.port() << c->peerPort() << sipInfo.host() << c->peerIpAddress().toString();
        if ( sipInfo.port() == c->peerPort() && sipInfo.host() == c->peerIpAddress().toString() )
             return c;
    }

    return NULL;
}


void
Servent::registerPeer( const Tomahawk::peerinfo_ptr& peerInfo )
{
    if ( peerInfo->hasControlConnection() )
    {
        peerInfoDebug( peerInfo ) << "already had control connection, doing nothing: " << peerInfo->controlConnection()->name();
        tLog() << "existing control connection has following peers:";
        foreach ( const peerinfo_ptr& otherPeerInfo, peerInfo->controlConnection()->peerInfos() )
        {
            peerInfoDebug( otherPeerInfo );
        }

        tLog() << "end peers";
        return;
    }

    if ( peerInfo->type() == Tomahawk::PeerInfo::Local )
    {
        peerInfoDebug(peerInfo) << "we need to establish the connection now... thinking";
        if ( !connectedToSession( peerInfo->sipInfo().nodeId() ) )
        {
            connectToPeer( peerInfo );
        }
        else
        {
//            FIXME: do we need to port this?!

//            qDebug() << "Already connected to" << host; // so peerInfo was 0 before
//            qDebug() << "They connected to us and we don't have a PeerInfo object, created one...";
//            m_peersOnline.append( peerInfo );

//            // attach to control connection
//            ControlConnection* conn = Servent::instance()->lookupControlConnection( sipInfo );

//            // we're connected to this nodeid, so we should find a control connection for this sipinfo, no?
//            Q_ASSERT( conn );

//            conn->addPeerInfo( peerInfo );
        }
    }
    else
    {
        SipInfo info;
        QString peerId = peerInfo->id();
        QString key = uuid();
        ControlConnection* conn = new ControlConnection( this );

        const QString& nodeid = Database::instance()->impl()->dbid();
        conn->setName( peerInfo->contactId() );
        conn->setId( nodeid );
        conn->addPeerInfo( peerInfo );

        if ( visibleExternally() )
        {
            registerOffer( key, conn );
            info.setVisible( true );
            info.setHost( externalAddress() );
            info.setPort( externalPort() );
            info.setKey( key );
            info.setNodeId( nodeid );

            tDebug() << "Asking them (" << peerInfo->id() << ") to connect to us:" << info;
        }
        else
        {
            info.setVisible( false );
            tDebug() << "We are not visible externally:" << info;
        }

        peerInfo->sendLocalSipInfo( info );

        handleSipInfo( peerInfo );
        connect( peerInfo.data(), SIGNAL( sipInfoChanged() ), SLOT( onSipInfoChanged() ) );
    }
}


void
Servent::onSipInfoChanged()
{
    Tomahawk::PeerInfo* peerInfo = qobject_cast< Tomahawk::PeerInfo* >( sender() );

    if ( !peerInfo )
        return;

    handleSipInfo( peerInfo->weakRef().toStrongRef() );
}


void Servent::handleSipInfo( const Tomahawk::peerinfo_ptr& peerInfo )
{
    tLog() << Q_FUNC_INFO << peerInfo->id() << peerInfo->sipInfo();

    SipInfo info = peerInfo->sipInfo();
    if ( !info.isValid() )
        return;

    /*
        If only one party is externally visible, connection is obvious
        If both are, peer with lowest IP address initiates the connection.

        This avoids dupe connections.
    */
    if ( info.isVisible() )
    {
        if ( !visibleExternally() ||
             externalAddress() < info.host() ||
             ( externalAddress() == info.host() && externalPort() < info.port() ) )
        {

            tDebug() << "Initiate connection to" << peerInfo->id() << "at" << info.host() << "peer of:" << peerInfo->sipPlugin()->account()->accountFriendlyName();
            connectToPeer( peerInfo );
        }
        else
        {
            tDebug() << Q_FUNC_INFO << "They should be conecting to us...";
        }
    }
    else
    {
        tDebug() << Q_FUNC_INFO << "They are not visible, doing nothing atm";

        if ( !visibleExternally() )
        {
            if ( peerInfo->controlConnection() )
                delete peerInfo->controlConnection();
        }
    }
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
    if ( !sock->setSocketDescriptor( sd ) )
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
    QPointer< QTcpSocketExtra > sock = (QTcpSocketExtra*)sender();

    if ( sock.isNull() || sock.data()->_disowned )
    {
        return;
    }

    if ( sock.data()->_msg.isNull() )
    {
        char msgheader[ Msg::headerSize() ];
        if ( sock.data()->bytesAvailable() < Msg::headerSize() )
            return;

        sock.data()->read( (char*) &msgheader, Msg::headerSize() );
        sock.data()->_msg = Msg::begin( (char*) &msgheader );
    }

    if ( sock.data()->bytesAvailable() < sock.data()->_msg->length() )
        return;

    QByteArray ba = sock.data()->read( sock.data()->_msg->length() );
    sock.data()->_msg->fill( ba );

    if ( !sock.data()->_msg->is( Msg::JSON ) )
    {
        tDebug() << ba;
        tDebug() << sock.data()->_msg->payload();
        Q_ASSERT( sock.data()->_msg->is( Msg::JSON ) );
    }

    ControlConnection* cc = 0;
    bool ok;
    QString key, conntype, nodeid, controlid;
    QVariantMap m = parser.parse( sock.data()->_msg->payload(), &ok ).toMap();
    if ( !ok )
    {
        tDebug() << "Invalid JSON on new connection, aborting";
        goto closeconnection;
    }

    conntype  = m.value( "conntype" ).toString();
    key       = m.value( "key" ).toString();
    nodeid    = m.value( "nodeid" ).toString();
    controlid = m.value( "controlid" ).toString();

    tDebug( LOGVERBOSE ) << "Incoming connection details:" << m;
    if ( !nodeid.isEmpty() ) // only control connections send nodeid
    {
        bool dupe = false;
        if ( m_connectedNodes.contains( nodeid ) )
        {
            tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Connected nodes contains it.";
            dupe = true;
        }

        foreach ( ControlConnection* con, m_controlconnections )
        {
            Q_ASSERT( con );

            tLog( LOGVERBOSE ) << Q_FUNC_INFO << "Known connection:" << con->id();
            if ( con->id() == nodeid )
            {
                dupe = true;
                break;
            }
        }

        // for zeroconf there might be no offer, that case is handled later
        ControlConnection* ccMatch = qobject_cast< ControlConnection* >( m_offers.value( key ).data() );
        if ( dupe && ccMatch )
        {
            tLog() << "Duplicate control connection detected, dropping:" << nodeid << conntype;

            tDebug() << "PEERINFO: to be dropped connection has following peers";
            foreach ( const peerinfo_ptr& currentPeerInfo, ccMatch->peerInfos() )
            {
                peerInfoDebug( currentPeerInfo );
            }

            foreach ( ControlConnection* keepConnection, m_controlconnections )
            {
                Q_ASSERT( keepConnection );

                if ( keepConnection->id() == nodeid )
                {
                    tDebug() << "Keep connection" << keepConnection->name() << "with following peers";
                    foreach ( const peerinfo_ptr& currentPeerInfo, keepConnection->peerInfos() )
                        peerInfoDebug( currentPeerInfo );

                    tDebug() << "Add these peers now";
                    foreach ( const peerinfo_ptr& currentPeerInfo, ccMatch->peerInfos() )
                    {
                        tDebug() << "Adding" << currentPeerInfo->id();
                        keepConnection->addPeerInfo( currentPeerInfo );
                    }
                    tDebug() << "Done adding.";
                }
            }
            goto closeconnection;
        }
    }

    foreach ( ControlConnection* con, m_controlconnections )
    {
        Q_ASSERT( con );

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
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << key << nodeid << "socket peer address =" << sock.data()->peerAddress() << "socket peer name =" << sock.data()->peerName();
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

        if ( !nodeid.isEmpty() )
        {
            conn->setId( nodeid );
            registerControlConnection( qobject_cast<ControlConnection*>(conn) );
        }

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
    if ( orig_conn && orig_conn->outbound() )
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
        m.insert( "controlid", Database::instance()->impl()->dbid() );

        QJson::Serializer ser;
        orig_conn->sendMsg( Msg::factory( ser.serialize(m), Msg::JSON ) );
    }
}


void
Servent::socketConnected()
{
    QTcpSocketExtra* sock = (QTcpSocketExtra*)sender();

    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << thread() << "socket:" << sock << ", hostaddr:" << sock->peerAddress() << ", hostname:" << sock->peerName();

    if ( sock->_conn.isNull() )
    {
        sock->close();
        sock->deleteLater();
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Socket's connection was null, could have timed out or been given an invalid address";
        return;
    }

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
Servent::connectToPeer( const peerinfo_ptr& peerInfo )
{
    Q_ASSERT( this->thread() == QThread::currentThread() );

    SipInfo sipInfo = peerInfo->sipInfo();

    peerInfoDebug( peerInfo ) << "connectToPeer: search for already established connections to the same nodeid:" << m_controlconnections.count() << "connections";
    if ( peerInfo->controlConnection() )
        delete peerInfo->controlConnection();

    bool isDupe = false;
    ControlConnection* conn = 0;
    // try to find a ControlConnection with the same SipInfo, then we dont need to try to connect again

    foreach ( ControlConnection* c, m_controlconnections )
    {
        Q_ASSERT( c );

        if ( c->id() == sipInfo.nodeId() )
        {
            conn = c;

            foreach ( const peerinfo_ptr& currentPeerInfo, c->peerInfos() )
            {
                peerInfoDebug( currentPeerInfo ) << "Same object:" << ( peerInfo == currentPeerInfo ) << ( peerInfo.data() == currentPeerInfo.data() ) << ( peerInfo->debugName() == currentPeerInfo->debugName() );

                if ( peerInfo == currentPeerInfo )
                {
                    isDupe = true;
                    peerInfoDebug( currentPeerInfo ) << "Not adding, because it's a dupe: peerInfoCount remains the same" << conn->peerInfos().count();
                    break;
                }
            }

            if ( !c->peerInfos().contains( peerInfo ) )
            {
                c->addPeerInfo( peerInfo );
//                peerInfoDebug(peerInfo) << "Adding " << peerInfo->debugName() << ", not a dupe... new peerInfoCount:" << c->peerInfos().count();
//                foreach ( const peerinfo_ptr& kuh, c->peerInfos() )
//                {
//                    peerInfoDebug(peerInfo) << " ** " << kuh->debugName();
//                }
            }

            break;
        }
    }

    peerInfoDebug( peerInfo ) << "connectToPeer: found a match:" << ( conn ? conn->name() : "false" ) << "dupe:" << isDupe;

    if ( isDupe )
    {
        peerInfoDebug( peerInfo ) << "it's a dupe, nothing to do here, returning and stopping processing: peerInfoCount:" << conn->peerInfos().count();
    }

    if ( conn )
        return;

    QVariantMap m;
    m["conntype"]  = "accept-offer";
    m["key"]       = sipInfo.key();
    m["port"]      = externalPort();
    m["nodeid"]    = Database::instance()->impl()->dbid();

    peerInfoDebug(peerInfo) << "No match found, creating a new ControlConnection...";
    conn = new ControlConnection( this );
    conn->addPeerInfo( peerInfo );
    conn->setFirstMessage( m );

    if ( peerInfo->id().length() )
        conn->setName( peerInfo->contactId() );
    if ( sipInfo.nodeId().length() )
        conn->setId( sipInfo.nodeId() );

    conn->setProperty( "nodeid", sipInfo.nodeId() );

    registerControlConnection( conn );
    connectToPeer( sipInfo.host(), sipInfo.port(), sipInfo.key(), conn );
}


void
Servent::connectToPeer( const QString& ha, int port, const QString& key, Connection* conn )
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

    if ( key.length() && conn->firstMessage().isNull() )
    {
        QVariantMap m;
        m["conntype"]  = "accept-offer";
        m["key"]       = key;
        m["port"]      = externalPort();
        m["controlid"] = Database::instance()->impl()->dbid();
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
    m["controlid"] = Database::instance()->impl()->dbid();
    new_conn->setFirstMessage( m );
    createParallelConnection( orig_conn, new_conn, QString() );
}


// return the appropriate connection for a given offer key, or NULL if invalid
Connection*
Servent::claimOffer( ControlConnection* cc, const QString &nodeid, const QString &key, const QHostAddress peer )
{
    // magic key for stream connections:
    if ( key.startsWith( "FILE_REQUEST_KEY:" ) )
    {
        // check if the source IP matches an existing, authenticated connection
        if ( !m_noAuth && peer != QHostAddress::Any && !isIPWhitelisted( peer ) )
        {
            bool authed = false;
            foreach ( ControlConnection* cc, m_controlconnections )
            {
                tDebug() << Q_FUNC_INFO << "Probing:" << cc->name();
                if ( cc->socket() && cc->socket()->peerAddress() == peer )
                {
                    authed = true;
                    break;
                }
            }
            if ( !authed )
            {
                tLog() << "File transfer request rejected, invalid source IP";
                return NULL;
            }
        }

        QString fid = key.right( key.length() - 17 );
        StreamConnection* sc = new StreamConnection( this, cc, fid );
        return sc;
    }

    if ( key == "whitelist" ) // LAN IP address, check source IP
    {
        if ( isIPWhitelisted( peer ) )
        {
            tDebug() << "Connection is from whitelisted IP range (LAN)";
            ControlConnection* conn = new ControlConnection( this );
            conn->setName( peer.toString() );

            Tomahawk::Accounts::Account* account = Tomahawk::Accounts::AccountManager::instance()->zeroconfAccount();

            // if we get this connection the account should exist and be enabled
            Q_ASSERT( account );
            Q_ASSERT( account->enabled() );

            // this is terrible, actually there should be a way to let this be created by the zeroconf plugin
            // because this way we rely on the ip being used as id in two totally different parts of the code
            Tomahawk::peerinfo_ptr peerInfo = Tomahawk::PeerInfo::get( account->sipPlugin(), peer.toString(), Tomahawk::PeerInfo::AutoCreate );
            peerInfo->setContactId( peer.toString() );
            peerInfoDebug( peerInfo );
            conn->addPeerInfo( peerInfo );
            return conn;
        }
        else
        {
            tDebug() << "Connection claimed to be whitelisted, but wasn't.";
            return NULL;
        }
    }

    if ( m_offers.contains( key ) )
    {
        QPointer<Connection> conn = m_offers.value( key );
        if ( conn.isNull() )
        {
            // This can happen if it's a streamconnection, but the audioengine has
            // already closed the iodevice, causing the connection to be deleted before
            // the peer connects and provides the first byte
            tLog() << Q_FUNC_INFO << "invalid/expired offer:" << key;
            return NULL;
        }

        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "nodeid is: " << nodeid;
        if ( !nodeid.isEmpty() )
        {
            // Used by the connection for the ACL check
            // If there isn't a nodeid it's not the first connection and will already have been stopped
            conn.data()->setProperty( "nodeid", nodeid );
        }

        if ( conn.data()->onceOnly() )
        {
            m_offers.remove( key );
            return conn.data();
        }
        else
        {
            return conn.data()->clone();
        }
    }
    else if ( m_noAuth )
    {
        Connection* conn;
        conn = new ControlConnection( this );
        conn->setName( key );
        return conn;
    }
    else
    {
        tLog() << "Invalid offer key:" << key;
        return NULL;
    }
}


void
Servent::remoteIODeviceFactory( const Tomahawk::result_ptr& result,
                                boost::function< void ( QSharedPointer< QIODevice >& ) > callback )
{
    QSharedPointer<QIODevice> sp;

    QStringList parts = result->url().mid( QString( "servent://" ).length() ).split( "\t" );
    const QString sourceName = parts.at( 0 );
    const QString fileId = parts.at( 1 );
    source_ptr s = SourceList::instance()->get( sourceName );
    if ( s.isNull() || !s->controlConnection() )
    {
        callback( sp );
        return;
    }

    ControlConnection* cc = s->controlConnection();
    StreamConnection* sc = new StreamConnection( this, cc, fileId, result );
    createParallelConnection( cc, sc, QString( "FILE_REQUEST_KEY:%1" ).arg( fileId ) );

    //boost::functions cannot accept temporaries as parameters
    sp = sc->iodevice();
    callback( sp );
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
    foreach ( StreamConnection* i, m_scsessions )
    {
        qDebug() << k << ") " << i->id();
    }
    qDebug() << endl;
}


bool
Servent::isIPWhitelisted( QHostAddress ip )
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Performing checks against ip" << ip.toString();
    typedef QPair< QHostAddress, int > range;
    QList< range > subnetEntries;

    QList< QNetworkInterface > networkInterfaces = QNetworkInterface::allInterfaces();
    foreach ( QNetworkInterface interface, networkInterfaces )
    {
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Checking interface" << interface.humanReadableName();
        QList< QNetworkAddressEntry > addressEntries = interface.addressEntries();
        foreach ( QNetworkAddressEntry addressEntry, addressEntries )
        {
            tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Checking address entry with ip" << addressEntry.ip().toString() << "and prefix length" << addressEntry.prefixLength();
            if ( ip.isInSubnet( addressEntry.ip(), addressEntry.prefixLength() ) )
            {
                tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "success";
                return true;
            }
        }
    }
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "failure";
    return false;
}


bool
Servent::connectedToSession( const QString& session )
{
    foreach ( ControlConnection* cc, m_controlconnections )
    {
        Q_ASSERT( cc );

        if ( cc->id() == session )
            return true;
    }

    return false;
}


void
Servent::triggerDBSync()
{
    // tell peers we have new stuff they should sync
    QList< source_ptr > sources = SourceList::instance()->sources();
    foreach ( const source_ptr& src, sources )
    {
        // skip local source
        if ( src.isNull() || src->isLocal() )
            continue;

        if ( src->controlConnection() && src->controlConnection()->dbSyncConnection() ) // source online?
            src->controlConnection()->dbSyncConnection()->trigger();
    }
    emit dbSyncTriggered();
}


void
Servent::registerIODeviceFactory( const QString &proto,
                                  IODeviceFactoryFunc fac )
{
    m_iofactories.insert( proto, fac );
}


void
Servent::getIODeviceForUrl( const Tomahawk::result_ptr& result,
                            boost::function< void ( QSharedPointer< QIODevice >& ) > callback )
{
    QSharedPointer<QIODevice> sp;

    QRegExp rx( "^([a-zA-Z0-9]+)://(.+)$" );
    if ( rx.indexIn( result->url() ) == -1 )
    {
        callback( sp );
        return;
    }

    const QString proto = rx.cap( 1 );
    if ( !m_iofactories.contains( proto ) )
    {
        callback( sp );
        return;
    }

    //QtScriptResolverHelper::customIODeviceFactory is async!
    m_iofactories.value( proto )( result, callback );
}


void
Servent::localFileIODeviceFactory( const Tomahawk::result_ptr& result,
                                   boost::function< void ( QSharedPointer< QIODevice >& ) > callback )
{
    // ignore "file://" at front of url
    QFile* io = new QFile( result->url().mid( QString( "file://" ).length() ) );
    if ( io )
        io->open( QIODevice::ReadOnly );

    //boost::functions cannot accept temporaries as parameters
    QSharedPointer< QIODevice > sp = QSharedPointer<QIODevice>( io );
    callback( sp );
}


void
Servent::httpIODeviceFactory( const Tomahawk::result_ptr& result,
                              boost::function< void ( QSharedPointer< QIODevice >& ) > callback )
{
    QNetworkRequest req( result->url() );
    QNetworkReply* reply = TomahawkUtils::nam()->get( req );

    //boost::functions cannot accept temporaries as parameters
    QSharedPointer< QIODevice > sp = QSharedPointer< QIODevice >( reply, &QObject::deleteLater );
    callback( sp );
}
