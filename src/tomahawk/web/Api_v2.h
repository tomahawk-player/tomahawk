/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013,      Uwe L. Korn <uwelk@xhochy.com>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Tomahawk is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef API_V2_H
#define API_V2_H

#include "Api_v2_0.h"

#include <QObject>

#include <QxtWeb/QxtWebSlotService>

class Api_v2 : public QxtWebSlotService
{
    Q_OBJECT
public:
    Api_v2( QxtAbstractWebSessionManager* sm, QObject* parent = 0 );

    void sendJsonOk( QxtWebRequestEvent* event );
    void sendJsonError( QxtWebRequestEvent* event, const QString& message );
    void sendPlain404( QxtWebRequestEvent* event, const QString& message, const QString& statusmessage );

signals:
    
public slots:
    /**
     * All api (non-UI) calls with go to /api/<version>/method
     */
    void api( QxtWebRequestEvent* event, const QString& version, const QString& method, const QString& arg1 = QString(), const QString& arg2 = QString(), const QString& arg3 = QString() );

private:
    /**
     * Method call failed, report failure.
     */
    void apiCallFailed( QxtWebRequestEvent* event, const QString& method);
    Api_v2_0 m_apiv2_0;
};

#endif // API_V2_H
