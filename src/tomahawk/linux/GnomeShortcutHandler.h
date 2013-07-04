/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Florian Richter <mail@f1ori.de>
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

#ifndef GNOMESHORTCUTHANDLER_H
#define GNOMESHORTCUTHANDLER_H

#include "ShortcutHandler.h"
#include "GnomeSettingsDaemonMediaKeysProxy.h"

#include <QObject>

namespace Tomahawk {


class GnomeShortcutHandler : public ShortcutHandler
{
    Q_OBJECT
public:
    explicit GnomeShortcutHandler(QObject *parent = 0);
    bool DoRegister();

    static const char* kGsdService;
    static const char* kGsdPath;
    static const char* kGsdInterface;

public slots:
    void RegisterFinished(QDBusPendingCallWatcher* watcher);
    void GnomeMediaKeyPressed( const QString& application, const QString& key );

private:
    org::gnome::SettingsDaemon::MediaKeys* interface_;

};

}

#endif // GNOMESHORTCUTHANDLER_H

