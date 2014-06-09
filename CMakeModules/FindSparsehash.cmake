# - Find google-sparsehash
# Find the google-sparsehash includes
# This module defines
# SPARSEHASH_INCLUDE_DIR, root jreen include dir.
# SPARSEHASH_FOUND, whether libjreen was found

FIND_PACKAGE(PkgConfig QUIET)
PKG_CHECK_MODULES(PC_SPARSEHASH QUIET libsparsehash)

FIND_PATH(SPARSEHASH_INCLUDE_DIR NAMES google/sparsetable
    HINTS
        ${PC_SPARSEHASH_INCLUDEDIR}
        ${PC_SPARSEHASH_INCLUDE_DIRS}
        ${CMAKE_INSTALL_INCLUDEDIR}
)

IF(SPARSEHASH_INCLUDE_DIR AND NOT PC_SPARSEHASH_VERSION)
    MESSAGE(WARNING "You don't have pkg-config and so the google-sparsehash version check does not work!")
ENDIF()

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Sparsehash
    REQUIRED_VARS SPARSEHASH_INCLUDE_DIR)

MARK_AS_ADVANCED(SPARSEHASH_INCLUDE_DIR)
