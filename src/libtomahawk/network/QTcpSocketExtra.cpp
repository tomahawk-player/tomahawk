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

#include "QTcpSocketExtra.h"

#include "utils/Logger.h"

void
QTcpSocketExtra::connectToHost( const QHostAddress& host, quint16 port, OpenMode openMode )
{
    if ( m_connectTimer->isActive() == true )
    {
        tLog() << Q_FUNC_INFO << "Connection already establishing.";
        return;
    }

    QTcpSocket::connectToHost( host, port, openMode);
    if ( m_connectTimeout > 0 )
        m_connectTimer->start( m_connectTimeout );
}

void
QTcpSocketExtra::connectToHost(const QString& host, quint16 port, OpenMode openMode)
{
    if ( m_connectTimer->isActive() == true )
    {
        tLog() << Q_FUNC_INFO << "Connection already establishing.";
        return;
    }

    QTcpSocket::connectToHost( host, port, openMode);
    if ( m_connectTimeout > 0 )
        m_connectTimer->start( m_connectTimeout );
}

void
QTcpSocketExtra::connectTimeout()
{
    m_connectTimer->stop();
    if ( state() != ConnectedState )
    {
        // We did not manage to connect in the given timespan, so abort the attempt...
        close();
        // .. and notify error handlers.
        emit error( SocketTimeoutError );
    }
}
