# Find VSXU - JSON handling library for Qt
#
# This module defines
#  VSXU_FOUND - whether the qsjon library was found
#  VSXU_LIBRARIES - the vsxu library
#  VSXU_INCLUDE_DIRS - the include path of the vsxu library
#

if (VSXU_INCLUDE_DIRS AND VSXU_LIBRARIES)

  # Already in cache
  set (VSXU_FOUND TRUE)

else (VSXU_INCLUDE_DIRS AND VSXU_LIBRARIES)

  if (NOT WIN32)
    # use pkg-config to get the values of VSXU_INCLUDE_DIRS
    # and VSXU_LIBRARY_DIRS to add as hints to the find commands.
    include (FindPkgConfig)
    pkg_check_modules (VSXU REQUIRED libvsxu)
  endif (NOT WIN32)

  find_library (VSXU_LIBRARIES
    NAMES
    libvsxu_engine
    PATHS
    ${VSXU_LIBRARY_DIRS}
    ${LIB_INSTALL_DIR}
    ${KDE4_LIB_DIR}
  )

  find_path (VSXU_INCLUDE_DIRS
    NAMES
    vsxu_platform.h
    PATHS
    ${VSXU_INCLUDE_DIRS}
    ${INCLUDE_INSTALL_DIR}
    ${KDE4_INCLUDE_DIR}
  )

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(VSXu DEFAULT_MSG VSXU_LIBRARIES VSXU_INCLUDE_DIRS)

  if ( UNIX AND NOT APPLE )
    set ( VSXU_LIBRARIES "${VSXU_LIBRARIES} ${VSXU_LDFLAGS}" CACHE INTERNAL "")
  endif ()

endif (VSXU_INCLUDE_DIRS AND VSXU_LIBRARIES)