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

#include "Api_v2_0.h"

#include <QxtWeb/QxtWebPageEvent>
#include <QxtWeb/QxtWebSlotService>

Api_v2_0::Api_v2_0( QxtWebSlotService *parent )
    : QObject( parent )
    , m_service( parent )
{
}

void
Api_v2_0::ping( QxtWebRequestEvent *event )
{
    QxtWebPageEvent * e = new QxtWebPageEvent( event->sessionID, event->requestID, "pong" );
    e->contentType = "text/plain";
    m_service->postEvent( e );
}
