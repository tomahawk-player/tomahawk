/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright 2013,      Uwe L. Korn <uwelk@xhochy.com>
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

#ifndef CONNECTION_H
#define CONNECTION_H

#include "Typedefs.h"
#include "DllMacro.h"

#include <QHostAddress>
#include <QPointer>
#include <QString>
#include <QTcpSocket>
#include <QVariant>

class ConnectionPrivate;
class Servent;

class DLLEXPORT Connection : public QObject
{
Q_OBJECT

public:

    Connection( Servent* parent );
    virtual ~Connection();
    virtual Connection* clone() = 0;

    QString id() const;
    void setId( const QString& );

    QString nodeId() const;
    void setNodeId( const QString& );

    void setFirstMessage( const QVariant& m );
    void setFirstMessage( msg_ptr m );
    msg_ptr firstMessage() const;

    const QPointer<QTcpSocket>& socket() const;

    void setOutbound( bool o );
    bool outbound() const;

    Servent* servent() const;

    /**
     * Get public port of remote peer.
     */
    int peerPort() const;
    void setPeerPort( int p );

    void markAsFailed();

    void setName( const QString& n );
    QString name() const;

    void setOnceOnly( bool b );
    bool onceOnly() const;

    bool isReady() const;
    bool isRunning() const;

    qint64 bytesSent() const;
    qint64 bytesReceived() const;

    void setMsgProcessorModeOut( quint32 m );
    void setMsgProcessorModeIn( quint32 m );

    const QHostAddress peerIpAddress() const;

    QString bareName() const;
signals:
    /**
     * Emitted if the authentication of this connection finally failed.
     */
    void authFailed();

    /**
     * Emitted if this connection was authenticated so that we can start talking to the other peer.
     */
    void authSuccessful();

    /**
     * Emiited if the authentication could not be processed in a given timespan.
     *
     * Though this does implicate a permanent problem in establishing a connection to the other peer,
     * this connection will be shutdown. In most cases this signal is emitted because we are waiting
     * for a user decision which has not yet be given.
     */
    void authTimeout();
    void ready();
    void failed();
    void finished();
    void statsTick( qint64 tx_bytes_sec, qint64 rx_bytes_sec );
    void socketClosed();
    void socketErrored( QAbstractSocket::SocketError );

protected:
    virtual void setup() = 0;

protected slots:
    virtual void handleMsg( msg_ptr msg ) = 0;
    virtual void authCheckTimeout();

public slots:
    virtual void start( QTcpSocket* sock );
    void sendMsg( QVariant );
    void sendMsg( msg_ptr );

    void shutdown( bool waitUntilSentAll = false );

private slots:
    void handleIncomingQueueEmpty();
    void sendMsg_now( msg_ptr );
    void socketDisconnected();
    void socketDisconnectedError( QAbstractSocket::SocketError );
    void readyRead();
    void doSetup();
    void checkACL();
    void aclDecision( Tomahawk::ACLStatus::Type status );
    void bytesWritten( qint64 );
    void calcStats();

private:
    Q_DECLARE_PRIVATE( Connection )
    ConnectionPrivate* d_ptr;

    void handleReadMsg();
    void actualShutdown();
};

#endif // CONNECTION_H
