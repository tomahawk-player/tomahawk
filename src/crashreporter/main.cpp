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

#include <libcrashreporter-gui/CrashReporter.h>

#include <QTranslator>
#include <iostream>

#include "utils/TomahawkUtils.h"


const char* k_usage =
    "Usage:\n"
    "  CrashReporter <dumpFilePath>\n";

int main( int argc, char* argv[] )
{
    // used by some Qt stuff, eg QSettings
    // leave first! As Settings object is created quickly
    QCoreApplication::setApplicationName( "Tomahawk" );
    QCoreApplication::setOrganizationName( "Tomahawk" );
    QCoreApplication::setOrganizationDomain( "tomahawk-player.org" );

    QApplication app( argc, argv );
    TomahawkUtils::installTranslator( &app );

    if ( app.arguments().size() != 2 )
    {
        std::cout << k_usage;
        return 1;
    }

    CrashReporter reporter( QUrl( "http://crash-reports.tomahawk-player.org/submit" ),  app.arguments() );

    reporter.setLogo(QPixmap("/home/domme/dev/sources/tomahawk/data/icons/tomahawk-icon-128x128.png"));


//     // socorro expects a 10 digit build id
//     QRegExp rx( "(\\d+\\.\\d+\\.\\d+).(\\d+)" );
//     rx.exactMatch( TomahawkUtils::appFriendlyVersion() );
//     //QString const version = rx.cap( 1 );
//     QString const buildId = rx.cap( 2 ).leftJustified( 10, '0' );

     reporter.setReportData( "BuildID", "YYYYMMDDHH" );
     reporter.setReportData( "ProductName",  "WaterWolf" );
     reporter.setReportData( "Version", TomahawkUtils::appFriendlyVersion().toLocal8Bit() );
     //reporter.setReportData( "timestamp", QByteArray::number( QDateTime::currentDateTime().toTime_t() ) );


        // add parameters

//        QList<Pair> pairs;
//        pairs  //<< Pair( "BuildID", buildId.toUtf8() )
//        << Pair( )
//        //<< Pair( "Version", TomahawkUtils::appFriendlyVersion().toLocal8Bit() )
//        //<< Pair( "Vendor", "Tomahawk" )
//        //<< Pair(  )

        //            << Pair("InstallTime", "1357622062")
        //            << Pair("Theme", "classic/1.0")
        //            << Pair("Version", "30")
        //            << Pair("id", "{ec8030f7-c20a-464f-9b0e-13a3a9e97384}")
        //            << Pair("Vendor", "Mozilla")
        //            << Pair("EMCheckCompatibility", "true")
        //            << Pair("Throttleable", "0")
        //            << Pair("URL", "http://code.google.com/p/crashme/")
        //            << Pair("version", "20.0a1")
        //            << Pair("CrashTime", "1357770042")
        //            << Pair("ReleaseChannel", "nightly")
        //            << Pair("submitted_timestamp", "2013-01-09T22:21:18.646733+00:00")
        //            << Pair("buildid", "20130107030932")
        //            << Pair("timestamp", "1357770078.646789")
        //            << Pair("Notes", "OpenGL: NVIDIA Corporation -- GeForce 8600M GT/PCIe/SSE2 -- 3.3.0 NVIDIA 313.09 -- texture_from_pixmap\r\n")
        //            << Pair("StartupTime", "1357769913")
        //            << Pair("FramePoisonSize", "4096")
        //            << Pair("FramePoisonBase", "7ffffffff0dea000")
        //            << Pair("Add-ons", "%7B972ce4c6-7e08-4474-a285-3208198ce6fd%7D:20.0a1,crashme%40ted.mielczarek.org:0.4")
        //            << Pair("BuildID", "YYYYMMDDHH")
        //            << Pair("SecondsSinceLastCrash", "1831736")
        //            << Pair("ProductName", "WaterWolf")
        //            << Pair("legacy_processing", "0")
        //            << Pair("ProductID", "{ec8030f7-c20a-464f-9b0e-13a3a9e97384}")

        ;


        //     // add logfile
        //     body += "--thkboundary\r\n";
        //     body += "Content-Disposition: form-data; name=\"upload_file_tomahawklog\"; filename=\"Tomahawk.log\"\r\n";
        //     body += "Content-Type: application/x-gzip\r\n";
        //     body += "\r\n";
        //     body += qCompress( contents( LOGFILE ) );
        //     body += "\r\n";
        //     body += "--thkboundary--\r\n";

    reporter.show();

    return app.exec();
}
