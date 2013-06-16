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

#ifndef CONTROLCONNECTION_P_H
#define CONTROLCONNECTION_P_H

#include "ControlConnection.h"

#include <QReadWriteLock>
#include <QTime>
#include <QTimer>

class ControlConnectionPrivate
{
public:
    ControlConnectionPrivate( ControlConnection* q )
        : q_ptr ( q )
        , dbsyncconn( 0 )
        , registered( false )
        , shutdownOnEmptyPeerInfos( true )
        , pingtimer( 0 )
    {
    }
    ControlConnection* q_ptr;
    Q_DECLARE_PUBLIC ( ControlConnection )

private:

    Tomahawk::source_ptr source;
    /**
     * Lock acces to the source member. A "write" access is only if we change the value of source, not if doing a non-const call.
     */
    mutable QReadWriteLock sourceLock;
    DBSyncConnection* dbsyncconn;

    QString dbconnkey;
    bool registered;
    bool shutdownOnEmptyPeerInfos;

    QTimer* pingtimer;
    QTime pingtimer_mark;

    QSet< Tomahawk::peerinfo_ptr > peerInfos;
};

#endif // CONTROLCONNECTION_P_H
