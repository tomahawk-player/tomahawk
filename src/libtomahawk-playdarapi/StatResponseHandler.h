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

#pragma once
#ifndef STATRESPONSEHANDLER_H
#define STATRESPONSEHANDLER_H

#include <QObject>

class Api_v1;
class QxtWebRequestEvent;

class StatResponseHandler : public QObject
{
    Q_OBJECT
public:
    StatResponseHandler( Api_v1* parent, QxtWebRequestEvent* event );

public slots:
    void statResult( const QString& clientToken, const QString& name, bool valid );

private:
    Api_v1* m_parent;
    QxtWebRequestEvent* m_storedEvent;
};

#endif // STATRESPONSEHANDLER_H
