# - Find libjreen
# Find the libjreen includes and the libjreen libraries
# This module defines
# LIBJREEN_INCLUDE_DIR, root jreen include dir. Include jreen includes with jreen/foo.h
# LIBJREEN_LIBRARY, the path to libjreen
# LIBJREEN_FOUND, whether libjreen was found

FIND_PACKAGE(PkgConfig QUIET)
if( TOMAHAWK_QT5 )
    set(JREEN_LIB_SUFFIX "-qt5")
    set(JREEN_INCLUDE_SUFFIX ${JREEN_LIB_SUFFIX})
else()
    set(JREEN_INCLUDE_SUFFIX "-qt4")
endif()
PKG_CHECK_MODULES(PC_JREEN QUIET libjreen${JREEN_LIB_SUFFIX})

FIND_PATH(JREEN_INCLUDE_DIR NAMES jreen/jreen.h
    HINTS
        ${PC_JREEN_INCLUDEDIR}
        ${PC_JREEN_INCLUDE_DIRS}
        ${CMAKE_INSTALL_INCLUDEDIR}
        ${KDE4_INCLUDE_DIR}
    PATH_SUFFIXES
        jreen${JREEN_INCLUDE_SUFFIX}
    
)

FIND_LIBRARY(JREEN_LIBRARIES NAMES jreen${JREEN_LIB_SUFFIX}
    HINTS
        ${PC_JREEN_LIBDIR}
        ${PC_JREEN_LIBRARY_DIRS}
        ${CMAKE_INSTALL_LIBDIR}
        ${KDE4_LIB_DIR}
)

IF(JREEN_LIBRARIES AND JREEN_INCLUDE_DIR AND NOT PC_JREEN_VERSION)
    MESSAGE(WARNING "You don't have pkg-config and so the Jreen version check does not work!")
ENDIF()

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Jreen
                                  REQUIRED_VARS JREEN_LIBRARIES JREEN_INCLUDE_DIR)

MARK_AS_ADVANCED(JREEN_INCLUDE_DIR JREEN_LIBRARIES)
