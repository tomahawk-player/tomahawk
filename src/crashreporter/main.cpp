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

#include "CrashReporterConfig.h"

#include <QTranslator>
#include <iostream>
#include <QApplication>
#include <QFileInfo>

#include "utils/TomahawkUtils.h"
#include "utils/Logger.h"


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

    reporter.setLogo(QPixmap(":/tomahawk-icon.png"));


    reporter.setReportData( "BuildID", CRASHREPORTER_BUILD_ID );
    reporter.setReportData( "ProductName",  "Tomahawk" );
    reporter.setReportData( "Version", TomahawkUtils::appFriendlyVersion().toLocal8Bit() );
    reporter.setReportData( "ReleaseChannel", CRASHREPORTER_RELEASE_CHANNEL);

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
        //            << Pair("submitted_timestamp", "2013-01-09T22:21:18.646733+00:00")
        //            << Pair("buildid", "20130107030932")
        //            << Pair("timestamp", "1357770078.646789")
        //            << Pair("Notes", "OpenGL: NVIDIA Corporation -- GeForce 8600M GT/PCIe/SSE2 -- 3.3.0 NVIDIA 313.09 -- texture_from_pixmap\r\n")
        //            << Pair("StartupTime", "1357769913")
        //            << Pair("FramePoisonSize", "4096")
        //            << Pair("FramePoisonBase", "7ffffffff0dea000")
        //            << Pair("Add-ons", "%7B972ce4c6-7e08-4474-a285-3208198ce6fd%7D:20.0a1,crashme%40ted.mielczarek.org:0.4")
        //            << Pair("SecondsSinceLastCrash", "1831736")
        //            << Pair("ProductName", "WaterWolf")
        //            << Pair("legacy_processing", "0")
        //            << Pair("ProductID", "{ec8030f7-c20a-464f-9b0e-13a3a9e97384}")

        ;

    // send log
    QFile logFile( TomahawkUtils::logFilePath() );
    logFile.open( QFile::ReadOnly );
    reporter.setReportData( "upload_file_tomahawklog", qCompress( logFile.readAll() ), "application/x-gzip", QFileInfo( logFile ).fileName().toUtf8());
    logFile.close();

    reporter.show();

    return app.exec();
}
