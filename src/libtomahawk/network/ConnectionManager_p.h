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

#ifndef CONNECTIONMANAGER_P_H
#define CONNECTIONMANAGER_P_H

#include "ConnectionManager.h"

#include <QMutex>

class ConnectionManagerPrivate
{
public:
    ConnectionManagerPrivate( ConnectionManager* q, const QString& _nodeid )
        : q_ptr ( q )
        , nodeid( _nodeid )
    {
    }
    ConnectionManager* q_ptr;
    Q_DECLARE_PUBLIC ( ConnectionManager )

private:
    // We just keep this for debug purposes and only during connection attempts.
    Tomahawk::peerinfo_ptr currentPeerInfo;
    QString nodeid;
    QPointer<ControlConnection> controlConnection;
    QList<SipInfo> sipCandidates;
    QMutex mutex;
    QWeakPointer< ConnectionManager > ownRef;
};

#endif // CONNECTIONMANAGER_P_H
