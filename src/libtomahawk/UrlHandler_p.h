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

#include "utils/NetworkReply.h"

class HttpIODeviceReadyHandler : public QObject
{
    Q_OBJECT

public:

    NetworkReply* reply;
    IODeviceCallback callback;
    QWeakPointer<HttpIODeviceReadyHandler> ref;

    HttpIODeviceReadyHandler( NetworkReply* _reply, IODeviceCallback _callback )
        : reply( _reply )
        , callback( _callback )
    {
        // Do Nothing
    }

public slots:

    void called()
    {
        QSharedPointer< QIODevice > sp = QSharedPointer< QIODevice >( reply->reply(), &QObject::deleteLater );
        callback( sp );

        // Call once, then self-destruct
        deleteLater();
    }

};

#endif // URLHANDLER_P_H
