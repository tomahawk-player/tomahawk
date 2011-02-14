#ifndef TOMAHAWKAPP_MAC_H
#define TOMAHAWKAPP_MAC_H

// this file and tomahawk_app.mm copied and inspired by mac_startup.* in clementine player,
// copyright David Sansome 2010

class QString;

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

};

#endif
