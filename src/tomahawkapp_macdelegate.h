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
