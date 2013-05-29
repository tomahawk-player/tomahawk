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
#include <QMutex>

class QTcpSocketExtra;

class DLLEXPORT ConnectionManager : public QObject
{
    Q_OBJECT

public:
    static QSharedPointer<ConnectionManager> getManagerForNodeId( const QString& nodeid );
    ConnectionManager( const QString& nodeid );

    void handleSipInfo( const Tomahawk::peerinfo_ptr& peerInfo );

private slots:
    void socketConnected();
    void socketError( QAbstractSocket::SocketError error );

private:
    void connectToPeer(const Tomahawk::peerinfo_ptr& peerInfo , bool lock);
    void handleSipInfoPrivate( const Tomahawk::peerinfo_ptr& peerInfo );

    /**
     * Transfers ownership of socket to the ControlConnection and inits the ControlConnection
     */
    void handoverSocket( QTcpSocketExtra* sock );

    /**
     * Attempt to connect to the peer using the current stored information.
     */
    void tryConnect();

    // We just keep this for debug purposes and only during connection attempts.
    Tomahawk::peerinfo_ptr m_currentPeerInfo;
    QString m_nodeid;
    QPointer<ControlConnection> m_controlConnection;
    QList<SipInfo> m_sipCandidates;
    QMutex m_mutex;
};

#endif // CONNECTIONMANAGER_H
