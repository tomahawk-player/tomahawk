
get_filename_component(Tomahawk_CMAKE_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
set( TOMAHAWK_USE_FILE "${Tomahawk_CMAKE_DIR}/TomahawkUse.cmake")
set( TOMAHAWK_INCLUDE_DIRS "${Tomahawk_CMAKE_DIR}/../../../include/libtomahawk" )

# this is not how you do it but proper exporting of targets causes weird issues with cross-compiling for me
#TODO: we can easily write the install dir on configuration of this file
find_library(TOMAHAWK_LIBRARY "tomahawklib"
                PATHS
                    "${Tomahawk_CMAKE_DIR}/../../../lib/"
                    "${Tomahawk_CMAKE_DIR}/../../../lib64/"
)
set( TOMAHAWK_LIBRARIES "${TOMAHAWK_LIBRARY}" )