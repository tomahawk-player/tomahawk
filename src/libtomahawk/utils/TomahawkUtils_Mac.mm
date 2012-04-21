#include "TomahawkUtils.h"

#import <AppKit/NSApplication.h>

namespace TomahawkUtils
{

void
bringToFront() {
    [NSApp activateIgnoringOtherApps:YES];
}

}
