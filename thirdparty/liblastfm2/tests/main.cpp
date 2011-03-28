/*
   This software is in the public domain, furnished "as is", without technical 
   support, and with no warranty, express or implied, as to its usefulness for 
   any purpose.
*/
#include <QtCore>
#include <QtTest>
#include "TestTrack.h"
#include "TestUrlBuilder.h"

int main( int argc, char** argv)
{
    QCoreApplication app( argc, argv );
    
    #define TEST( Type ) { \
        Type o; \
        if (int r = QTest::qExec( &o, argc, argv ) != 0) return r; }

    TEST( TestTrack );
    TEST( TestUrlBuilder );
    return 0;
}
