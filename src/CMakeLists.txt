include( ${PROJECT_BINARY_DIR}/TomahawkUse.cmake )

setup_qt()

include_directories( ${CMAKE_CURRENT_BINARY_DIR}/libtomahawk )
include_directories( ${CMAKE_CURRENT_LIST_DIR}/libtomahawk )

include_directories( ${CMAKE_CURRENT_BINARY_DIR} )
include_directories( ${CMAKE_CURRENT_LIST_DIR} )

# library
add_subdirectory( libtomahawk )
add_subdirectory( libtomahawk-widgets )
add_subdirectory( libtomahawk-playdarapi )

# plugins
add_subdirectory( accounts )
add_subdirectory( infoplugins )
add_subdirectory( viewpages )


# application
add_subdirectory( tomahawk )
add_subdirectory( crashreporter )

# tests
add_subdirectory( tests )
