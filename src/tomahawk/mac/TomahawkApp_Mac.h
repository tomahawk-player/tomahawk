/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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

#ifndef TOMAHAWKAPP_MAC_H
#define TOMAHAWKAPP_MAC_H

// this file and tomahawk_app.mm copied and inspired by mac_startup.* in clementine player,
// copyright David Sansome 2010

class QString;
class QObject;

namespace Tomahawk {

class MacShortcutHandler;

/// Interface between cocoa and tomahawk
class PlatformInterface {
 public:
  // Called when the application should show itself.
  virtual void activate() = 0;
  virtual bool loadUrl( const QString& url ) = 0;

  virtual ~PlatformInterface() {}
};

void macMain();
void setShortcutHandler(Tomahawk::MacShortcutHandler* engine);
// used for opening files with tomahawk
void setApplicationHandler(PlatformInterface* handler);
void checkForUpdates();

// Pass in a QObject with slots "fullScreenEntered() and fullScreenExited() in order to be notified
void toggleFullscreen();
void enableFullscreen( QObject* notifier );

};

#endif
