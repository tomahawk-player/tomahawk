#ifndef MACDELEGATE_H
#define MACDELEGATE_H

// This file inspired by clementine's MacDelegate.h

#import <Cocoa/Cocoa.h>

#include "SPMediaKeyTap.h"

namespace Tomahawk {
    class MacShortcutHandler;
    class PlatformInterface;
}

#if ( defined MAC_OS_X_VERSION_10_7 || defined SNOW_LEOPARD )
@interface AppDelegate :NSObject <NSApplicationDelegate> {
#else
@interface AppDelegate :NSObject {
#endif
  Tomahawk::PlatformInterface* application_handler_;
  NSMenu* dock_menu_;
  SPMediaKeyTap* key_tap_;
  Tomahawk::MacShortcutHandler* shortcut_handler_;
}

- (id) initWithHandler: (Tomahawk::PlatformInterface*)handler;
// NSApplicationDelegate
- (BOOL) applicationShouldHandleReopen: (NSApplication*)app hasVisibleWindows:(BOOL)flag;
- (NSMenu*) applicationDockMenu: (NSApplication*)sender;
- (void) setDockMenu: (NSMenu*)menu;
- (Tomahawk::MacShortcutHandler*) shortcutHandler;
- (void) setShortcutHandler: (Tomahawk::MacShortcutHandler*)backend;
- (void)applicationDidFinishLaunching:(NSNotification*)aNotification;
- (NSApplicationTerminateReply) applicationShouldTerminate:(NSApplication*)sender;
- (void) mediaKeyTap: (SPMediaKeyTap*)keyTap receivedMediaKeyEvent:(NSEvent*)event;
@end



#endif // MACDELEGATE_H
