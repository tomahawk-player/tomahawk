# Try to find the Attica library
# Once done this will define
#
#   LIBATTICA_FOUND          Indicates that Attica was found
#   LIBATTICA_LIBRARIES      Libraries needed to use Attica
#   LIBATTICA_LIBRARY_DIRS   Paths needed for linking against Attica
#   LIBATTICA_INCLUDE_DIR    Path needed for finding Attica include files
#
# The minimum required version of LibAttica can be specified using the
# standard syntax, e.g. find_package(LibAttica 0.20)

# Copyright (c) 2009 Frederik Gladhorn <gladhorn@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.

# Support LIBATTICA_MIN_VERSION for compatibility:
IF(NOT LibAttica_FIND_VERSION)
  SET(LibAttica_FIND_VERSION "${LIBATTICA_MIN_VERSION}")
ENDIF(NOT LibAttica_FIND_VERSION)

# the minimum version of LibAttica we require
IF(NOT LibAttica_FIND_VERSION)
  SET(LibAttica_FIND_VERSION "0.1.0")
ENDIF(NOT LibAttica_FIND_VERSION)


IF (NOT WIN32)
   # use pkg-config to get the directories and then use these values
   # in the FIND_PATH() and FIND_LIBRARY() calls
   FIND_PACKAGE(PkgConfig)
   PKG_CHECK_MODULES(PC_LIBATTICA QUIET libattica)
   SET(LIBATTICA_DEFINITIONS ${PC_ATTICA_CFLAGS_OTHER})
ENDIF (NOT WIN32)

FIND_PATH(LIBATTICA_INCLUDE_DIR attica/provider.h
   HINTS
   ${PC_LIBATTICA_INCLUDEDIR}
   ${PC_LIBATTICA_INCLUDE_DIRS}
   PATH_SUFFIXES attica
   )

# Store the version number in the cache, so we don't have to search everytime:
IF(LIBATTICA_INCLUDE_DIR  AND NOT  LIBATTICA_VERSION)
  FILE(READ ${LIBATTICA_INCLUDE_DIR}/attica/version.h LIBATTICA_VERSION_CONTENT)
  STRING (REGEX MATCH "LIBATTICA_VERSION_STRING \".*\"\n" LIBATTICA_VERSION_MATCH "${LIBATTICA_VERSION_CONTENT}")
  IF(LIBATTICA_VERSION_MATCH)
    STRING(REGEX REPLACE "LIBATTICA_VERSION_STRING \"(.*)\"\n" "\\1" _LIBATTICA_VERSION ${LIBATTICA_VERSION_MATCH})
  ENDIF(LIBATTICA_VERSION_MATCH)
  SET(LIBATTICA_VERSION "${_LIBATTICA_VERSION}" CACHE STRING "Version number of LibAttica" FORCE)
ENDIF(LIBATTICA_INCLUDE_DIR  AND NOT  LIBATTICA_VERSION)


FIND_LIBRARY(LIBATTICA_LIBRARIES NAMES attica libattica
   HINTS
   ${PC_LIBATTICA_LIBDIR}
   ${PC_LIBATTICA_LIBRARY_DIRS}
   )

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LibAttica  REQUIRED_VARS LIBATTICA_LIBRARIES LIBATTICA_INCLUDE_DIR
                                             VERSION_VAR LIBATTICA_VERSION)

MARK_AS_ADVANCED(LIBATTICA_INCLUDE_DIR LIBATTICA_LIBRARIES)
