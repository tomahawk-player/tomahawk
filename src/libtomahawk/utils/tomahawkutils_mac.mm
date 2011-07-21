#include "tomahawkutils.h"

#import <AppKit/NSApplication.h>

namespace TomahawkUtils
{

void
bringToFront() {
    qDebug() << "foo";
    [NSApp activateIgnoringOtherApps:YES];
}

}
