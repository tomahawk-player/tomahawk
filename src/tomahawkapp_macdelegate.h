/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 * 
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#import <AppKit/NSApplication.h>

#include "config.h"

// this file  copied and inspired by mac_startup.* in clementine player,
// copyright David Sansome 2010
namespace Tomahawk {
    class PlatformInterface;
}

#ifdef SNOW_LEOPARD
@interface AppDelegate : NSObject <NSApplicationDelegate> {
#else
@interface AppDelegate : NSObject {
#endif
  Tomahawk::PlatformInterface* application_handler_;
  //NSMenu* dock_menu_;
}

- (id) initWithHandler: (Tomahawk::PlatformInterface*)handler;
// NSApplicationDelegate
- (BOOL) applicationShouldHandleReopen: (NSApplication*)app hasVisibleWindows:(BOOL)flag;
//- (NSMenu*) applicationDockMenu: (NSApplication*)sender;
//- (void) setDockMenu: (NSMenu*)menu;
@end
