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

#include "TomahawkApp_Mac.h"

#include "MacDelegate.h"
#include "MacShortcutHandler.h"
#include "config.h"
#include "TomahawkWindow.h"
#include "audio/AudioEngine.h"

#import <Cocoa/Cocoa.h>

#ifdef HAVE_SPARKLE
#import <Sparkle/SUUpdater.h>
#endif

#include <QDebug>
#include <QApplication>
#include <QObject>
#include <QMetaObject>

@interface MacApplication :NSApplication {
    AppDelegate* delegate_;
    Tomahawk::MacShortcutHandler* shortcut_handler_;
    Tomahawk::PlatformInterface* application_handler_;
}

- (Tomahawk::MacShortcutHandler*) shortcutHandler;
- (void) setShortcutHandler: (Tomahawk::MacShortcutHandler*)handler;

- (Tomahawk::PlatformInterface*) application_handler;
- (void) setApplicationHandler: (Tomahawk::PlatformInterface*)handler;

#ifdef HAVE_SPARKLE
// SUUpdaterDelegate
- (void)updater:(SUUpdater *)updater willInstallUpdate:(SUAppcastItem *)update;
#endif

@end


@implementation AppDelegate

- (id) init {
  if ((self = [super init])) {
      application_handler_ = nil;
      shortcut_handler_ = nil;
      //dock_menu_ = nil;
  }
  return self;
}

- (id) initWithHandler: (Tomahawk::PlatformInterface*)handler {
  application_handler_ = handler;

  // Register defaults for the whitelist of apps that want to use media keys
  [[NSUserDefaults standardUserDefaults] registerDefaults:[NSDictionary dictionaryWithObjectsAndKeys:
     [SPMediaKeyTap defaultMediaKeyUserBundleIdentifiers], @"SPApplicationsNeedingMediaKeys",
      nil]];


  return self;
}

- (BOOL) applicationShouldHandleReopen: (NSApplication*)app hasVisibleWindows:(BOOL)flag {
  if (application_handler_) {
    application_handler_->activate();
  }
  return YES;
}

- (void) setDockMenu: (NSMenu*)menu {
  dock_menu_ = menu;
}

- (NSMenu*) applicationDockMenu: (NSApplication*)sender {
  return dock_menu_;
}


- (Tomahawk::MacShortcutHandler*) shortcutHandler {
    return shortcut_handler_;
}

- (void) setShortcutHandler: (Tomahawk::MacShortcutHandler*)handler {
    // should be the same as MacApplication's
  shortcut_handler_ = handler;
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
  key_tap_ = [[SPMediaKeyTap alloc] initWithDelegate:self];
  if([SPMediaKeyTap usesGlobalMediaKeyTap])
    [key_tap_ startWatchingMediaKeys];
  else
    qWarning()<<"Media key monitoring disabled";

}

- (void) mediaKeyTap: (SPMediaKeyTap*)keyTap receivedMediaKeyEvent:(NSEvent*)event {
  NSAssert([event type] == NSSystemDefined && [event subtype] == SPSystemDefinedEventMediaKeys, @"Unexpected NSEvent in mediaKeyTap:receivedMediaKeyEvent:");

  int key_code = (([event data1] & 0xFFFF0000) >> 16);
  int key_flags = ([event data1] & 0x0000FFFF);
  BOOL key_is_pressed = (((key_flags & 0xFF00) >> 8)) == 0xA;
  // not used. keep just in case
  //  int key_repeat = (key_flags & 0x1);

  if (!shortcut_handler_) {
    qWarning() << "No shortcut handler when we get a media key event...";
    return;
  }
  if (key_is_pressed) {
    shortcut_handler_->macMediaKeyPressed(key_code);
  }
}

- (BOOL) application: (NSApplication*)app openFile:(NSString*)filename {

  if (application_handler_->loadUrl(QString::fromUtf8([filename UTF8String]))) {
    return YES;
  }

  return NO;
}

- (NSApplicationTerminateReply) applicationShouldTerminate:(NSApplication*) sender {
  return NSTerminateNow;
}

@end

@implementation MacApplication

- (id) init {
  if ((self = [super init])) {
      [self setShortcutHandler:nil];
      [self setApplicationHandler:nil];

      NSAppleEventManager *em = [NSAppleEventManager sharedAppleEventManager];
      [em
        setEventHandler:self
            andSelector:@selector(getUrl:withReplyEvent:)
          forEventClass:kInternetEventClass
             andEventID:kAEGetURL];
      [em
        setEventHandler:self
            andSelector:@selector(getUrl:withReplyEvent:)
          forEventClass:'WWW!'
             andEventID:'OURL'];
      NSString *bundleID = [[NSBundle mainBundle] bundleIdentifier];
      OSStatus httpResult = LSSetDefaultHandlerForURLScheme((CFStringRef)@"tomahawk", (CFStringRef)bundleID);

      Q_UNUSED(httpResult);
      //TODO: Check httpResult and httpsResult for errors
  }
  return self;
}

- (Tomahawk::MacShortcutHandler*) shortcutHandler {
    return shortcut_handler_;
}

- (void) setShortcutHandler: (Tomahawk::MacShortcutHandler*)handler {
    // should be the same as AppDelegate's
  shortcut_handler_ = handler;
}

- (Tomahawk::PlatformInterface*) application_handler {
  return application_handler_;
}

- (void) setApplicationHandler: (Tomahawk::PlatformInterface*)handler {
  delegate_ = [[AppDelegate alloc] initWithHandler:handler];
  // App-shortcut-handler set before delegate is set.
  // this makes sure the delegate's shortcut_handler is set
  [delegate_ setShortcutHandler:shortcut_handler_];
  [self setDelegate:delegate_];
}

-(void) sendEvent: (NSEvent*)event {
    // If event tap is not installed, handle events that reach the app instead
    BOOL shouldHandleMediaKeyEventLocally = ![SPMediaKeyTap usesGlobalMediaKeyTap];

    if(shouldHandleMediaKeyEventLocally && [event type] == NSSystemDefined && [event subtype] == SPSystemDefinedEventMediaKeys) {
      [(id)[self delegate] mediaKeyTap: nil receivedMediaKeyEvent: event];
    }

    [super sendEvent: event];
}

#ifdef HAVE_SPARKLE
- (void)updater:(SUUpdater *)updater willInstallUpdate:(SUAppcastItem *)update
{
    tLog() << "NSApp in willInstallUpdate, deleting Phonon objects";
    AudioEngine::instance()->stop();
    delete AudioEngine::instance();
}
#endif

@end

void Tomahawk::macMain() {
  [[NSAutoreleasePool alloc] init];
  // Creates and sets the magic global variable so QApplication will find it.
  [MacApplication sharedApplication];
#ifdef HAVE_SPARKLE
    // Creates and sets the magic global variable for Sparkle.
    [[SUUpdater sharedUpdater] setDelegate: NSApp];
#endif
}


void Tomahawk::setShortcutHandler(Tomahawk::MacShortcutHandler* handler) {
  [NSApp setShortcutHandler: handler];
}

void Tomahawk::setApplicationHandler(Tomahawk::PlatformInterface* handler) {
  [NSApp setApplicationHandler: handler];
}

void Tomahawk::checkForUpdates() {
#ifdef HAVE_SPARKLE
  [[SUUpdater sharedUpdater] checkForUpdates: NSApp];
#endif
}

#if defined(LION) || defined(MOUNTAIN_LION)
#define SET_LION_FULLSCREEN NSWindowCollectionBehaviorFullScreenPrimary
#define LION_FULLSCREEN_ENTER_NOTIFICATION_VALUE NSWindowWillEnterFullScreenNotification
#define LION_FULLSCREEN_EXIT_NOTIFICATION_VALUE NSWindowDidExitFullScreenNotification
#else
#define SET_LION_FULLSCREEN (NSUInteger)(1 << 7) // Defined as NSWindowCollectionBehaviorFullScreenPrimary in lion's NSWindow.h
#define LION_FULLSCREEN_ENTER_NOTIFICATION_VALUE @"NSWindowWillEnterFullScreenNotification"
#define LION_FULLSCREEN_EXIT_NOTIFICATION_VALUE @"NSWindowDidExitFullScreenNotification"
#endif

void Tomahawk::toggleFullscreen()
{
    if ( QSysInfo::MacintoshVersion != QSysInfo::MV_SNOWLEOPARD &&
         QSysInfo::MacintoshVersion != QSysInfo::MV_LEOPARD   )
    {
        qDebug() << "Toggling Lion Full-screeen";
        // Can't include TomahawkApp.h in a .mm file, pulls in InfoSystem.h which uses
        // the objc keyword 'id'
        foreach( QWidget* w, QApplication::topLevelWidgets() )
        {
            if ( qobject_cast< TomahawkWindow* >( w ) )
            {
                NSView *nsview = (NSView *)w->winId();
                NSWindow *nswindow = [nsview window];
                [nswindow toggleFullScreen: nil];
            }
        }
    }
}

void Tomahawk::enableFullscreen( QObject* receiver )
{
    // We don't support anything below leopard, so if it's not [snow] leopard it must be lion
    // Can't check for lion as Qt 4.7 doesn't have the enum val, not checking for Unknown as it will be lion
    // on 4.8
    if ( QSysInfo::MacintoshVersion != QSysInfo::MV_SNOWLEOPARD &&
         QSysInfo::MacintoshVersion != QSysInfo::MV_LEOPARD   )
    {
        qDebug() << "Enabling Lion Full-screeen";
        // Can't include TomahawkApp.h in a .mm file, pulls in InfoSystem.h which uses
        // the objc keyword 'id'
        foreach( QWidget* w, QApplication::topLevelWidgets() )
        {
            if ( qobject_cast< TomahawkWindow* >( w ) )
            {
                NSView *nsview = (NSView *)w->winId();
                NSWindow *nswindow = [nsview window];
                [nswindow setCollectionBehavior:SET_LION_FULLSCREEN];

                if ( !receiver )
                    continue;

                [[NSNotificationCenter defaultCenter] addObserverForName:LION_FULLSCREEN_ENTER_NOTIFICATION_VALUE
                                                                  object:nswindow
                                                                   queue:nil
                                                              usingBlock:^(NSNotification * note) {
                    NSLog(@"Became Full Screen!");
                    QMetaObject::invokeMethod( receiver, "fullScreenEntered", Qt::DirectConnection );
                }];
                [[NSNotificationCenter defaultCenter] addObserverForName:LION_FULLSCREEN_EXIT_NOTIFICATION_VALUE
                                                                  object:nswindow
                                                                   queue:nil
                                                              usingBlock:^(NSNotification * note) {
                    NSLog(@"Left Full Screen!");
                    QMetaObject::invokeMethod( receiver, "fullScreenExited", Qt::DirectConnection );
                }];
            }
        }
    }
}
