########################################################################################
# Copyright (c) 2010 Patrick von Reth <patrick.vonreth@gmail.com>                      #
#                                                                                      #
# This program is free software; you can redistribute it and/or modify it under        #
# the terms of the GNU General Public License as published by the Free Software        #
# Foundation; either version 2 of the License, or (at your option) any later           #
# version.                                                                             #
#                                                                                      #
# This program is distributed in the hope that it will be useful, but WITHOUT ANY      #
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      #
# PARTICULAR PURPOSE. See the GNU General Public License for more details.             #
#                                                                                      #
# You should have received a copy of the GNU General Public License along with         #
# this program.  If not, see <http://www.gnu.org/licenses/>.                           #
########################################################################################

# - Try to find the libsnore library
# Once done this will define
#
#  LIBSNORE_FOUND - system has the LIBSNORE library
#  LIBSNORE_LIBRARIES - The libraries needed to use LIBSNORE
#  LIBSNORE_PLUGIN_PATH - Path of the plugins

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
