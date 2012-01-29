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
#include "ubuntuunityhack.h"
#include "tomahawksettings.h"

#include <QTranslator>

#ifdef Q_WS_MAC
    #include "tomahawkapp_mac.h"
    #include </System/Library/Frameworks/CoreServices.framework/Versions/A/Frameworks/AE.framework/Versions/A/Headers/AppleEvents.h>
    static pascal OSErr appleEventHandler( const AppleEvent*, AppleEvent*, long );
#endif

#ifndef ENABLE_HEADLESS
    #include "TomahawkSettingsGui.h"
    #include "breakpad/BreakPad.h"
#endif

inline QDataStream& operator<<(QDataStream& out, const AtticaManager::StateHash& states)
{
    out <<  TOMAHAWK_SETTINGS_VERSION;
    out << (quint32)states.count();
    foreach( const QString& key, states.keys() )
    {
        AtticaManager::Resolver resolver = states[ key ];
        out << key << resolver.version << resolver.scriptPath << (qint32)resolver.state << resolver.userRating;
    }
    return out;
}


inline QDataStream& operator>>(QDataStream& in, AtticaManager::StateHash& states)
{
    quint32 count = 0, version = 0;
    in >> version;
    in >> count;
    for ( uint i = 0; i < count; i++ )
    {
        QString key, version, scriptPath;
        qint32 state, userRating;
        in >> key;
        in >> version;
        in >> scriptPath;
        in >> state;
        in >> userRating;
        states[ key ] = AtticaManager::Resolver( version, scriptPath, userRating, (AtticaManager::ResolverState)state );
    }
    return in;
}

int
main( int argc, char *argv[] )
{
#ifdef Q_WS_MAC
    // Do Mac specific startup to get media keys working.
    // This must go before QApplication initialisation.
    Tomahawk::macMain();

    // used for url handler
    AEEventHandlerUPP h = AEEventHandlerUPP( appleEventHandler );
    AEInstallEventHandler( 'GURL', 'GURL', h, 0, false );
#endif

/*    // Unity hack taken from Clementine's main.cpp
#ifdef Q_OS_LINUX
    // In 11.04 Ubuntu decided that the system tray should be reserved for certain
    // whitelisted applications.  Tomahawk will override this setting and insert
    // itself into the list of whitelisted apps.
    setenv( "QT_X11_NO_NATIVE_MENUBAR", "1", true );
    UbuntuUnityHack hack;
#endif*/

    TomahawkApp a( argc, argv );

    // MUST register StateHash ****before*** initing TomahawkSettingsGui as constructor of settings does upgrade before Gui subclass registers type
    qRegisterMetaType< AtticaManager::StateHash >( "AtticaManager::StateHash" );
    qRegisterMetaTypeStreamOperators<AtticaManager::StateHash>("AtticaManager::StateHash");

#ifdef ENABLE_HEADLESS
    new TomahawkSettings( &a );
#else
    new TomahawkSettingsGui( &a );
#endif

#ifndef ENABLE_HEADLESS
    new BreakPad( QDir::tempPath(), TomahawkSettings::instance()->crashReporterEnabled() );
#endif

    KDSingleApplicationGuard guard( &a, KDSingleApplicationGuard::AutoKillOtherInstances );
    QObject::connect( &guard, SIGNAL( instanceStarted( KDSingleApplicationGuard::Instance ) ), &a, SLOT( instanceStarted( KDSingleApplicationGuard::Instance )  ) );

    if ( guard.isPrimaryInstance() )
        a.init();

    QString locale = QLocale::system().name();
    if ( locale == "C" )
        locale = "en";
    QTranslator translator;
    if ( translator.load( QString( ":/lang/tomahawk_" ) + locale ) )
    {
        tDebug() << "Using system locale:" << locale;
    }
    else
    {
        tDebug() << "Using default locale, system locale one not found:" << locale;
        translator.load( QString( ":/lang/tomahawk_en" ) );
    }
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
