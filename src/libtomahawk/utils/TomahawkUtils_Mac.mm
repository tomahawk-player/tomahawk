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

#include "mac/FileHelpers.h"

#import <Cocoa/Cocoa.h>

#include "TomahawkUtils.h"
#include "TomahawkUtils_Mac.h"

#include <QDir>
#include <QTemporaryFile>

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
    // HACK since I can't figure out how to get QuaZip to maintain executable permissions after unzip (nor find the info)
    // we set the binary to executable here

    NSLog(@"Move succeeded!, handling result");

    NSFileManager *manager = [[[NSFileManager alloc] init] autorelease];
    NSError* error;
    NSDictionary* attrs = [NSDictionary dictionaryWithObjectsAndKeys:[NSNumber numberWithInt:0755], NSFilePosixPermissions, nil];

    NSString* target = [[NSString alloc] initWithBytes:path.toUtf8() length:path.length() encoding: NSUTF8StringEncoding];
    NSLog(@"Changing permissions to executable for: %@", target);
    BOOL success = [manager setAttributes:attrs ofItemAtPath:target error:&error];
    if (!success) {
        NSLog( @"Failed to do chmod +x of moved resolver! %@", [[error userInfo] objectForKey: NSLocalizedDescriptionKey] );
    }

    if ( receiver )
        QMetaObject::invokeMethod(receiver, "installSucceeded", Qt::DirectConnection, Q_ARG(QString, path));

    [target release];

}

- (void)moveFailedWithError:(NSError *)error
{
    NSLog(@"Move failed, handling result");
    if ( receiver )
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
copyWithAuthentication( const QString& srcFile, const QDir dest, QObject* receiver )
{
  /**
    On OS X, we have to do the following:
    1) Authenticate to be able to have write access to the /Applications folder
    2) Copy file to dest
    5) Call result slots on receiver object
  */

    MoveDelegate* del = [[MoveDelegate alloc] init];
    [del setReceiver: receiver];

    // Get the filename + path to save for later
    QFileInfo srcInfo( srcFile );
    const QString resultingPath = dest.absoluteFilePath( srcInfo.fileName() );
    [del setMoveTo: resultingPath];

    const QFileInfo info( srcFile );
    const QString destPath = dest.absoluteFilePath( info.fileName() );

    NSString* src = [[NSString alloc] initWithBytes: srcFile.toUtf8() length: srcFile.length() encoding: NSUTF8StringEncoding];
    NSString* destStr = [[NSString alloc] initWithBytes: destPath.toUtf8() length: destPath.length() encoding: NSUTF8StringEncoding];
    [FileHelpers moveFile:src to:destStr withDelegate:del];
}


}
