/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012 Leo Franchi <lfranchi@kde.org>
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

#include "SourceTreePopupDialog_mac.h"

#include <QWidget>

#import <Cocoa/Cocoa.h>

void
SourceTreePopupHelper::clearBackground( QWidget* widget )
{
    Q_ASSERT( widget );
    // Workaround from QTBUG-15368
    NSView* view = reinterpret_cast< NSView* >( widget->winId() );
    NSWindow* cocoaWindow = [view window];
    [cocoaWindow invalidateShadow];
}
