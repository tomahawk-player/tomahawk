/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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


#include "TomahawkApp.h"

#include "thirdparty/kdsingleapplicationguard/kdsingleapplicationguard.h"
#include "UbuntuUnityHack.h"
#include "TomahawkSettings.h"
#include "utils/TomahawkUtils.h"
#include "config.h"
#include "utils/Logger.h"

#ifdef Q_WS_MAC
    #include "TomahawkApp_Mac.h"
    #include </System/Library/Frameworks/CoreServices.framework/Versions/A/Frameworks/AE.framework/Versions/A/Headers/AppleEvents.h>
    static pascal OSErr appleEventHandler( const AppleEvent*, AppleEvent*, long );
#endif

#ifndef ENABLE_HEADLESS
    #include "TomahawkSettingsGui.h"
    #ifdef WITH_BREAKPAD
        #include "breakpad/BreakPad.h"
    #endif
#endif


#ifdef Q_OS_WIN
// code from patch attached to QTBUG-19064 by Honglei Zhang
LRESULT QT_WIN_CALLBACK qt_LowLevelKeyboardHookProc( int nCode, WPARAM wParam, LPARAM lParam );
HHOOK hKeyboardHook;
HINSTANCE hGuiLibInstance;

LRESULT QT_WIN_CALLBACK qt_LowLevelKeyboardHookProc( int nCode, WPARAM wParam, LPARAM lParam )
{
    LPKBDLLHOOKSTRUCT kbHookStruct = reinterpret_cast<LPKBDLLHOOKSTRUCT>(lParam);

    switch(kbHookStruct->vkCode)
    {
        case VK_VOLUME_MUTE:
        case VK_VOLUME_DOWN:
        case VK_VOLUME_UP:
        case VK_MEDIA_NEXT_TRACK:
        case VK_MEDIA_PREV_TRACK:
        case VK_MEDIA_STOP:
        case VK_MEDIA_PLAY_PAUSE:
        case VK_LAUNCH_MEDIA_SELECT:
            // send message
            {
                HWND hWnd = NULL;
                foreach ( QWidget *widget, QApplication::topLevelWidgets() )
                {
                    // relay message to each top level widgets(window)
                    // if the window has focus, we don't send a duplicate message
                    if ( QApplication::activeWindow() == widget )
                        continue;

                    hWnd = widget->winId();

                    // generate message and post it to the message queue
                    LPKBDLLHOOKSTRUCT pKeyboardHookStruct = reinterpret_cast<LPKBDLLHOOKSTRUCT>(lParam);
                    WPARAM _wParam = pKeyboardHookStruct->vkCode;
                    LPARAM _lParam = MAKELPARAM( pKeyboardHookStruct->scanCode, pKeyboardHookStruct->flags );
                    PostMessage( hWnd, wParam, _wParam, _lParam );
                }
            }
            break;

        default:
            break;
    }

    return CallNextHookEx( 0, nCode, wParam, lParam );
}

#include <io.h>
#define argc __argc
#define argv __argv
// code taken from AbiWord, (c) AbiSource Inc.

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow )
{
    hKeyboardHook = NULL;
    hGuiLibInstance = hInstance;

    // setup keyboard hook to receive multimedia key events when application is at background
    hKeyboardHook = SetWindowsHookEx( WH_KEYBOARD_LL,(HOOKPROC) qt_LowLevelKeyboardHookProc, hGuiLibInstance, 0 );

    if ( fileno( stdout ) != -1 && _get_osfhandle( fileno( stdout ) ) != -1 )
    {
        /* stdout is fine, presumably redirected to a file or pipe */
    }
    else
    {
        typedef BOOL (WINAPI * AttachConsole_t) (DWORD);
        AttachConsole_t p_AttachConsole = (AttachConsole_t) GetProcAddress( GetModuleHandleW( L"kernel32.dll" ), "AttachConsole" );

        if ( p_AttachConsole != NULL && p_AttachConsole( ATTACH_PARENT_PROCESS ) )
        {
            _wfreopen ( L"CONOUT$", L"w", stdout );
            dup2( fileno( stdout ), 1 );
            _wfreopen ( L"CONOUT$", L"w", stderr );
            dup2( fileno( stderr ), 2 );
        }
    }
#else // Q_OS_WIN

int
main( int argc, char *argv[] )
{
#ifdef Q_WS_MAC
    // Do Mac specific startup to get media keys working.
    // This must go before QApplication initialisation.
    Tomahawk::macMain();

    // Fixes focus issue with NSSearchField, see QTBUG-11401
    // code taken from clementine:main.cpp:336
    QCoreApplication::setAttribute( Qt::AA_NativeWindows, true );

    // used for url handler
    AEEventHandlerUPP h = AEEventHandlerUPP( appleEventHandler );
    AEInstallEventHandler( 'GURL', 'GURL', h, 0, false );
    #endif // Q_WS_MAC
#endif //Q_OS_WIN

    TomahawkApp a( argc, argv );

    // MUST register StateHash ****before*** initing TomahawkSettingsGui as constructor of settings does upgrade before Gui subclass registers type
    TomahawkSettings::registerCustomSettingsHandlers();
    TomahawkSettingsGui::registerCustomSettingsHandlers();

#ifdef ENABLE_HEADLESS
    new TomahawkSettings( &a );
#else
    new TomahawkSettingsGui( &a );
#endif

#ifndef ENABLE_HEADLESS
#ifdef WITH_BREAKPAD
    new BreakPad( QDir::tempPath(), TomahawkSettings::instance()->crashReporterEnabled() && !TomahawkUtils::headless() );
#endif
#endif

    KDSingleApplicationGuard guard( KDSingleApplicationGuard::AutoKillOtherInstances );
    QObject::connect( &guard, SIGNAL( instanceStarted( KDSingleApplicationGuard::Instance ) ), &a, SLOT( instanceStarted( KDSingleApplicationGuard::Instance ) ) );

    if ( guard.isPrimaryInstance() )
        a.init();

    int returnCode = 0;
    if ( guard.isPrimaryInstance() )
        returnCode = a.exec();

#ifdef Q_OS_WIN
    // clean up keyboard hook
    if ( hKeyboardHook )
    {
        UnhookWindowsHookEx( hKeyboardHook );
        hKeyboardHook = NULL;
    }
#endif

    return returnCode;
}


#ifdef Q_WS_MAC
static pascal OSErr
appleEventHandler( const AppleEvent* e, AppleEvent*, long )
{
    OSType id = typeWildCard;
    AEGetAttributePtr( e, keyEventIDAttr, typeType, 0, &id, sizeof( id ), 0 );

    switch ( id )
    {
        case 'GURL':
        {
            DescType type;
            Size size;

            char buf[1024];
            AEGetParamPtr( e, keyDirectObject, typeChar, &type, &buf, 1023, &size );
            buf[size] = '\0';

            QString url = QString::fromUtf8( buf );
            static_cast<TomahawkApp*>(qApp)->loadUrl( url );
            return noErr;
        }

        default:
            return unimpErr;
    }
}
#endif
