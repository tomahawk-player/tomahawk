# - Find libechonest
# Find the libechonest includes and the libechonest libraries
# This module defines
# ECHONEST_INCLUDE_DIR, root echonest include dir. Include echonest includes with echonest/foo.h
# ECHONEST_LIBRARIES, the path to libechonest
# ECHONEST_FOUND, whether libechonest was found

FIND_PACKAGE(PkgConfig QUIET)
PKG_CHECK_MODULES(PC_ECHONEST QUIET libechonest)

FIND_PATH(ECHONEST_INCLUDE_DIR NAMES echonest/Track.h
    HINTS
        ${PC_ECHONEST_INCLUDEDIR}
        ${PC_ECHONEST_INCLUDE_DIRS}
        ${CMAKE_INSTALL_INCLUDEDIR}
        ${KDE4_INCLUDE_DIR}
)

FIND_LIBRARY(ECHONEST_LIBRARIES NAMES echonest
    HINTS
        ${PC_ECHONEST_LIBDIR}
        ${PC_ECHONEST_LIBRARY_DIRS}
        ${CMAKE_INSTALL_LIBDIR}
        ${KDE4_LIB_DIR}
)

IF(ECHONEST_LIBRARIES AND ECHONEST_INCLUDE_DIR AND NOT PC_ECHONEST_VERSION)
    MESSAGE(WARNING "You don't have pkg-config and so the libechonest version check does not work!")
ENDIF()

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Echonest
                                  REQUIRED_VARS ECHONEST_LIBRARIES ECHONEST_INCLUDE_DIR
                                  VERSION_VAR PC_ECHONEST_VERSION)

MARK_AS_ADVANCED(ECHONEST_INCLUDE_DIR ECHONEST_LIBRARIES)
