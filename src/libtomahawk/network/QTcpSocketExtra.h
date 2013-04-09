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

#ifndef QTCPSOCKETEXTRA_H
#define QTCPSOCKETEXTRA_H

// time before new connection terminates if no auth received
#define AUTH_TIMEOUT 180000

#include <QPointer>
#include <QString>
#include <QTcpSocket>
#include <QTimer>

#include "Msg.h"
#include "DllMacro.h"

class Connection;

// this is used to hold a bit of state, so when a connected signal is emitted
// from a socket, we can associate it with a Connection object etc.
// In addition, functionality to limit the connection timeout is implemented.
class DLLEXPORT QTcpSocketExtra : public QTcpSocket
{
Q_OBJECT

public:
    QTcpSocketExtra() : QTcpSocket(), m_connectTimeout( -1 )
    {
        QTimer::singleShot( AUTH_TIMEOUT, this, SLOT( authTimeout() ) ) ;
        m_connectTimer = new QTimer( this );
        connect( m_connectTimer, SIGNAL( timeout() ), this, SLOT( connectTimeout() ) );
    }

    void connectToHost(const QString& host, quint16 port, OpenMode openMode = ReadWrite );
    void connectToHost( const QHostAddress& host, quint16 port, OpenMode openMode = ReadWrite );

    QPointer<Connection> _conn;
    bool _outbound;
    bool _disowned;
    msg_ptr _msg;

    /**
     * Set a time limit for establishing a connection.
     */
    void setConnectTimeout( qint32 timeout ) { m_connectTimeout = timeout; }

    /**
     * Get the current timeout for establishing a connection.
     */
    qint32 connectTimeout() const { return m_connectTimeout; }

private slots:
    void connectTimeout();
    void authTimeout()
    {
      if( _disowned )
          return;

      qDebug() << "Connection timed out before providing a valid offer-key";
      this->disconnectFromHost();
    }
private:
    /**
     * How long we will wait for a connection to establish
     */
    qint32 m_connectTimeout;

    /**
     * Timer to measure the connection initialisation
     */
    QTimer* m_connectTimer;
};

#endif // QTCPSOCKETEXTRA_H
