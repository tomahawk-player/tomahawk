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

#import "FileHelpers.h"

#import <CoreServices/CoreServices.h>
#import <Security/Security.h>
#import <sys/stat.h>
#import <sys/wait.h>
#import <dirent.h>
#import <unistd.h>
#import <sys/param.h>

#include <QDebug>

static NSString * const TKCopySourceKey = @"TKInstallerSourcePath";
static NSString * const TKCopyDestinationKey = @"TKInstallerDestinationPath";
static NSString * const TKInstallerDelegateKey = @"TKInstallerDelegate";
static NSString * const TKInstallerResultKey = @"TKInstallerResult";
static NSString * const TKInstallerErrorKey = @"TKInstallerError";

class CAutoreleasePool
{
  NSAutoreleasePool *pool;

public:
  CAutoreleasePool()
  {
    pool = [[NSAutoreleasePool alloc] init];
  }

  ~CAutoreleasePool()
  {
    [pool drain];
  }
};

// Authorization code based on generous contribution from Allan Odgaard. Thanks, Allan!
static BOOL AuthorizationExecuteWithPrivilegesAndWait(AuthorizationRef authorization, const char* executablePath, AuthorizationFlags options, const char* const* arguments)
{
        // *** MUST BE SAFE TO CALL ON NON-MAIN THREAD!

        sig_t oldSigChildHandler = signal(SIGCHLD, SIG_DFL);
        BOOL returnValue = YES;

        if (AuthorizationExecuteWithPrivileges(authorization, executablePath, options, (char* const*)arguments, NULL) == errAuthorizationSuccess)
        {
                int status;
                pid_t pid = wait(&status);
                if (pid == -1 || !WIFEXITED(status) || WEXITSTATUS(status) != 0)
                        returnValue = NO;
        }
        else
                returnValue = NO;

        signal(SIGCHLD, oldSigChildHandler);
        return returnValue;
}

@implementation FileHelpers

+ (void) moveFile:(NSString *)source to:(NSString*)dest withDelegate:delegate
{
    NSLog(@"FileHelpers moving file from %@ to %@", source, dest);

    NSDictionary *info = [NSDictionary dictionaryWithObjectsAndKeys:source, TKCopySourceKey, dest, TKCopyDestinationKey, delegate, TKInstallerDelegateKey, nil];
    [NSThread detachNewThreadSelector:@selector(performMoveWithInfo:) toTarget:self withObject:info];
}


+ (void)performMoveWithInfo:(NSDictionary *)info
{
        // *** GETS CALLED ON NON-MAIN THREAD!

    CAutoreleasePool _p;

    NSString* fromPath = [info objectForKey: TKCopySourceKey];
    NSString* toPath = [info objectForKey: TKCopyDestinationKey];

    AuthorizationRef auth = NULL;
    OSStatus authStat = errAuthorizationDenied;

    NSLog(@"FileHelpers moving file from %@ to %@", fromPath, toPath);
    BOOL haveOld = [[NSFileManager defaultManager] fileExistsAtPath: toPath];

    if (haveOld == YES) { // delete the old file if it's there
        if (0 != access([[toPath stringByDeletingLastPathComponent] fileSystemRepresentation], W_OK)
            || 0 != access([[[toPath stringByDeletingLastPathComponent] stringByDeletingLastPathComponent] fileSystemRepresentation], W_OK))
        {
            const char* rmParams[] = { [toPath fileSystemRepresentation], NULL };
            // NSLog( @"WOULD DELETE: %@", [toPath fileSystemRepresentation] );

            while( authStat == errAuthorizationDenied )
            {
                authStat = AuthorizationCreate(NULL,
                                               kAuthorizationEmptyEnvironment,
                                               kAuthorizationFlagDefaults,
                                               &auth);
            }
            if (authStat == errAuthorizationSuccess)
            {
                BOOL res = AuthorizationExecuteWithPrivilegesAndWait( auth, "/bin/rm", kAuthorizationFlagDefaults, rmParams );
                if (!res)
                    NSLog(@"Could not delete: %@", toPath);
            } else {
                qDebug() << "Failed to authenticate to delete file under target to move, aborting";
                return;
            }
        } else {
            // We can delete it ourselves w/out authenticating
            NSFileManager *manager = [[[NSFileManager alloc] init] autorelease];
            NSError* error;
            BOOL success = [manager removeItemAtPath:toPath error:&error];

            if (!success) {
                NSLog(@"Failed to delete file (w/out perms) underneath copy!: %@", [[error userInfo] objectForKey: NSLocalizedDescriptionKey]);
                return;
            }
        }
    }

    FSRef dstRef, dstDirRef;
    OSStatus err = FSPathMakeRefWithOptions((UInt8 *)[toPath fileSystemRepresentation], kFSPathMakeRefDoNotFollowLeafSymlink, &dstRef, NULL);

    if (err != noErr && err != fnfErr) { // If the file is not found that's fine, we're moving to there after all
        qDebug() << "GOT AN ERROR DOING FSPathMakeRefWithOptions!!!!! aborting move";
        return;
    }

    if (0 != access([[toPath stringByDeletingLastPathComponent] fileSystemRepresentation], W_OK)
        || 0 != access([[[toPath stringByDeletingLastPathComponent] stringByDeletingLastPathComponent] fileSystemRepresentation], W_OK))
    {
        // Not writeable by user, so authenticate
        if (!auth) {
            while( authStat == errAuthorizationDenied )
            {
                authStat = AuthorizationCreate(NULL,
                                               kAuthorizationEmptyEnvironment,
                                               kAuthorizationFlagDefaults,
                                               &auth);
            }
        }

        if (authStat == errAuthorizationSuccess)
        {
            // Fix perms before moving so we have them correct when they arrive
            struct stat dstSB;
            stat([[toPath stringByDeletingLastPathComponent] fileSystemRepresentation], &dstSB);
            char uidgid[42];
            snprintf(uidgid, sizeof(uidgid), "%d:%d",
                     dstSB.st_uid, dstSB.st_gid);

            const char* coParams[] = { "-R", uidgid, [fromPath fileSystemRepresentation], NULL };
            BOOL res = AuthorizationExecuteWithPrivilegesAndWait( auth, "/usr/sbin/chown", kAuthorizationFlagDefaults, coParams );
            if( !res )
                qDebug() << "Failed to set permissions before moving";

            // Do the move
            const char* mvParams[] = { "-f", [fromPath fileSystemRepresentation], [toPath fileSystemRepresentation], NULL };
            res = AuthorizationExecuteWithPrivilegesAndWait( auth, "/bin/mv", kAuthorizationFlagDefaults, mvParams );
            if( !res )
                NSLog(@"Failed to move source file from %@ to %@ with error %@", fromPath, toPath, res );

            AuthorizationFree(auth, 0);
            auth = NULL;
        }

        return;
    }

    if (auth) {
        AuthorizationFree(auth, 0);
        auth = NULL;
    }

    err = FSPathMakeRef((UInt8 *)[[toPath stringByDeletingLastPathComponent] fileSystemRepresentation], &dstDirRef, NULL);

    if (err != noErr) {
        qDebug() << "GOT AN ERROR DOING FSPathMakeRef to get dir to copy into!!!!! aborting move";
        return;
    }

    NSFileManager *manager = [[[NSFileManager alloc] init] autorelease];
    NSError* error;
    BOOL success = [manager moveItemAtPath:fromPath toPath:toPath error:&error];
    if (!success) {
        NSLog( @"Failed to do non-authenticated move! Help! %@", [[error userInfo] objectForKey: NSLocalizedDescriptionKey] );
    }
    [self notifyDelegate:[NSDictionary dictionaryWithObjectsAndKeys:[NSNumber numberWithBool:success], TKInstallerResultKey, [info objectForKey:TKInstallerDelegateKey], TKInstallerDelegateKey, error, TKInstallerErrorKey, nil]];
}


+ (void)notifyDelegate:(NSDictionary *)info
{
        // *** GETS CALLED ON NON-MAIN THREAD!
    BOOL result = [[info objectForKey:TKInstallerResultKey] boolValue];
    if (result)
    {
        if ([[info objectForKey:TKInstallerDelegateKey] respondsToSelector:@selector(moveFinished)])
            [[info objectForKey:TKInstallerDelegateKey] performSelectorOnMainThread: @selector(moveFinished) withObject:nil waitUntilDone: NO];
    }
    else
    {
        if ([[info objectForKey:TKInstallerDelegateKey] respondsToSelector:@selector(moveFailedWithError:)])
        {
            [[info objectForKey:TKInstallerDelegateKey] performSelectorOnMainThread: @selector(moveFailedWithError) withObject:[NSDictionary dictionaryWithObjectsAndKeys:[info objectForKey:TKInstallerErrorKey], TKInstallerErrorKey, nil] waitUntilDone: NO];
        }
    }
}

@end
