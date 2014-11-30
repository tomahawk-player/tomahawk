#include"filemetadata/MusicScanner.h"

#include <QCoreApplication>
#include <QFileInfo>

#include <iostream>

#include "libtomahawk/audio/AudioOutput.h"

void
usage()
{
    std::cout << "Usage:" << std::endl;
    std::cout << "\ttomahawk-test-musicscan <path>" << std::endl;
    std::cout << std::endl;
    std::cout << "\tpath\tEither an audio file or a directory" << std::endl;
}

int
main( int argc, char* argv[] )
{
    if ( argc != 2 )
    {
        usage();
    }

    QCoreApplication a( argc, argv );
    QFileInfo pathInfo( argv[1] );

    if ( !pathInfo.exists() )
    {
        std::cerr << "Given path does not exist" << std::endl;
        exit(EXIT_FAILURE);
    }

    if ( pathInfo.isFile() )
    {
        // Start VLC instance
        AudioOutput* ao = new AudioOutput();

        qDebug() << MusicScanner::readTags( pathInfo, ao->vlcInstance() );

        // Close AudioOutput again
        delete ao;
    }
    else if ( pathInfo.isDir() )
    {
        // Register needed metatypes
        qRegisterMetaType< QDir >( "QDir" );
        qRegisterMetaType< QFileInfo >( "QFileInfo" );

        // Start VLC instance
        AudioOutput* ao = new AudioOutput();

        // Create the MusicScanner instance
        QStringList paths;
        paths << pathInfo.canonicalFilePath();
        MusicScanner scanner( MusicScanner::DirScan, paths, ao->vlcInstance() );

        // We want a dry-run of the scanner and not update any internal data.
        scanner.setDryRun( true );
        scanner.setVerbose( true );

        // Start the MusicScanner in its own thread
        QThread scannerThread( 0 );
        scannerThread.start();
        // We need to do this or the finished() signal/quit() SLOT is not called.
        scannerThread.moveToThread( &scannerThread );
        scanner.moveToThread( &scannerThread );
        QObject::connect( &scanner, SIGNAL( finished() ), &scannerThread, SLOT( quit() ) );
        QMetaObject::invokeMethod( &scanner, "scan", Qt::QueuedConnection );

        // Wait until the scanner has done its work.
        scannerThread.wait();

        // Close AudioOutput again
        delete ao;
    }
    else
    {
        std::cerr << "Unknown path (type) given, cannot handle this." << std::endl;
    }
}
