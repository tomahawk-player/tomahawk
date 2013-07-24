# - Try to find the libsnore library
#  Once done this will define
#
#  LIBSNORE_FOUND - system has the LIBSNORE library
#  LIBSNORE_LIBRARIES - The libraries needed to use LIBSNORE
#  LIBSNORE_INCLUDE_DIRS - The includes needed to use LIBSNORE
#  LIBSNORE_PLUGIN_PATH - Path of the plugins
#  Copyright 2013 Patrick von Reth <vonreth@kde.org>

find_path(LIBSNORE_INCLUDE_DIR
  NAMES snore/core/snore.h
  PATHS ${KDE4_INCLUDE_DIR}
)

find_library(LIBSNORE_LIBRARY
  NAMES
  libsnore
  snore
  PATHS ${KDE4_LIB_DIR}
)

find_path(LIBSNORE_PLUGIN_PATH snoreplugins)

if(LIBSNORE_LIBRARY AND LIBSNORE_PLUGIN_PATH)
    set(LIBSNORE_PLUGIN_PATH ${LIBSNORE_PLUGIN_PATH}/snoreplugins)
endif()

set(LIBSNORE_LIBRARIES ${LIBSNORE_LIBRARY})
set(LIBSNORE_INCLUDE_DIRS ${LIBSNORE_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LIBSNORE DEFAULT_MSG LIBSNORE_LIBRARIES LIBSNORE_INCLUDE_DIRS)

mark_as_advanced(LIBSNORE_LIBRARIES LIBSNORE_INCLUDE_DIRS)
