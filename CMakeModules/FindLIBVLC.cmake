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
    set(LIBVLC_MIN_VERSION "2.1")
endif(NOT LIBVLC_MIN_VERSION)

find_package(PkgConfig QUIET)
pkg_check_modules(PC_LIBVLC QUIET libvlc)
set(LIBVLC_DEFINITIONS ${PC_LIBVLC_CFLAGS_OTHER})

find_path(LIBVLC_INCLUDE_DIR vlc/vlc.h
    HINTS
        ${PC_LIBVLC_INCLUDEDIR}
        ${PC_LIBVLC_INCLUDE_DIRS}
)

find_path(LIBVLCCORE_INCLUDE_DIR vlc_plugin.h
    HINTS
        ${PC_LIBVLC_INCLUDEDIR}
        ${PC_LIBVLC_INCLUDE_DIRS}
)

find_library(LIBVLC_LIBRARY NAMES vlc libvlc
    HINTS
        ${PC_LIBVLC_LIBDIR}
        ${PC_LIBVLC_LIBRARY_DIRS}
)

find_library(LIBVLCCORE_LIBRARY NAMES vlccore libvlccore
    HINTS
        ${PC_LIBVLC_LIBDIR}
        ${PC_LIBVLC_LIBRARY_DIRS}
)

set(LIBVLC_VERSION ${PC_LIBVLC_VERSION})
if (NOT LIBVLC_VERSION)
    # TODO: implement means to detect version on windows (vlc --version && regex? ... ultimately we would get it from a header though...)
endif (NOT LIBVLC_VERSION)

find_package_handle_standard_args(LibVLC DEFAULT_MSG LIBVLC_LIBRARY LIBVLCCORE_LIBRARY LIBVLC_INCLUDE_DIR)

if (LIBVLC_VERSION STRLESS "${LIBVLC_MIN_VERSION}")
    message(WARNING "LibVLC version not found: version searched: ${LIBVLC_MIN_VERSION}, found ${LIBVLC_VERSION}\nUnless you are on Windows this is bound to fail.")
# TODO: only activate once version detection can be garunteed (which is currently not the case on windows)
#     set(LIBVLC_FOUND FALSE)
endif (LIBVLC_VERSION STRLESS "${LIBVLC_MIN_VERSION}")


if (LIBVLC_FOUND)
    if (NOT LIBVLC_FIND_QUIETLY)
        message(STATUS "Found LibVLC include-dir path: ${LIBVLC_INCLUDE_DIR}")
        message(STATUS "Found LibVLC library: ${LIBVLC_LIBRARY}")
        message(STATUS "Found LibVLCcore library: ${LIBVLCCORE_LIBRARY}")
        message(STATUS "Found LibVLC version: ${LIBVLC_VERSION} (searched for: ${LIBVLC_MIN_VERSION})")
    endif (NOT LIBVLC_FIND_QUIETLY)
else (LIBVLC_FOUND)
    if (LIBVLC_FIND_REQUIRED)
        message(FATAL_ERROR "Could not find LibVLC")
    endif (LIBVLC_FIND_REQUIRED)
endif (LIBVLC_FOUND)

 
