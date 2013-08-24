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

#ifndef PLAYDARAPI_P_H
#define PLAYDARAPI_P_H

#include "PlaydarApi.h"

#include "Api_v1.h"

class PlaydarApiPrivate
{
public:
    PlaydarApiPrivate( PlaydarApi* q )
        : q_ptr( q )
    {
    }

    PlaydarApi* q_ptr;
    Q_DECLARE_PUBLIC( PlaydarApi )

private:
    QScopedPointer< Api_v1 > instance;
    QScopedPointer< QxtHttpServerConnector > connector;
    QScopedPointer< QxtHttpSessionManager > session;
    QHostAddress ha;
    qint16 port;
};

#endif // PLAYDARAPI_P_H
