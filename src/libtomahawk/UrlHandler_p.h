/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2014, Uwe L. Korn <uwelk@xhochy.com>
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

#ifndef URLHANDLER_P_H
#define URLHANDLER_P_H

#include "UrlHandler.h"

#include "utils/Logger.h"
#include "utils/NetworkReply.h"

class HttpIODeviceReadyHandler : public QObject
{
    Q_OBJECT

public:

    QSharedPointer<NetworkReply> reply;
    IODeviceCallback callback;

    HttpIODeviceReadyHandler( const QSharedPointer<NetworkReply>& _reply, IODeviceCallback _callback )
        : reply( _reply )
        , callback( _callback )
    {
        // Do Nothing
    }

public slots:

    void called()
    {
        tLog() << Q_FUNC_INFO << reply->reply();
        QSharedPointer< QNetworkReply > sp( reply->reply(), &QObject::deleteLater );
        reply->disconnectFromReply();
        QSharedPointer< QIODevice > spIO = sp.staticCast< QIODevice>();
        callback( sp->url().toString(), spIO );

        // Call once, then self-destruct
        deleteLater();
    }

};

#endif // URLHANDLER_P_H
