setup_qt()

add_subdirectory( libtomahawk )

include_directories( ${CMAKE_CURRENT_BINARY_DIR}/libtomahawk )
include_directories( ${CMAKE_CURRENT_LIST_DIR}/libtomahawk )

add_subdirectory( tomahawk )

add_subdirectory( breakpad/CrashReporter/ )

add_subdirectory( accounts )
add_subdirectory( infoplugins )