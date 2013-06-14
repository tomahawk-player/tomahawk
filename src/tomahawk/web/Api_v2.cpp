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

#include "Api_v2.h"

#include <QxtWeb/QxtWebPageEvent>

Api_v2::Api_v2( QxtAbstractWebSessionManager* sm, QObject* parent )
    : QxtWebSlotService( sm, parent )
    , m_apiv2_0( this )
{
}

void
Api_v2::api( QxtWebRequestEvent *event, const QString &version, const QString &method )
{
    if ( version == "2.0" )
    {
        if ( !QMetaObject::invokeMethod( &m_apiv2_0, method.toAscii().constData(), Q_ARG( QxtWebRequestEvent*, event ) ) )
        {
            // Method call failed, report failure
            sendPlain404( event, QString( "Method \"%1\" for API 2.0 not found" ).arg( method ), "Method in API 2.0 not found" );
        }

    }
    else
    {
        sendPlain404( event, QString( "Unknown API version %1" ).arg( version ), "API version not found" );
    }
}

void
Api_v2::sendPlain404( QxtWebRequestEvent *event, const QString &message, const QString &statusmessage )
{
    QxtWebPageEvent * e = new QxtWebPageEvent( event->sessionID, event->requestID, message.toUtf8() );
    e->contentType = "text/plain";
    e->status = 404;
    e->statusMessage = statusmessage.toAscii().constData();
    postEvent( e );
}
