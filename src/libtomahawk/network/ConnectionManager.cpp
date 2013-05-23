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

#include "ConnectionManager.h"
#include "ControlConnection.h"
#include "Servent.h"

#include "database/Database.h"
#include "database/DatabaseImpl.h"
#include "sip/SipPlugin.h"
#include "utils/Logger.h"

#include <boost/bind.hpp>
#include <qtconcurrentrun.h>

ConnectionManager::ConnectionManager( const QString &nodeid )
    : m_nodeid( nodeid )
{
    // TODO sth?
}

void
ConnectionManager::handleSipInfo( const Tomahawk::peerinfo_ptr &peerInfo )
{
    // Start handling in a separate thread so that we do not block the event loop.
    QtConcurrent::run( this, &ConnectionManager::handleSipInfoPrivate, peerInfo );
}

void
ConnectionManager::handleSipInfoPrivate( const Tomahawk::peerinfo_ptr &peerInfo )
{
    m_mutex.lock();
    // Respect different behaviour before 0.7.99
    peerInfoDebug( peerInfo ) << Q_FUNC_INFO << "Trying to connect to client with version " << peerInfo->versionString().split(' ').last() << TomahawkUtils::compareVersionStrings( peerInfo->versionString().split(' ').last(), "0.7.99" );
    if ( !peerInfo->versionString().isEmpty() && TomahawkUtils::compareVersionStrings( peerInfo->versionString().split(' ').last(), "0.7.99" ) < 0)
    {
        peerInfoDebug( peerInfo ) << Q_FUNC_INFO << "Using old-style (<0.7.99) connection order.";
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
        m_mutex.unlock();
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
    m_mutex.unlock();
}

void
ConnectionManager::connectToPeer( const Tomahawk::peerinfo_ptr &peerInfo, bool lock )
{
    // Lock, so that we will not attempt to do two parallell connects.
    if (lock)
    {
        m_mutex.lock();
    }
    // Check that we are not already connected to this peer
    ControlConnection* cconn = Servent::instance()->lookupControlConnection( peerInfo->nodeId() );
    if ( cconn != NULL || !m_controlConnection.isNull() )
    {
        // We are already connected to this peer, so just add some more details.
        peerInfoDebug( peerInfo ) << "Existing connection found, not connecting.";
        cconn->addPeerInfo( peerInfo );
        if ( cconn != NULL )
        {
            m_controlConnection = QPointer<ControlConnection>(cconn);
        }
        m_mutex.unlock();
        return;
        // TODO: Keep the peerInfo in mind for reconnecting
        // FIXME: Do we need this for reconnecting if the connection drops?
    }

    // If we are not connected, try to connect
    m_currentPeerInfo = peerInfo;
    peerInfoDebug( peerInfo ) << "No existing connection found, trying to connect.";
    m_sipCandidates.append( peerInfo->sipInfos() );

    QVariantMap m;
    m["conntype"]  = "accept-offer";
    m["key"]       = peerInfo->key();
    m["nodeid"]    = Database::instance()->impl()->dbid();

    m_controlConnection = QPointer<ControlConnection>( new ControlConnection( Servent::instance() ) );
    m_controlConnection->addPeerInfo( peerInfo );
    m_controlConnection->setFirstMessage( m );

    if ( peerInfo->id().length() )
        m_controlConnection->setName( peerInfo->contactId() );
    if ( peerInfo->nodeId().length() )
        m_controlConnection->setId( peerInfo->nodeId() );

    m_controlConnection->setProperty( "nodeid", peerInfo->nodeId() );

    Servent::instance()->registerControlConnection( m_controlConnection.data() );
    tryConnect();
}

void ConnectionManager::tryConnect()
{
    // ATTENTION: mutex should be already locked by the calling function.
    Q_ASSERT( !m_controlConnection.isNull() );

    if ( m_sipCandidates.isEmpty() )
    {
        // No more possibilities to connect.
        peerInfoDebug( m_currentPeerInfo ) << Q_FUNC_INFO << "No more possible SIP endpoints for " << m_controlConnection->name() << " skipping.";

        // Clean up.
        m_currentPeerInfo.clear();
        delete m_controlConnection.data();
        m_mutex.unlock();
        return;
    }

    // Use first available SIP endpoint and remove it from the list
    SipInfo info = m_sipCandidates.takeFirst();
    if ( !info.isVisible() )
    {
        peerInfoDebug( m_currentPeerInfo ) << Q_FUNC_INFO << "Try next SipInfo, we can't connect to this one";
        tryConnect();
        return;
    }

    peerInfoDebug( m_currentPeerInfo ) << Q_FUNC_INFO << "Connecting to " << info.host() << ":" << info.port();
    Q_ASSERT( info.port() > 0 );

    // Check that we are not connecting to ourselves
    foreach( QHostAddress ha, Servent::instance()->addresses() )
    {
        if ( info.host() == ha.toString() && info.port() == Servent::instance()->port() )
        {
            peerInfoDebug( m_currentPeerInfo ) << Q_FUNC_INFO << "Tomahawk won't try to connect to" << info.host() << ":" << info.port() << ": same ip:port as ourselves.";
            tryConnect();
            return;
        }
    }
    if ( info.host() == Servent::instance()->additionalAddress() && info.port() == Servent::instance()->additionalPort() )
    {
        peerInfoDebug( m_currentPeerInfo ) << Q_FUNC_INFO << "Tomahawk won't try to connect to" << info.host() << ":" << info.port() << ": same ip:port as ourselves.";
        tryConnect();
        return;
    }

    // We should have already setup a first message in connectToPeer
    Q_ASSERT( !m_controlConnection->firstMessage().isNull() );

    QTcpSocketExtra* sock = new QTcpSocketExtra();
    sock->setConnectTimeout( CONNECT_TIMEOUT );
    sock->_disowned = false;
    sock->_conn = m_controlConnection.data();
    sock->_outbound = true;

    connect( sock, SIGNAL( connected() ), this, SLOT( socketConnected() ) );
    connect( sock, SIGNAL( error( QAbstractSocket::SocketError ) ), this, SLOT( socketError( QAbstractSocket::SocketError ) ) );

    peerInfoDebug( m_currentPeerInfo ) << Q_FUNC_INFO << "Connecting socket to " << info.host() << ":" << info.port();
    sock->connectToHost( info.host(), info.port(), QTcpSocket::ReadWrite );
    sock->moveToThread( thread() );
}

void
ConnectionManager::socketError( QAbstractSocket::SocketError error )
{
    Q_UNUSED( error );
    Q_ASSERT( !m_controlConnection.isNull() );

    QTcpSocketExtra* sock = (QTcpSocketExtra*)sender();
    peerInfoDebug( m_currentPeerInfo ) << Q_FUNC_INFO << "Connecting to " << sock->peerAddress().toString() << " failed: " << sock->errorString();
    sock->deleteLater();

    // Try to connect with the next available SipInfo.
    tryConnect();
}


void
ConnectionManager::socketConnected()
{
    QTcpSocketExtra* sock = (QTcpSocketExtra*)sender();

    peerInfoDebug( m_currentPeerInfo ) << Q_FUNC_INFO << "Connected to hostaddr: " << sock->peerAddress() << ", hostname:" << sock->peerName();

    Q_ASSERT( !sock->_conn.isNull() );

    handoverSocket( sock );
    m_currentPeerInfo.clear();
    m_mutex.unlock();
}

void
ConnectionManager::handoverSocket( QTcpSocketExtra* sock )
{
    Q_ASSERT( !m_controlConnection.isNull() );
    Q_ASSERT( sock );
    Q_ASSERT( m_controlConnection->socket().isNull() );
    Q_ASSERT( sock->isValid() );

    disconnect( sock, SIGNAL( disconnected() ), sock, SLOT( deleteLater() ) );
    disconnect( sock, SIGNAL( error( QAbstractSocket::SocketError ) ), this, SLOT( socketError( QAbstractSocket::SocketError ) ) );

    sock->_disowned = true;
    m_controlConnection->setOutbound( sock->_outbound );
    m_controlConnection->setPeerPort( sock->peerPort() );

    m_controlConnection->start( sock );
    m_currentPeerInfo.clear();
    m_mutex.unlock();
}


static QMutex* nodeMapMutex = new QMutex();
static QMap< QString, QSharedPointer< ConnectionManager > > connectionManagers;

QSharedPointer<ConnectionManager>
ConnectionManager::getManagerForNodeId( const QString &nodeid )
{
    QMutexLocker locker( nodeMapMutex );
    if ( connectionManagers.contains( nodeid ) && !connectionManagers.value( nodeid ).isNull() ) {
        return connectionManagers.value( nodeid );
    }

    // There exists no connection for this nodeid
    QSharedPointer< ConnectionManager > manager( new ConnectionManager( nodeid ) );
    connectionManagers[nodeid] = manager;
    return manager;
}
