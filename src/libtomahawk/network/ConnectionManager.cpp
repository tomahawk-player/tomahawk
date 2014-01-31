/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Uwe L. Korn <uwelk@xhochy.com>
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

#include "ConnectionManager_p.h"

#include "accounts/Account.h"
#include "database/Database.h"
#include "database/DatabaseImpl.h"
#include "network/Msg.h"
#include "sip/SipInfo.h"
#include "sip/SipPlugin.h"
#include "utils/Logger.h"
#include "utils/WeakObjectHash.h"

#include "ControlConnection.h"
#include "PlaylistEntry.h"
#include "QTcpSocketExtra.h"
#include "Servent.h"
#include "Source.h"

#include <boost/bind.hpp>
#include <qtconcurrentrun.h>

/* Management of ConnectionManagers */

static QMutex nodeMapMutex;
static Tomahawk::Utils::WeakObjectHash< ConnectionManager > connectionManagers;
static QHash< QString, QSharedPointer< ConnectionManager > > activeConnectionManagers;

QSharedPointer<ConnectionManager>
ConnectionManager::getManagerForNodeId( const QString &nodeid )
{
    QMutexLocker locker( &nodeMapMutex );
    if ( connectionManagers.hash().contains( nodeid ) && !connectionManagers.hash().value( nodeid ).isNull() ) {
        return connectionManagers.hash().value( nodeid ).toStrongRef();
    }

    // There exists no connection for this nodeid
    QSharedPointer< ConnectionManager > manager( new ConnectionManager( nodeid ) );
    manager->setWeakRef( manager.toWeakRef() );
    connectionManagers.insert( nodeid, manager );
    return manager;
}

void
ConnectionManager::setActive( bool active, const QString& nodeid, const QSharedPointer<ConnectionManager>& manager )
{
    QMutexLocker locker( &nodeMapMutex );
    if ( active )
    {
        activeConnectionManagers[ nodeid ] = manager;
    }
    else
    {
        activeConnectionManagers.remove( nodeid );
    }
}

/* ConnectionManager Implementation */

ConnectionManager::ConnectionManager( const QString &nodeid )
    : d_ptr( new ConnectionManagerPrivate( this, nodeid ) )
{
    // TODO sth?
}

ConnectionManager::~ConnectionManager()
{
    delete d_ptr;
}

void
ConnectionManager::handleSipInfo( const Tomahawk::peerinfo_ptr &peerInfo )
{
    // Start handling in a separate thread so that we do not block the event loop.
    QtConcurrent::run( &ConnectionManager::handleSipInfoPrivateS, peerInfo, weakRef().toStrongRef() );
}

QWeakPointer<ConnectionManager>
ConnectionManager::weakRef() const
{
    return d_func()->ownRef;
}

void
ConnectionManager::setWeakRef( QWeakPointer<ConnectionManager> weakRef )
{
    d_func()->ownRef = weakRef;
}


void
ConnectionManager::authSuccessful()
{
    Q_D( ConnectionManager );

    // We have successfully connected to the other peer, we're done.
    disconnect( d->controlConnection.data(), SIGNAL( authSuccessful() ), this, SLOT( authSuccessful() ) );
    disconnect( d->controlConnection.data(), SIGNAL( authFailed() ), this , SLOT( authFailed() ) );
    disconnect( d->controlConnection.data(), SIGNAL( authTimeout() ), this, SLOT( authFailed() ) );

    d->currentPeerInfo.clear();
    deactivate();
}


void
ConnectionManager::authFailed()
{
    Q_D( ConnectionManager );

    // We could not auth with the peer on the other side, maybe this is the wrong one?
    disconnect( d->controlConnection.data(), SIGNAL( authSuccessful() ), this, SLOT( authSuccessful() ) );
    disconnect( d->controlConnection.data(), SIGNAL( authFailed() ), this, SLOT( authFailed() ) );
    disconnect( d->controlConnection.data(), SIGNAL( authTimeout() ), this, SLOT( authFailed() ) );

    peerInfoDebug( d->currentPeerInfo ) << Q_FUNC_INFO << "Connection authentication failed";

    // Only retry if we have any retries left.
    if (!d->currentPeerInfo->sipInfos().isEmpty()) {
      // If auth failed, we need to setup a new controlconnection as the old will be destroyed.
      newControlConnection( d->currentPeerInfo );
      // Try to connect with the next available SipInfo.
      tryConnect();
    }
}


void
ConnectionManager::handleSipInfoPrivate( const Tomahawk::peerinfo_ptr &peerInfo )
{
    activate();
    // Respect different behaviour before 0.7.100
    peerInfoDebug( peerInfo ) << Q_FUNC_INFO << "Trying to connect to client with version " << peerInfo->versionString().split(' ').last() << TomahawkUtils::compareVersionStrings( peerInfo->versionString().split(' ').last(), "0.7.99" );
    if ( !peerInfo->versionString().isEmpty() && TomahawkUtils::compareVersionStrings( peerInfo->versionString().split(' ').last(), "0.7.100" ) < 0)
    {
        peerInfoDebug( peerInfo ) << Q_FUNC_INFO << "Using old-style (<0.7.100) connection order.";
        SipInfo we = Servent::getSipInfoForOldVersions( Servent::instance()->getLocalSipInfos( QString( "default" ), QString( "default" ) ) );
        SipInfo they = peerInfo->sipInfos().first();
        if ( they.isVisible() )
        {
            if ( !we.isVisible() || we.host() < they.host() || (we.host() == they.host() && we.port() < they.port()))
            {
                tLog( LOGVERBOSE ) << Q_FUNC_INFO << "Initiate connection to" << peerInfo->id() << "at" << they.host() << "peer of:" << peerInfo->sipPlugin()->account()->accountFriendlyName();
                connectToPeer( peerInfo, false );
                return;
                // We connected to the peer, so we are done here.
            }
        }
        deactivate();
        return;
    }
    foreach ( SipInfo info, peerInfo->sipInfos() )
    {
        if (info.isVisible())
        {
            // There is at least one SipInfo that may be visible. Try connecting.
            // Duplicate Connections are checked by connectToPeer, so we do not need to take care of this
            tLog( LOGVERBOSE ) << Q_FUNC_INFO << "Initiate connection to" << peerInfo->id() << "at" << info.host() << "peer of:" << peerInfo->sipPlugin()->account()->accountFriendlyName();
            connectToPeer( peerInfo, false );
            // We connected to the peer, so we are done here.
            return;
        }
    }
    deactivate();
}


void
ConnectionManager::newControlConnection( const Tomahawk::peerinfo_ptr& peerInfo )
{
    Q_D( ConnectionManager );
    QVariantMap m;
    m["conntype"]  = "accept-offer";
    m["key"]       = peerInfo->key();
    m["nodeid"]    = Tomahawk::Database::instance()->impl()->dbid();

    d->controlConnection = QPointer<ControlConnection>( new ControlConnection( Servent::instance() ) );
    d->controlConnection->setShutdownOnEmptyPeerInfos( false );
    d->controlConnection->addPeerInfo( peerInfo );
    d->controlConnection->setFirstMessage( m );

    if ( peerInfo->id().length() )
        d->controlConnection->setName( peerInfo->contactId() );
    if ( peerInfo->nodeId().length() )
        d->controlConnection->setId( peerInfo->nodeId() );

    d->controlConnection->setNodeId( peerInfo->nodeId() );

    Servent::instance()->registerControlConnection( d->controlConnection.data() );
}


void
ConnectionManager::connectToPeer( const Tomahawk::peerinfo_ptr &peerInfo, bool lock )
{
    // Lock, so that we will not attempt to do two parallell connects.
    if (lock)
    {
        activate();
    }
    // Check that we are not already connected to this peer
    ControlConnection* cconn = Servent::instance()->lookupControlConnection( peerInfo->nodeId() );
    if ( cconn != NULL || !d_func()->controlConnection.isNull() )
    {
        // We are already connected to this peer, so just add some more details.
        peerInfoDebug( peerInfo ) << "Existing connection found, not connecting.";
        cconn->addPeerInfo( peerInfo );
        if ( cconn != NULL )
        {
            d_func()->controlConnection = QPointer<ControlConnection>(cconn);
        }
        deactivate();
        return;
        // TODO: Keep the peerInfo in mind for reconnecting
        // FIXME: Do we need this for reconnecting if the connection drops?
    }

    // If we are not connected, try to connect
    d_func()->currentPeerInfo = peerInfo;
    peerInfoDebug( peerInfo ) << "No existing connection found, trying to connect.";
    // Sort SipInfos
    QList< SipInfo > anyOther;
    QList< SipInfo > publicIPv4;
    QList< SipInfo > publicIPv6;
    QList< SipInfo > privateIPv4;
    QList< SipInfo > privateIPv6;
    foreach ( SipInfo sipInfo, peerInfo->sipInfos() )
    {
        if ( !sipInfo.isVisible() )
        {
            continue;
        }

        QHostAddress ha;
        if ( ha.setAddress( sipInfo.host() ) )
        {
            if ( Servent::isValidExternalIP( ha ) )
            {
                if ( ha.protocol() == QAbstractSocket::IPv6Protocol )
                {
                    publicIPv6.append( sipInfo );
                }
                else
                {
                    publicIPv4.append( sipInfo );
                }
            }
            else
            {
                if ( ha.protocol() == QAbstractSocket::IPv6Protocol )
                {
                    privateIPv6.append( sipInfo );
                }
                else
                {
                    privateIPv4.append( sipInfo );
                }
            }
        }
        else
        {
            anyOther.append( sipInfo );
        }

    }
    if ( Servent::instance()->ipv6ConnectivityLikely() && !publicIPv6.isEmpty() )
    {
        // Prefer IPv6 over IPv4
        d_func()->sipCandidates.append( anyOther );
        d_func()->sipCandidates.append( publicIPv6 );
        d_func()->sipCandidates.append( publicIPv4 );
        d_func()->sipCandidates.append( privateIPv6 );
        d_func()->sipCandidates.append( privateIPv4 );
    }
    else
    {
        // First try all IPv4 before trying IPv6
        d_func()->sipCandidates.append( anyOther );
        d_func()->sipCandidates.append( publicIPv4 );
        d_func()->sipCandidates.append( privateIPv4 );
        d_func()->sipCandidates.append( publicIPv6 );
        d_func()->sipCandidates.append( privateIPv6 );
    }

    newControlConnection( peerInfo );
    tryConnect();
}

void ConnectionManager::tryConnect()
{
    // ATTENTION: mutex should be already locked by the calling function.
    Q_ASSERT( !d_func()->controlConnection.isNull() );

    if ( d_func()->sipCandidates.isEmpty() )
    {
        // No more possibilities to connect.
        peerInfoDebug( d_func()->currentPeerInfo ) << Q_FUNC_INFO << "No more possible SIP endpoints for " << d_func()->controlConnection->name() << " skipping.";

        // Clean up.
        d_func()->currentPeerInfo.clear();
        delete d_func()->controlConnection.data();
        deactivate();
        return;
    }

    // Use first available SIP endpoint and remove it from the list
    SipInfo info = d_func()->sipCandidates.takeFirst();
    if ( !info.isVisible() )
    {
        peerInfoDebug( d_func()->currentPeerInfo ) << Q_FUNC_INFO << "Try next SipInfo, we can't connect to this one";
        tryConnect();
        return;
    }

    peerInfoDebug( d_func()->currentPeerInfo ) << Q_FUNC_INFO << "Connecting to " << info.host() << ":" << info.port();
    Q_ASSERT( info.port() > 0 );

    // Check that we are not connecting to ourselves
    foreach( QHostAddress ha, Servent::instance()->addresses() )
    {
        if ( info.host() == ha.toString() && info.port() == Servent::instance()->port() )
        {
            peerInfoDebug( d_func()->currentPeerInfo ) << Q_FUNC_INFO << "Tomahawk won't try to connect to" << info.host() << ":" << info.port() << ": same ip:port as ourselves.";
            tryConnect();
            return;
        }
    }
    if ( info.host() == Servent::instance()->additionalAddress() && info.port() == Servent::instance()->additionalPort() )
    {
        peerInfoDebug( d_func()->currentPeerInfo ) << Q_FUNC_INFO << "Tomahawk won't try to connect to" << info.host() << ":" << info.port() << ": same ip:port as ourselves.";
        tryConnect();
        return;
    }

    // We should have already setup a first message in connectToPeer
    Q_ASSERT( !d_func()->controlConnection->firstMessage().isNull() );

    QTcpSocketExtra* sock = new QTcpSocketExtra();
    sock->setConnectTimeout( CONNECT_TIMEOUT );
    sock->_disowned = false;
    sock->_conn = d_func()->controlConnection.data();
    sock->_outbound = true;

    connect( sock, SIGNAL( connected() ), this, SLOT( socketConnected() ) );
    connect( sock, SIGNAL( error( QAbstractSocket::SocketError ) ), this, SLOT( socketError( QAbstractSocket::SocketError ) ) );

    peerInfoDebug( d_func()->currentPeerInfo ) << Q_FUNC_INFO << "Connecting socket to " << info.host() << ":" << info.port();
    sock->connectToHost( info.host(), info.port(), QTcpSocket::ReadWrite );
    sock->moveToThread( Servent::instance()->thread() );
}

void
ConnectionManager::socketError( QAbstractSocket::SocketError error )
{
    Q_UNUSED( error );
    Q_ASSERT( !d_func()->controlConnection.isNull() );

    QTcpSocketExtra* sock = (QTcpSocketExtra*)sender();
    peerInfoDebug( d_func()->currentPeerInfo ) << Q_FUNC_INFO << "Connecting to " << sock->peerAddress().toString() << " failed: " << sock->errorString();
    sock->deleteLater();

    // Try to connect with the next available SipInfo.
    tryConnect();
}

void
ConnectionManager::handleSipInfoPrivateS( const Tomahawk::peerinfo_ptr &peerInfo, const QSharedPointer<ConnectionManager> &connectionManager )
{
    connectionManager->handleSipInfoPrivate( peerInfo );
}

void
ConnectionManager::activate()
{
    d_func()->mutex.lock();
    setActive( true, d_func()->nodeid, weakRef().toStrongRef() );
}

void
ConnectionManager::deactivate()
{
    QSharedPointer<ConnectionManager> strongRef = weakRef().toStrongRef();
    setActive( false, d_func()->nodeid, strongRef );
    strongRef->d_func()->mutex.unlock();
}


void
ConnectionManager::socketConnected()
{
    Q_D( ConnectionManager );
    QTcpSocketExtra* sock = (QTcpSocketExtra*)sender();

    peerInfoDebug( d->currentPeerInfo ) << Q_FUNC_INFO << "Connected to hostaddr: " << sock->peerAddress() << ", hostname:" << sock->peerName();

    Q_ASSERT( !sock->_conn.isNull() );

    handoverSocket( sock );

    // Connect signal to wait for authentication result.
    connect( d->controlConnection.data(), SIGNAL( authSuccessful() ), SLOT( authSuccessful() ) );
    connect( d->controlConnection.data(), SIGNAL( authFailed()) , SLOT( authFailed() ) );
    connect( d->controlConnection.data(), SIGNAL( authTimeout() ), SLOT( authFailed() ) );
}

void
ConnectionManager::handoverSocket( QTcpSocketExtra* sock )
{
    Q_ASSERT( !d_func()->controlConnection.isNull() );
    Q_ASSERT( sock );
    Q_ASSERT( d_func()->controlConnection->socket().isNull() );
    Q_ASSERT( sock->isValid() );

    disconnect( sock, SIGNAL( disconnected() ), sock, SLOT( deleteLater() ) );
    disconnect( sock, SIGNAL( error( QAbstractSocket::SocketError ) ), this, SLOT( socketError( QAbstractSocket::SocketError ) ) );

    sock->_disowned = true;
    d_func()->controlConnection->setOutbound( sock->_outbound );
    d_func()->controlConnection->setPeerPort( sock->peerPort() );

    QMetaObject::invokeMethod( d_func()->controlConnection, "start", Qt::QueuedConnection, Q_ARG( QTcpSocket*, sock ) );
    // ControlConntection is now connected, now it can be destroyed if the PeerInfos disappear
    d_func()->controlConnection->setShutdownOnEmptyPeerInfos( true );
}
