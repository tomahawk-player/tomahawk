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

#ifndef API_V2_0_H
#define API_V2_0_H

#include <QObject>

class Api_v2;
class QxtWebRequestEvent;
class QxtWebSlotService;

class Api_v2_0 : public QObject
{
    Q_OBJECT
public:
    Api_v2_0( Api_v2* parent = 0 );
    
signals:
    
public slots:
    /**
     * Simple test to check for API 2.0 support.
     *
     * This call needs no authentication.
     */
    void ping( QxtWebRequestEvent* event );

    /**
     * Control playback.
     *
     * This call needs to be authenticated.
     */
    void playback( QxtWebRequestEvent* event, const QString& command );
private:
    Api_v2* m_service;
};

#endif // API_V2_0_H
