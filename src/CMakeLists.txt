include( ../TomahawkUse.cmake )

setup_qt()

include_directories( ${CMAKE_CURRENT_BINARY_DIR}/libtomahawk )
include_directories( ${CMAKE_CURRENT_LIST_DIR}/libtomahawk )

# library
add_subdirectory(libtomahawk)

# plugins
add_subdirectory(accounts)
add_subdirectory(infoplugins)


# application
add_subdirectory(tomahawk)
add_subdirectory(crashreporter)