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

#ifndef CONNECTIONMANAGER_H
#define CONNECTIONMANAGER_H

#include "DllMacro.h"

#include "sip/PeerInfo.h"

#include <QAbstractSocket>
#include <QObject>

class ConnectionManagerPrivate;
class QTcpSocketExtra;

/**
 * Handle all (outgoing) connection attempts to a specific nodeid.
 */
class DLLEXPORT ConnectionManager : public QObject
{
    Q_OBJECT

public:
    /**
     * Get the ConnectionManager responsible for the given nodeid, if there is none yet, create a new instance.
     */
    static QSharedPointer<ConnectionManager> getManagerForNodeId( const QString& nodeid );

    /**
     * Activate/Deactivate the ConnectionManager for a specific nodeid.
     *
     * A strong reference is held for every active ConnectionManagers so that they are not automatically deleted while in operation.
     */
    static void setActive( bool active, const QString& nodeid, const QSharedPointer<ConnectionManager>& manager );

    virtual ~ConnectionManager();

    /**
     * Receive incoming SipInfos and start a new thread to connect to this peer.
     */
    void handleSipInfo( const Tomahawk::peerinfo_ptr& peerInfo );

    QWeakPointer< ConnectionManager > weakRef() const;
    void setWeakRef( QWeakPointer< ConnectionManager > weakRef );

private slots:
    void authSuccessful();
    void authFailed();
    void socketConnected();
    void socketError( QAbstractSocket::SocketError error );

private:
    Q_DECLARE_PRIVATE( ConnectionManager )
    ConnectionManagerPrivate* d_ptr;

    /**
     * Create a new ConnectionManager.
     *
     * This should only be done internally so that we do not have more than one ConnectionManager for a nodeid.
     */
    ConnectionManager( const QString& nodeid );

    /**
     * Create a new ControlConnection for talking to a peer.
     */
    void newControlConnection(const Tomahawk::peerinfo_ptr &peerInfo);

    /**
     * Proxy handleSipInfoPrivate to hand over a strong reference to the connectionManager
     * so that the refcount is >0 while transferring the context of operation to another thread.
     */
    static void handleSipInfoPrivateS( const Tomahawk::peerinfo_ptr& peerInfo, const QSharedPointer<ConnectionManager>& connectionManager );

    /**
     * Acquire the object lock and register this ConnectionManager as active.
     */
    void activate();

    /**
     * Release the object lock and register this ConnectionManager as inactive.
     */
    void deactivate();

    /**
     * Try to connect to a peer with a given SipInfo.
     */
    void connectToPeer(const Tomahawk::peerinfo_ptr& peerInfo , bool lock);

    /**
     * Look for existing connections and try to connect if there is none.
     */
    void handleSipInfoPrivate( const Tomahawk::peerinfo_ptr& peerInfo );

    /**
     * Transfers ownership of socket to the ControlConnection and inits the ControlConnection
     */
    void handoverSocket( QTcpSocketExtra* sock );

    /**
     * Attempt to connect to the peer using the current stored information.
     */
    void tryConnect();
};

#endif // CONNECTIONMANAGER_H
