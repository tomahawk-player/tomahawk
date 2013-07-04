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

#ifndef UPOWERHANDLER_H
#define UPOWERHANDLER_H

#include "UPowerProxy.h"

#include <QObject>
#include <QMutex>
#include <QSharedPointer>

namespace Tomahawk {

class UPowerHandler : public QObject
{
    Q_OBJECT
public:
    explicit UPowerHandler( QObject *parent = 0 );
    bool registerHandler();

    static const char* UPowerService;
    static const char* UPowerPath;
    static const char* UPowerInterface;
private:
    QSharedPointer<org::freedesktop::UPower> m_interface;
    QMutex m_mutex;

private slots:
    void handleSleep();
    void handleResume();
    void actualResume();
};

}

#endif // UPOWERHANDLER_H
