/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2015, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright 2013,      Teo Mrnjavac <teo@kde.org>
 *   Copyright 2013-2014, Uwe L. Korn <uwelk@xhochy.com>
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

#include "Servent_p.h"

#include "accounts/AccountManager.h"
#include "database/Database.h"
#include "database/DatabaseImpl.h"
#include "network/acl/AclRegistry.h"
#include "network/Msg.h"
#include "network/ConnectionManager.h"
#include "network/DbSyncConnection.h"
#include "sip/SipInfo.h"
#include "sip/PeerInfo.h"
#include "sip/SipPlugin.h"
#include "utils/Closure.h"
#include "utils/Json.h"
#include "utils/TomahawkUtils.h"
#include "utils/Logger.h"
#include "utils/NetworkAccessManager.h"
#include "utils/NetworkReply.h"

#include "Connection.h"
#include "ControlConnection.h"
#include "PortFwdThread.h"
#include "QTcpSocketExtra.h"
#include "Source.h"
#include "SourceList.h"
#include "StreamConnection.h"
#include "UrlHandler.h"

#include <QCoreApplication>
#include <QMutexLocker>
#include <QNetworkInterface>
#include <QFile>
#include <QThread>
#include <QNetworkProxy>
#include <QNetworkRequest>
#include <QNetworkReply>


typedef QPair< QList< SipInfo >, Connection* > sipConnectionPair;
Q_DECLARE_METATYPE( sipConnectionPair )
Q_DECLARE_METATYPE( QList< SipInfo > )
Q_DECLARE_METATYPE( Connection* )
Q_DECLARE_METATYPE( QTcpSocketExtra* )
Q_DECLARE_METATYPE( Tomahawk::peerinfo_ptr )


using namespace Tomahawk;

Servent* Servent::s_instance = 0;


Servent*
Servent::instance()
{
    return s_instance;
}


Servent::Servent( QObject* parent )
    : QTcpServer( parent ),
      d_ptr( new ServentPrivate( this ) )
{
    s_instance = this;

    d_func()->noAuth = qApp->arguments().contains( "--noauth" );

    setProxy( QNetworkProxy::NoProxy );

    IODeviceFactoryFunc fac = std::bind( &Servent::remoteIODeviceFactory, this,
                                         std::placeholders::_1,
                                         std::placeholders::_2,
                                         std::placeholders::_3 );
    Tomahawk::UrlHandler::registerIODeviceFactory( "servent", fac );
}


Servent::~Servent()
{
    tDebug() << Q_FUNC_INFO;

    foreach ( ControlConnection* cc, d_func()->controlconnections )
        delete cc;

    if ( d_func()->portfwd )
    {
        d_func()->portfwd.data()->quit();
        d_func()->portfwd.data()->wait( 60000 );
        delete d_func()->portfwd.data();
    }

    delete d_ptr;
}


bool
Servent::startListening( QHostAddress ha, bool upnp, int port, Tomahawk::Network::ExternalAddress::Mode mode, int defaultPort, bool autoDetectExternalIp, const QString& externalHost, int externalPort )
{
    Q_D( Servent );

    d->externalAddresses = QList<QHostAddress>();
    d->port = port;

    // Listen on both the selected port and, if not the same, the default port -- the latter sometimes necessary for zeroconf
    // TODO: only listen on both when zeroconf sip is enabled
    // TODO: use a real zeroconf system instead of a simple UDP broadcast?
    if ( !listen( ha, d->port ) )
    {
        if ( d->port != defaultPort )
        {
            if ( !listen( ha, defaultPort ) )
            {
                tLog() << Q_FUNC_INFO << "Failed to listen on both port" << d->port << "and port" << defaultPort;
                tLog() << Q_FUNC_INFO << "Error string is:" << errorString();
                return false;
            }
            else
                d->port = defaultPort;
        }
    }

    d->externalListenAll = false;

    if ( ha == QHostAddress::Any || ha == QHostAddress::AnyIPv6 )
    {
        // We are listening on all available addresses, so we should send a SipInfo for all of them.
        d->externalAddresses = QNetworkInterface::allAddresses();
        cleanAddresses( d->externalAddresses );
        tLog( LOGVERBOSE ) << Q_FUNC_INFO << "Listening to" << d->externalAddresses;
        d->externalListenAll = true;
    }
    else if ( ( ha.toString() != "127.0.0.1" ) && ( ha.toString() != "::1" ) && ( ha.toString() != "::7F00:1" ) )
    {
        // We listen only to one specific Address, only announce this.
        d->externalAddresses.append( ha );
    }
    // If we only accept connections via localhost, we'll announce nothing.

    tLog( LOGVERBOSE ) << Q_FUNC_INFO << "Servent listening on port" << d->port
                       << "using address" << ha.toString()
                       << "- servent thread:" << thread()
                       << "- address mode:" << (int)( mode );

    switch ( mode )
    {
        case Tomahawk::Network::ExternalAddress::Static:
            d->externalPort = externalPort;
            if ( autoDetectExternalIp )
            {
                QNetworkReply* reply = Tomahawk::Utils::nam()->get( QNetworkRequest( QUrl( "http://toma.hk/?stat=1" ) ) );
                connect( reply, SIGNAL( finished() ), SLOT( ipDetected() ) );
                // Not emitting ready here as we are not done.
            }
            else
            {
                d->externalHostname = externalHost;
                d->ready = true;
                // All setup is made, were done.
                emit ready();
            }
            break;

        case Tomahawk::Network::ExternalAddress::Lan:
            // Nothing has to be done here.
            d->ready = true;
            emit ready();
            break;

        case Tomahawk::Network::ExternalAddress::Upnp:
            if ( upnp )
            {
                // upnp could be turned off on the cli with --noupnp
                tLog( LOGVERBOSE ) << Q_FUNC_INFO << "External address mode set to upnp...";
                d->portfwd = QPointer< PortFwdThread >( new PortFwdThread( d->port ) );
                Q_ASSERT( d->portfwd );
                connect( d->portfwd.data(), SIGNAL( externalAddressDetected( QHostAddress, unsigned int ) ),
                                      SLOT( setExternalAddress( QHostAddress, unsigned int ) ) );
                d->portfwd.data()->start();
            }
            else
            {
                d->ready = true;
                emit ready();
            }
            break;
    }

    connect( ACLRegistry::instance(), SIGNAL( aclResult( QString, QString, Tomahawk::ACLStatus::Type ) ),
             this, SLOT( checkACLResult( QString, QString, Tomahawk::ACLStatus::Type ) ),
             Qt::QueuedConnection );

    return true;
}


void
Servent::setExternalAddress( QHostAddress ha, unsigned int port )
{
    if ( isValidExternalIP( ha ) )
    {
        d_func()->externalHostname = ha.toString();
        d_func()->externalPort = port;
    }

    if ( d_func()->externalPort == 0 || !isValidExternalIP( ha ) )
        tLog() << Q_FUNC_INFO << "UPnP failed, no further external address could be acquired!";
    else
        tLog( LOGVERBOSE ) << Q_FUNC_INFO << "UPnP setup successful";

    d_func()->ready = true;
    emit ready();
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
Servent::isValidExternalIP( const QHostAddress& addr )
{
    QString ip = addr.toString();
    if (addr.protocol() == QAbstractSocket::IPv4Protocol)
    {
        // private network
        if ( addr.isInSubnet(QHostAddress::parseSubnet( "10.0.0.0/8" ) ) )
            return false;
        // localhost
        if ( addr.isInSubnet(QHostAddress::parseSubnet( "127.0.0.0/8" ) ) )
            return false;
        // private network
        if ( addr.isInSubnet(QHostAddress::parseSubnet( "169.254.0.0/16" ) ) )
            return false;
        // private network
        if ( addr.isInSubnet(QHostAddress::parseSubnet( "172.16.0.0/12" ) ) )
            return false;
        // private network
        if ( addr.isInSubnet(QHostAddress::parseSubnet( "192.168.0.0/16" ) ) )
            return false;
        // multicast
        if ( addr.isInSubnet(QHostAddress::parseSubnet( "224.0.0.0/4" ) ) )
            return false;
    }
    else if (addr.protocol() == QAbstractSocket::IPv6Protocol)
    {
        // "unspecified address"
        if ( addr.isInSubnet(QHostAddress::parseSubnet( "::/128" ) ) )
            return false;
        // link-local
        if ( addr.isInSubnet(QHostAddress::parseSubnet( "fe80::/10" ) ) )
            return false;
        // unique local addresses
        if ( addr.isInSubnet(QHostAddress::parseSubnet( "fc00::/7" ) ) )
            return false;
        // benchmarking only
        if ( addr.isInSubnet(QHostAddress::parseSubnet( "2001:2::/48" ) ) )
            return false;
        // non-routed IPv6 addresses used for Cryptographic Hash Identifiers
        if ( addr.isInSubnet(QHostAddress::parseSubnet( "2001:10::/28" ) ) )
            return false;
        // documentation prefix
        if ( addr.isInSubnet(QHostAddress::parseSubnet( "2001:db8::/32" ) ) )
            return false;
        // multicast
        if ( addr.isInSubnet(QHostAddress::parseSubnet( "ff00::0/8" ) ) )
            return false;
    }
    else
    {
        return false;
    }

    return !addr.isNull();
}


void
Servent::registerOffer( const QString& key, Connection* conn )
{
    d_func()->offers[key] = QPointer<Connection>(conn);
}


void
Servent::registerLazyOffer(const QString &key, const peerinfo_ptr &peerInfo, const QString &nodeid, const int timeout )
{
    d_func()->lazyoffers[key] = QPair< peerinfo_ptr, QString >( peerInfo, nodeid );
    QTimer* timer = new QTimer( this );
    timer->setInterval( timeout );
    timer->setSingleShot( true );
    NewClosure( timer, SIGNAL( timeout() ), this, SLOT( deleteLazyOffer( const QString& ) ), key );
    timer->start();
}


void
Servent::deleteLazyOffer( const QString& key )
{
    d_func()->lazyoffers.remove( key );

    // Cleanup.
    QTimer* timer = (QTimer*)sender();
    if ( timer )
    {
        timer->deleteLater();
    }
}


void
Servent::registerControlConnection( ControlConnection* conn )
{
    Q_D( Servent );
    Q_ASSERT( conn );

    QMutexLocker locker( &d->controlconnectionsMutex );
    tLog( LOGVERBOSE ) << Q_FUNC_INFO << conn->name();
    d->controlconnections << conn;
    d->connectedNodes << conn->id();
}


void
Servent::unregisterControlConnection( ControlConnection* conn )
{
    Q_D( Servent );
    Q_ASSERT( conn );

    QMutexLocker locker( &d->controlconnectionsMutex );
    tLog( LOGVERBOSE ) << Q_FUNC_INFO << conn->name();
    d->connectedNodes.removeAll( conn->id() );
    d->controlconnections.removeAll( conn );
}


ControlConnection*
Servent::lookupControlConnection( const SipInfo& sipInfo )
{
    Q_D( Servent );
    QMutexLocker locker( &d->controlconnectionsMutex );

    foreach ( ControlConnection* c, d_func()->controlconnections )
    {
        tLog() << sipInfo.port() << c->peerPort() << sipInfo.host() << c->peerIpAddress().toString();
        if ( sipInfo.port() == c->peerPort() && sipInfo.host() == c->peerIpAddress().toString() )
             return c;
    }

    return NULL;
}


ControlConnection*
Servent::lookupControlConnection( const QString& nodeid )
{
    Q_D( Servent );
    QMutexLocker locker( &d->controlconnectionsMutex );

    foreach ( ControlConnection* c, d_func()->controlconnections )
    {
        if ( c->id() == nodeid )
             return c;
    }
    return NULL;
}


QList<SipInfo>
Servent::getLocalSipInfos( const QString& nodeid, const QString& key )
{
    Q_D( Servent );

    QList<SipInfo> sipInfos = QList<SipInfo>();
    QList<QHostAddress> addresses = d->externalAddresses;

    if ( d->externalListenAll )
    {
        addresses = QNetworkInterface::allAddresses();
        cleanAddresses( addresses );
    }

    foreach ( QHostAddress ha, addresses )
    {
        SipInfo info = SipInfo();
        info.setHost( ha.toString() );
        info.setPort( d_func()->port );
        info.setKey( key );
        info.setVisible( true );
        info.setNodeId( nodeid );
        sipInfos.append( info );
    }
    if ( !d_func()->externalHostname.isEmpty() )
    {
        SipInfo info = SipInfo();
        info.setHost( d_func()->externalHostname );
        info.setPort( d_func()->externalPort );
        info.setKey( key );
        info.setVisible( true );
        info.setNodeId( nodeid );
        sipInfos.append( info );
    }

    if ( sipInfos.isEmpty() )
    {
        // We are not visible via any IP, send a dummy SipInfo
        SipInfo info = SipInfo();
        info.setVisible( false );
        info.setKey( key );
        info.setNodeId( nodeid );
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Only accepting connections, no usable IP to listen to found.";
    }

    return sipInfos;
}


void
Servent::queueForAclResult( const QString& username, const QSet<peerinfo_ptr>& peerInfos )
{
    if ( peerInfos.isEmpty() || (*peerInfos.begin())->sipInfos().isEmpty() )
    {
        // If all peerInfos disappeared, do not queue.
        // If the peerInfo has not got a sipInfo anymore, do not queue either.
        return;
    }

    if ( !d_func()->queuedForACLResult.contains( username ) )
    {
        d_func()->queuedForACLResult[username] = QMap<QString, QSet<Tomahawk::peerinfo_ptr> >();
    }
    d_func()->queuedForACLResult[username][ (*peerInfos.begin())->nodeId() ] = QSet<Tomahawk::peerinfo_ptr>( peerInfos );
}


SipInfo
Servent::getSipInfoForOldVersions( const QList<SipInfo>& sipInfos )
{
    SipInfo info = SipInfo();
    info.setVisible( false );
    foreach ( SipInfo _info, sipInfos )
    {
        QHostAddress ha = QHostAddress( _info.host() );
        if ( ( Servent::isValidExternalIP( ha ) && ha.protocol() == QAbstractSocket::IPv4Protocol ) || ( ha.protocol() == QAbstractSocket::UnknownNetworkLayerProtocol ) || ( ha.isNull() && !_info.host().isEmpty() ))
        {
            info = _info;
            break;
        }
    }

    return info;
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
        if ( !connectedToSession( peerInfo->nodeId() ) )
        {
            handleSipInfo( peerInfo );
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
        QString key = uuid();
        const QString& nodeid = Database::instance()->impl()->dbid();

        QList<SipInfo> sipInfos = getLocalSipInfos( nodeid, key );
        // The offer should be removed after some time or we will build up a heap of unused PeerInfos
        registerLazyOffer( key, peerInfo, nodeid, sipInfos.length() * 1.5 * CONNECT_TIMEOUT );
        // SipInfos were single-value before 0.7.100
        if ( !peerInfo->versionString().isEmpty() && TomahawkUtils::compareVersionStrings( peerInfo->versionString().split(' ').last(), "0.7.100" ) < 0)
        {
            SipInfo info = getSipInfoForOldVersions( sipInfos );
            peerInfo->sendLocalSipInfos( QList<SipInfo>() << info );
        }
        else
        {
            peerInfo->sendLocalSipInfos( sipInfos );
        }

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


void
Servent::handleSipInfo( const Tomahawk::peerinfo_ptr& peerInfo )
{
    // We do not have received the initial SipInfo for this client yet, so wait for it.
    // Each client will have at least one non-visible SipInfo
    if ( peerInfo->sipInfos().isEmpty() )
        return;

    QSharedPointer<ConnectionManager> manager = ConnectionManager::getManagerForNodeId( peerInfo->nodeId() );
    manager->handleSipInfo( peerInfo );
}


void
Servent::incomingConnection( qintptr sd )
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
    Q_D( Servent );
    Q_ASSERT( this->thread() == QThread::currentThread() );
    QPointer< QTcpSocketExtra > sock = (QTcpSocketExtra*)sender();
    tLog( LOGVERBOSE ) << Q_FUNC_INFO << "Starting to read from new incoming connection from: " << sock->peerAddress().toString();

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
    QVariantMap m = TomahawkUtils::parseJson( sock.data()->_msg->payload(), &ok ).toMap();
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
        QMutexLocker locker( &d->controlconnectionsMutex );
        bool dupe = false;
        if ( d_func()->connectedNodes.contains( nodeid ) )
        {
            tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Connected nodes contains it.";
            dupe = true;
        }

        foreach ( ControlConnection* con, d_func()->controlconnections )
        {
            Q_ASSERT( con );

            tLog( LOGVERBOSE ) << Q_FUNC_INFO << "Known connection:" << con->id();
            // Only check for known inboud ControlConnections
            if ( con->id() == nodeid && !con->outbound() )
            {
                dupe = true;
                break;
            }
        }

        // for zeroconf there might be no offer, that case is handled later
        ControlConnection* ccMatch = qobject_cast< ControlConnection* >( d_func()->offers.value( key ).data() );
        if ( dupe && ccMatch )
        {
            tLog() << "Duplicate control connection detected, dropping:" << nodeid << conntype;

            tDebug() << "PEERINFO: to be dropped connection has following peers";
            foreach ( const peerinfo_ptr& currentPeerInfo, ccMatch->peerInfos() )
            {
                peerInfoDebug( currentPeerInfo );
            }

            foreach ( ControlConnection* keepConnection, d_func()->controlconnections )
            {
                Q_ASSERT( keepConnection );

                // Only check for known inboud ControlConnections
                if ( keepConnection->id() == nodeid && !keepConnection->outbound() )
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

    {
        QMutexLocker locker( &d->controlconnectionsMutex );
        foreach ( ControlConnection* con, d_func()->controlconnections )
        {
            Q_ASSERT( con );

            // Only check for known inboud ControlConnections
            if ( con->id() == controlid && !con->outbound() )
            {
                cc = con;
                break;
            }
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
        SipInfo info = SipInfo();
        info.setVisible( true );
        info.setKey( key );
        info.setNodeId( orig_conn->id() );
        info.setHost( orig_conn->socket()->peerName() );
        info.setPort( orig_conn->peerPort() );
        Q_ASSERT( info.isValid() );
        initiateConnection( info, new_conn );
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
        m.insert( "controlid", Database::instance()->impl()->dbid() );

        if (orig_conn) {
            orig_conn->sendMsg( Msg::factory( TomahawkUtils::toJson( m ), Msg::JSON ) );
        }
    }
}


void
Servent::socketConnected()
{
    QTcpSocketExtra* sock = (QTcpSocketExtra*)sender();

    tLog( LOGVERBOSE ) << Q_FUNC_INFO << thread() << "socket:" << sock << ", hostaddr:" << sock->peerAddress() << ", hostname:" << sock->peerName();

    if ( sock->_conn.isNull() )
    {
        sock->close();
        sock->deleteLater();
        tLog( LOGVERBOSE ) << Q_FUNC_INFO << "Socket's connection was null, could have timed out or been given an invalid address";
        return;
    }

    Connection* conn = sock->_conn.data();
    handoverSocket( conn, sock );
}


// transfers ownership of socket to the connection and inits the connection
void
Servent::handoverSocket( Connection* conn, QTcpSocketExtra* sock )
{
    Q_ASSERT( conn );
    Q_ASSERT( sock );
    Q_ASSERT( conn->socket().isNull() );
    Q_ASSERT( sock->isValid() );

    disconnect( sock, SIGNAL( readyRead() ),    this, SLOT( readyRead() ) );
    disconnect( sock, SIGNAL( disconnected() ), sock, SLOT( deleteLater() ) );
    disconnect( sock, SIGNAL( error( QAbstractSocket::SocketError ) ), this, SLOT( socketError( QAbstractSocket::SocketError ) ) );

    sock->_disowned = true;
    conn->setOutbound( sock->_outbound );
    conn->setPeerPort( sock->peerPort() );

    conn->start( sock );
}


void
Servent::cleanupSocket( QTcpSocketExtra* sock )
{
    if ( !sock )
    {
        tLog() << "SocketError, sock is null";
        return;
    }

    if ( sock->_conn.isNull() )
    {
        tLog() << "SocketError, connection is null";
    }
    sock->deleteLater();
}


void
Servent::initiateConnection( const SipInfo& sipInfo, Connection* conn )
{
    Q_D( Servent );

    Q_ASSERT( sipInfo.isValid() );
    Q_ASSERT( sipInfo.isVisible() );
    Q_ASSERT( sipInfo.port() > 0 );
    Q_ASSERT( conn );

    // Check that we are not connecting to ourselves
    QList<QHostAddress> addresses = d->externalAddresses;

    if ( d->externalListenAll )
    {
        addresses = QNetworkInterface::allAddresses();
    }

    foreach ( QHostAddress ha, addresses )
    {
        if ( sipInfo.host() == ha.toString() && sipInfo.port() == d_func()->port )
        {
            tLog( LOGVERBOSE ) << Q_FUNC_INFO << "Tomahawk won't try to connect to" << sipInfo.host() << ":" << sipInfo.port() << ": same IP as ourselves.";
            return;
        }
    }
    if ( sipInfo.host() == d_func()->externalHostname && sipInfo.port() == d_func()->port )
    {
        tLog( LOGVERBOSE ) << Q_FUNC_INFO << "Tomahawk won't try to connect to" << sipInfo.host() << ":" << sipInfo.port() << ": same IP as ourselves.";
        return;
    }

    if ( conn->firstMessage().isNull() )
    {
        QVariantMap m;
        m["conntype"]  = "accept-offer";
        m["key"]       = sipInfo.key();
        m["controlid"] = Database::instance()->impl()->dbid();
        conn->setFirstMessage( m );
    }

    QTcpSocketExtra* sock = new QTcpSocketExtra();
    sock->setConnectTimeout( CONNECT_TIMEOUT );
    sock->_disowned = false;
    sock->_conn = conn;
    sock->_outbound = true;

    connect( sock, SIGNAL( connected() ), SLOT( socketConnected() ) );
    connect( sock, SIGNAL( error( QAbstractSocket::SocketError ) ), this, SLOT( socketError( QAbstractSocket::SocketError ) ) );

    if ( !conn->peerIpAddress().isNull() )
        sock->connectToHost( conn->peerIpAddress(), sipInfo.port(), QTcpSocket::ReadWrite );
    else
        sock->connectToHost( sipInfo.host(), sipInfo.port(), QTcpSocket::ReadWrite );
    sock->moveToThread( thread() );
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
Servent::checkACLResult( const QString& nodeid, const QString& username, Tomahawk::ACLStatus::Type peerStatus )
{
    Q_D( Servent );

    if ( !d->queuedForACLResult.contains( username ) )
    {
        return;
    }
    if ( !d->queuedForACLResult.value( username ).contains( nodeid ) )
    {
        return;
    }

    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << QString( "ACL status for user %1 is" ).arg( username ) << peerStatus;
    QSet<Tomahawk::peerinfo_ptr> peerInfos = d->queuedForACLResult.value( username ).value( nodeid );
    if ( peerStatus == Tomahawk::ACLStatus::Stream )
    {
        foreach ( const Tomahawk::peerinfo_ptr& peerInfo, peerInfos )
        {
            registerPeer( peerInfo );
        }

    }
    // We have a result, so remove from queue
    d->queuedForACLResult[username].remove( nodeid );
}


void
Servent::ipDetected()
{
    Q_D( Servent );
    QNetworkReply* reply = qobject_cast<QNetworkReply*>( sender() );

    if ( reply->error() == QNetworkReply::NoError )
    {
        bool ok;
        // We are called when the NetworkReply has finished so we should have all data available.
        const QVariantMap res = TomahawkUtils::parseJson( reply->readAll(), &ok ).toMap();
        if ( !ok )
        {
            tLog() << Q_FUNC_INFO << "Failed parsing ip-autodetection response";
            d->externalPort = -1;
            emit ipDetectionFailed( QNetworkReply::NoError, tr( "Automatically detecting external IP failed: Could not parse JSON response." ) );
        }
        else
        {
            QString externalIP = res.value( "ip" ).toString();
            tLog( LOGVERBOSE ) << Q_FUNC_INFO << "Found external IP:" << externalIP;
            d->externalHostname = externalIP;
        }
    }
    else
    {
        d->externalPort = -1;
        tLog() << Q_FUNC_INFO << "ip-autodetection returned an error:" << reply->errorString();
        emit ipDetectionFailed( reply->error(), tr( "Automatically detecting external IP failed: %1" ).arg( reply->errorString() ) );
    }

    d->ready = true;
    emit ready();
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
    m["controlid"] = Database::instance()->impl()->dbid();
    new_conn->setFirstMessage( m );
    createParallelConnection( orig_conn, new_conn, theirkey );
}


bool
Servent::visibleExternally() const
{
    return !d_func()->externalHostname.isNull() || !d_func()->externalAddresses.isEmpty();
}


bool
Servent::ipv6ConnectivityLikely() const
{
    foreach ( QHostAddress ha, d_func()->externalAddresses )
    {
        if ( ha.protocol() == QAbstractSocket::IPv6Protocol && Servent::isValidExternalIP( ha ) )
        {
            return true;
        }
    }

    return false;
}


int
Servent::port() const
{
    return d_func()->port;
}


QList<QHostAddress>
Servent::addresses() const
{
    Q_D( const Servent );

    if ( d->externalListenAll )
    {
        QList<QHostAddress> addresses( QNetworkInterface::allAddresses() );
        cleanAddresses( addresses );
        return addresses;
    }

    return d->externalAddresses;
}


QString
Servent::additionalAddress() const
{
    return d_func()->externalHostname;
}


int
Servent::additionalPort() const
{
    return d_func()->externalPort;
}


bool
equalByIPv6Address( QHostAddress a1, QHostAddress a2 )
{
    Q_IPV6ADDR addr1 = a1.toIPv6Address();
    Q_IPV6ADDR addr2 = a2.toIPv6Address();

    for (int i = 0; i < 16; ++i)
    {
        if ( addr1[i] != addr2[i] )
            return false;
    }
    return true;
}


// return the appropriate connection for a given offer key, or NULL if invalid
Connection*
Servent::claimOffer( ControlConnection* cc, const QString &nodeid, const QString &key, const QHostAddress peer )
{
    Q_D( Servent );

    // magic key for stream connections:
    if ( key.startsWith( "FILE_REQUEST_KEY:" ) )
    {
        // check if the source IP matches an existing, authenticated connection
        if ( !d->noAuth && peer != QHostAddress::Any && !isIPWhitelisted( peer ) )
        {
            bool authed = false;
            tLog( LOGVERBOSE ) << Q_FUNC_INFO << "Checking for ControlConnection with IP" << peer;
            QMutexLocker locker( &d->controlconnectionsMutex );
            foreach ( ControlConnection* cc, d->controlconnections )
            {
                if ( cc->socket() )
                    tLog( LOGVERBOSE ) << Q_FUNC_INFO << "Probing:" << cc->name() << cc->socket()->peerAddress();
                else
                    tLog( LOGVERBOSE ) << Q_FUNC_INFO << "Probing error:" << cc->name() << "has invalid socket";

                // Always compare IPv6 addresses as IPv4 address are sometime simply IPv4 addresses, sometimes mapped IPv6 addresses
                if ( cc->socket() && equalByIPv6Address( cc->socket()->peerAddress(), peer ) )
                {
                    authed = true;
                    break;
                }
            }
            if ( !authed )
            {
                tLog() << "File transfer request rejected as the request came from an IP which we could not match to existing peer connections.";
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

    if ( d->lazyoffers.contains( key ) )
    {
        ControlConnection* conn = new ControlConnection( this );
        conn->setName( d->lazyoffers.value( key ).first->contactId() );
        conn->addPeerInfo( d->lazyoffers.value( key ).first );
        conn->setId( d->lazyoffers.value( key ).second );

        if ( !nodeid.isEmpty() )
        {
            // Used by the connection for the ACL check
            // If there isn't a nodeid it's not the first connection and will already have been stopped
            conn->setNodeId( nodeid );
        }

        return conn;
    }
    else if ( d->offers.contains( key ) )
    {
        QPointer<Connection> conn = d->offers.value( key );
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
            conn->setNodeId( nodeid );
        }

        if ( conn.data()->onceOnly() )
        {
            d->offers.remove( key );
            return conn.data();
        }
        else
        {
            return conn.data()->clone();
        }
    }
    else if ( d->noAuth )
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
Servent::remoteIODeviceFactory( const Tomahawk::result_ptr& result, const QString& url,
                                std::function< void ( const QString&, QSharedPointer< QIODevice >& ) > callback )
{
    QSharedPointer<QIODevice> sp;

    QStringList parts = url.mid( QString( "servent://" ).length() ).split( "\t" );
    const QString sourceName = parts.at( 0 );
    const QString fileId = parts.at( 1 );
    source_ptr s = SourceList::instance()->get( sourceName );
    if ( s.isNull() || !s->controlConnection() )
    {
        callback( result->url(), sp );
        return;
    }

    ControlConnection* cc = s->controlConnection();
    StreamConnection* sc = new StreamConnection( this, cc, fileId, result );
    createParallelConnection( cc, sc, QString( "FILE_REQUEST_KEY:%1" ).arg( fileId ) );

    // std::functions cannot accept temporaries as parameters
    sp = sc->iodevice();
    callback( result->url(), sp );
}


void
Servent::registerStreamConnection( StreamConnection* sc )
{
    Q_ASSERT( !d_func()->scsessions.contains( sc ) );
    tDebug( LOGVERBOSE ) << "Registering Stream" << d_func()->scsessions.length() + 1;

    QMutexLocker lock( &d_func()->ftsession_mut );
    d_func()->scsessions.append( sc );

    printCurrentTransfers();
    emit streamStarted( sc );
}


void
Servent::onStreamFinished( StreamConnection* sc )
{
    Q_ASSERT( sc );
    tDebug( LOGVERBOSE ) << "Stream Finished, unregistering" << sc->id();

    QMutexLocker lock( &d_func()->ftsession_mut );
    d_func()->scsessions.removeAll( sc );

    printCurrentTransfers();
    emit streamFinished( sc );
}


// used for debug output:
void
Servent::printCurrentTransfers()
{
    int k = 1;
//    qDebug() << "~~~ Active file transfer connections:" << m_scsessions.length();
    foreach ( StreamConnection* i, d_func()->scsessions )
    {
        qDebug() << k << ") " << i->id();
    }
    qDebug() << endl;
}


void
Servent::cleanAddresses( QList<QHostAddress>& addresses ) const
{
    QList<QHostAddress>::iterator iter = addresses.begin();
    while ( iter != addresses.end() )
    {
        QString hostString = iter->toString();
        if ( hostString.startsWith( QLatin1String( "127.0.0." ) ) //< IPv4 localhost
             // IPv6 localhost
             || hostString == "::1"
             // IPv4 localhost as IPv6 address
             || hostString == "::7F00:1" )
        {
            iter = addresses.erase( iter );
            // Always continue if we changed iter as we might have reached the end
            continue;
        }

        // Remove IPv6 link local addresses
        if ( iter->isInSubnet( QHostAddress::parseSubnet( "fe80::/10" ) ) )
        {
            iter = addresses.erase( iter );
            continue;
        }

        // Advance to next element
        ++iter;
    }
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

    // Qt cannot cope correctly with IPv4 addresses mapped into the IPv6
    // address space
    if ( ip.protocol() == QAbstractSocket::IPv6Protocol )
    {
        Q_IPV6ADDR ipv6 = ip.toIPv6Address();
        // Check if it is actually an IPv4 address
        // First 80 bits are zero, then 16 bits 1s
        bool isIPv4 = true;
        for ( int i = 0; i < 9; i++) {
            isIPv4 &= ( ipv6[i] == 0 );
        }
        isIPv4 &= ( ipv6[10] == 0xff );
        isIPv4 &= ( ipv6[11] == 0xff );

        if ( isIPv4 )
        {
            // Convert to a real IPv4 address and rerun checks
            quint32 ipv4 = (static_cast<quint32>(ipv6[12]) << 24)
                    | (static_cast<quint32>(ipv6[13]) << 16)
                    | (static_cast<quint32>(ipv6[14]) << 8)
                    | static_cast<quint32>(ipv6[15]);
            QHostAddress addr( ipv4 );
            return isIPWhitelisted( addr );
        }
    }

    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "failure";
    return false;
}


bool
Servent::connectedToSession( const QString& session )
{
    Q_D( Servent );

    QMutexLocker locker( &d->controlconnectionsMutex );
    foreach ( ControlConnection* cc, d_func()->controlconnections )
    {
        Q_ASSERT( cc );

        if ( cc->id() == session )
            return true;
    }

    return false;
}


unsigned int
Servent::numConnectedPeers() const
{
    return d_func()->controlconnections.length();
}


QList<StreamConnection*>
Servent::streams() const
{
    return d_func()->scsessions;
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


bool
Servent::isReady() const
{
    return d_func()->ready;
}
