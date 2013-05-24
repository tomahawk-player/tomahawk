ADD_DEFINITIONS( -ggdb )
ADD_DEFINITIONS( -Wall )
ADD_DEFINITIONS( -g )
ADD_DEFINITIONS( -fno-operator-names )
ADD_DEFINITIONS( -fPIC )

IF( APPLE )
    INCLUDE( "CMakeLists.osx.cmake" )
ENDIF( APPLE )

IF( UNIX AND NOT APPLE )
    INCLUDE( "CMakeLists.linux.cmake" )
ENDIF( UNIX AND NOT APPLE )
