/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012, Leo Franchi <lfranchi@kde.org
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Tomahawk is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */

#include "TomahawkUtils.h"

#include "TomahawkUtils_Mac.h"
#include "mac/FileHelpers.h"

#include <QTemporaryFile>

#import <AppKit/NSApplication.h>
#import <Foundation/Foundation.h>

@implementation MoveDelegate


-(void) setReceiver:(QObject*) object
{
    receiver = object;
}

-(void) setMoveTo:(QString) p
{
    path = p;
}

- (void)moveFinished
{
    QMetaObject::invokeMethod(receiver, "installSucceeded", Qt::DirectConnection, Q_ARG(QString, path));
}

- (void)moveFailedWithError:(NSError *)error
{
    QMetaObject::invokeMethod(receiver, "installFailed", Qt::DirectConnection);
}
@end

namespace TomahawkUtils
{

void
bringToFront() {
    [NSApp activateIgnoringOtherApps:YES];
}


void
extractBinaryResolver( const QString& zipFilename, const QString& resolverId, QObject* receiver )
{
    /**
      On OS X, we have to do the following:
      2) Extract file in temporary location
      3) Authenticate to be able to have write access to the /Applications folder
      4) Copy the contents of the zipfile to the Tomahawk.app/Contents/MacOS/ folder
      5) Call result slots on receiver object
    */

    MoveDelegate* del = [[MoveDelegate alloc] init];
    [del setReceiver: receiver];

    // Unzip in temporary folder and copy the contents to MacOS/
    NSError* err = NULL;
    NSFileManager *manager = [[[NSFileManager alloc] init] autorelease];
    NSURL* tempDir = [manager URLForDirectory:NSCachesDirectory inDomain:NSUserDomainMask appropriateForURL:NULL create:YES error:&err];
    if ( err )
    {
        qDebug() << "GOT ERROR trying to create temp dir to unzip in...:" << err;
        return;
    }

    qDebug() << "Using temporary directory:" << [tempDir absoluteString];


//    [del setMoveTo: to];
}

}
