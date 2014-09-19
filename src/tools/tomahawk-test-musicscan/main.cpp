#include"filemetadata/MusicScanner.h"

#include <QCoreApplication>
#include <QFileInfo>

#include <iostream>

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
        qDebug() << MusicScanner::readTags( pathInfo );
    }
    else if ( pathInfo.isDir() )
    {
        // TODO: Run MusicScanner recursively
    }
    else
    {
        std::cerr << "Unknown path (type) given, cannot handle this." << std::endl;
    }
}
   
