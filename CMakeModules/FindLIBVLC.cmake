
# CMake module to search for LIBVLC (VLC library)
# Authors: Rohit Yadav <rohityadav89@gmail.com>
#          Harald Sitter <apachelogger@ubuntu.com>
#
# If it's found it sets LIBVLC_FOUND to TRUE
# and following variables are set:
#    LIBVLC_INCLUDE_DIR
#    LIBVLC_LIBRARY
#    LIBVLC_VERSION

if(NOT LIBVLC_MIN_VERSION)
    set(LIBVLC_MIN_VERSION "0.0")
endif(NOT LIBVLC_MIN_VERSION)

# find_path and find_library normally search standard locations
# before the specified paths. To search non-standard paths first,
# FIND_* is invoked first with specified paths and NO_DEFAULT_PATH
# and then again with no specified paths to search the default
# locations. When an earlier FIND_* succeeds, subsequent FIND_*s
# searching for the same item do nothing.

if (NOT WIN32)
    find_package(PkgConfig)
    pkg_check_modules(PC_LIBVLC libvlc)
    set(LIBVLC_DEFINITIONS ${PC_LIBVLC_CFLAGS_OTHER})
endif (NOT WIN32)

#Put here path to custom location
#example: /home/user/vlc/include etc..
find_path(LIBVLC_INCLUDE_DIR vlc/vlc.h
HINTS "$ENV{LIBVLC_INCLUDE_PATH}"
PATHS
    "$ENV{LIB_DIR}/include"
    "$ENV{LIB_DIR}/include/vlc"
    "/usr/include"
    "/usr/include/vlc"
    "/usr/local/include"
    "/usr/local/include/vlc"
    #mingw
    c:/msys/local/include
)
find_path(LIBVLC_INCLUDE_DIR PATHS "${CMAKE_INCLUDE_PATH}/vlc" NAMES vlc.h 
        HINTS ${PC_LIBVLC_INCLUDEDIR} ${PC_LIBVLC_INCLUDE_DIRS})

#Put here path to custom location
#example: /home/user/vlc/lib etc..
find_library(LIBVLC_LIBRARY NAMES vlc libvlc
HINTS "$ENV{LIBVLC_LIBRARY_PATH}" ${PC_LIBVLC_LIBDIR} ${PC_LIBVLC_LIBRARY_DIRS}
PATHS
    "$ENV{LIB_DIR}/lib"
    #mingw
    c:/msys/local/lib
)
find_library(LIBVLC_LIBRARY NAMES vlc libvlc)
find_library(LIBVLCCORE_LIBRARY NAMES vlccore libvlccore
HINTS "$ENV{LIBVLC_LIBRARY_PATH}" ${PC_LIBVLC_LIBDIR} ${PC_LIBVLC_LIBRARY_DIRS}
PATHS
    "$ENV{LIB_DIR}/lib"
    #mingw
    c:/msys/local/lib
)
find_library(LIBVLCCORE_LIBRARY NAMES vlccore libvlccore)

set(LIBVLC_VERSION ${PC_LIBVLC_VERSION})
if (NOT LIBVLC_VERSION)
# TODO: implement means to detect version on windows (vlc --version && regex? ... ultimately we would get it from a header though...)
endif (NOT LIBVLC_VERSION)

if (LIBVLC_INCLUDE_DIR AND LIBVLC_LIBRARY AND LIBVLCCORE_LIBRARY)
set(LIBVLC_FOUND TRUE)
endif (LIBVLC_INCLUDE_DIR AND LIBVLC_LIBRARY AND LIBVLCCORE_LIBRARY)

if (LIBVLC_VERSION STRLESS "${LIBVLC_MIN_VERSION}")
    message(WARNING "LibVLC version not found: version searched: ${LIBVLC_MIN_VERSION}, found ${LIBVLC_VERSION}\nUnless you are on Windows this is bound to fail.")
# TODO: only activate once version detection can be garunteed (which is currently not the case on windows)
#     set(LIBVLC_FOUND FALSE)
endif (LIBVLC_VERSION STRLESS "${LIBVLC_MIN_VERSION}")

if (LIBVLC_FOUND)
    if (NOT LIBVLC_FIND_QUIETLY)
        message(STATUS "Found LibVLC include-dir path: ${LIBVLC_INCLUDE_DIR}")
        message(STATUS "Found LibVLC library path:${LIBVLC_LIBRARY}")
        message(STATUS "Found LibVLCcore library path:${LIBVLCCORE_LIBRARY}")
        message(STATUS "Found LibVLC version: ${LIBVLC_VERSION} (searched for: ${LIBVLC_MIN_VERSION})")
    endif (NOT LIBVLC_FIND_QUIETLY)
else (LIBVLC_FOUND)
    if (LIBVLC_FIND_REQUIRED)
        message(FATAL_ERROR "Could not find LibVLC")
    endif (LIBVLC_FIND_REQUIRED)
endif (LIBVLC_FOUND)

 
