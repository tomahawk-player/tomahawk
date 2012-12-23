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

#include "CrashReporter.h"

#include <QTranslator>
#include <iostream>

#include "utils/TomahawkUtils.h"


const char* k_usage =
    "Usage:\n"
    "  CrashReporter <logDir> <dumpFileName> <productName>\n";

int main( int argc, char* argv[] )
{
    // used by some Qt stuff, eg QSettings
    // leave first! As Settings object is created quickly
    QCoreApplication::setApplicationName( "Tomahawk" );
    QCoreApplication::setOrganizationName( "Tomahawk" );
    QCoreApplication::setOrganizationDomain( "tomahawk-player.org" );

    QApplication app( argc, argv );
    TomahawkUtils::installTranslator( &app );

    if ( app.arguments().size() != 4 )
    {
        std::cout << k_usage;
        return 1;
    }

    CrashReporter reporter( app.arguments() );
    reporter.show();

    return app.exec();
}
