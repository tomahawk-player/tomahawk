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

// implement listen on media keys events provided by the gnome settings daemon
// as documented here:
// https://github.com/GNOME/gnome-settings-daemon/blob/master/plugins/media-keys/README.media-keys-API

#include "GnomeShortcutHandler.h"
#include "utils/Logger.h"


# include <QtDBus>

#include <QAction>

using namespace Tomahawk;

const char* GnomeShortcutHandler::kGsdService = "org.gnome.SettingsDaemon";
const char* GnomeShortcutHandler::kGsdPath = "/org/gnome/SettingsDaemon/MediaKeys";
const char* GnomeShortcutHandler::kGsdInterface = "org.gnome.SettingsDaemon.MediaKeys";


GnomeShortcutHandler::GnomeShortcutHandler(QObject *parent) :
    Tomahawk::ShortcutHandler(parent),
    interface_(NULL)
{

}

bool GnomeShortcutHandler::DoRegister() {
    tLog(LOGVERBOSE) << "registering for gnome media keys";

    // Check if the GSD service is available
    if (!QDBusConnection::sessionBus().interface()->isServiceRegistered(kGsdService)) {
        tLog(LOGVERBOSE) << "gnome settings daemon not registered";
        return false;
    }

    if (!interface_) {
        interface_ = new org::gnome::SettingsDaemon::MediaKeys(
            kGsdService, kGsdPath, QDBusConnection::sessionBus(), this->parent());
    }

    QDBusPendingReply<> reply = interface_->GrabMediaPlayerKeys(
            QCoreApplication::applicationName(), 0);

    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)),
               this, SLOT(RegisterFinished(QDBusPendingCallWatcher*)));

    return true;
}

void GnomeShortcutHandler::RegisterFinished(QDBusPendingCallWatcher* watcher) {
    QDBusMessage reply = watcher->reply();
    watcher->deleteLater();

    if (reply.type() == QDBusMessage::ErrorMessage) {
        tLog(LOGVERBOSE) << "Failed to grab media keys"
               << reply.errorName() <<reply.errorMessage();
        return;
    }

    connect(interface_, SIGNAL(MediaPlayerKeyPressed(QString,QString)),
            this, SLOT(GnomeMediaKeyPressed(QString,QString)));

    tLog(LOGVERBOSE) << "gnome media keys registered";
}

void
GnomeShortcutHandler::GnomeMediaKeyPressed( const QString& app, const QString& val )
{
    if (app != QCoreApplication::applicationName())
        return;

    tLog(LOGVERBOSE) << "gnome media key " << val << " pressed";
    if ( val == "Play" ) {
        emit playPause();
    }
    if ( val == "Next" ) {
        emit next();
    }
    if ( val == "Previous" ) {
        emit previous();
    }
}

