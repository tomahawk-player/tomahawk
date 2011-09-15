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

#include "tomahawkapp.h"

#include "thirdparty/kdsingleapplicationguard/kdsingleapplicationguard.h"
#include <QTranslator>

#ifndef Q_WS_WIN
    #include "breakpad/BreakPad.h"
#endif

#ifdef Q_WS_MAC
    #include "tomahawkapp_mac.h"
    #include </System/Library/Frameworks/CoreServices.framework/Versions/A/Frameworks/AE.framework/Versions/A/Headers/AppleEvents.h>
    static pascal OSErr appleEventHandler( const AppleEvent*, AppleEvent*, long );
#endif


int
main( int argc, char *argv[] )
{
#ifndef Q_WS_WIN
    new BreakPad( "/tmp" );
#endif

#ifdef Q_WS_MAC
      // Do Mac specific startup to get media keys working.
      // This must go before QApplication initialisation.
      Tomahawk::macMain();

      // used for url handler
      AEEventHandlerUPP h = AEEventHandlerUPP( appleEventHandler );
      AEInstallEventHandler( 'GURL', 'GURL', h, 0, false );
#endif

    TomahawkApp a( argc, argv );
    KDSingleApplicationGuard guard( &a, KDSingleApplicationGuard::AutoKillOtherInstances );
    QObject::connect( &guard, SIGNAL( instanceStarted( KDSingleApplicationGuard::Instance ) ), &a, SLOT( instanceStarted( KDSingleApplicationGuard::Instance )  ) );

    if ( guard.isPrimaryInstance() )
        a.init();

    QString locale = QLocale::system().name();

    QTranslator translator;
    translator.load( QString( ":/lang/tomahawk_" ) + locale );
    a.installTranslator( &translator );

    if ( argc > 1 )
    {
        QString arg = a.arguments()[ 1 ];
        a.loadUrl( arg );
    }

    return a.exec();
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
