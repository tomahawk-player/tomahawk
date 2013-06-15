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

#ifndef CONNECTION_P_H
#define CONNECTION_P_H

#include "Connection.h"

#include "MsgProcessor.h"

#include <QTime>
#include <QTimer>

class ConnectionPrivate
{
public:
    ConnectionPrivate( Connection* q )
        : q_ptr ( q )
        , do_shutdown( false )
        , actually_shutting_down( false )
        , peer_disconnected( false )
        , tx_bytes( 0 )
        , tx_bytes_requested( 0 )
        , rx_bytes( 0 )
        , id( "Connection()" )
        , statstimer( 0 )
        , stats_tx_bytes_per_sec( 0 )
        , stats_rx_bytes_per_sec( 0 )
        , rx_bytes_last( 0 )
        , tx_bytes_last( 0 )
    {
    }
    Connection* q_ptr;
    Q_DECLARE_PUBLIC ( Connection )

private:
    QHostAddress peerIpAddress;
    bool do_shutdown;
    bool actually_shutting_down;
    bool peer_disconnected;
    qint64 tx_bytes;
    qint64 tx_bytes_requested;
    qint64 rx_bytes;
    QString id;
    QString nodeid;

    QTimer* statstimer;
    QTime statstimer_mark;
    qint64 stats_tx_bytes_per_sec;
    qint64 stats_rx_bytes_per_sec;
    qint64 rx_bytes_last;
    qint64 tx_bytes_last;

    MsgProcessor msgprocessor_in;
    MsgProcessor msgprocessor_out;
};


#endif // CONNECTION_P_H
