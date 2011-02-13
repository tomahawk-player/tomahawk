#include "tomahawk/tomahawkapp.h"

#include <exception>
int main( int argc, char *argv[] )
{
    try {
        TomahawkApp a( argc, argv );
        return a.exec();
    } catch( const std::runtime_error& e ) {
        return 0;
    }
}
