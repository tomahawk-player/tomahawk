#include "tomahawk/tomahawkapp.h"

#ifdef Q_WS_MAC
#include "tomahawkapp_mac.h"
#include </System/Library/Frameworks/CoreServices.framework/Versions/A/Frameworks/AE.framework/Versions/A/Headers/AppleEvents.h>
static pascal OSErr appleEventHandler( const AppleEvent*, AppleEvent*, long );
#endif

#include <exception>
int main( int argc, char *argv[] )
{
#ifdef Q_WS_MAC
      // Do Mac specific startup to get media keys working.
      // This must go before QApplication initialisation.
      Tomahawk::macMain();

      // used for url handler
      AEEventHandlerUPP h = AEEventHandlerUPP( appleEventHandler );
      AEInstallEventHandler( 'GURL', 'GURL', h, 0, false );

#endif
    try {
        TomahawkApp a( argc, argv );
        return a.exec();
    } catch( const std::runtime_error& e ) {
        return 0;
    }
}

#ifdef Q_WS_MAC
static pascal OSErr appleEventHandler( const AppleEvent* e, AppleEvent*, long )
{
    OSType id = typeWildCard;
    AEGetAttributePtr( e, keyEventIDAttr, typeType, 0, &id, sizeof(id), 0 );

    switch (id)
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
