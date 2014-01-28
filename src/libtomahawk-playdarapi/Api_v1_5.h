/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2014,      Uwe L. Korn <uwelk@xhochy.com>
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

#ifndef API_V1_5_H
#define API_V1_5_H

#include <QObject>

class Api_v1;
class QxtWebRequestEvent;

class Api_v1_5 : public QObject
{
    Q_OBJECT
public:
    Api_v1_5( Api_v1* parent = 0 );

signals:

public slots:
    /**
     * Simple test to check for API 1.5 support.
     *
     * This call needs no authentication.
     */
    void ping( QxtWebRequestEvent* event );

    /**
     *  Control playback.
     */
    void playback( QxtWebRequestEvent* event, const QString& command );

protected:
    void jsonReply( QxtWebRequestEvent* event, const char* funcInfo, const QString& errorMessage, bool isError );

private:
    Api_v1* m_service;
};

#endif // API_V1_5_H
