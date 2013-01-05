# - Try to find LibLastFm
#
#  LIBLASTFM_FOUND - system has liblastfm
#  LIBLASTFM_INCLUDE_DIRS - the liblastfm include directories
#  LIBLASTFM_LIBRARIES - link these to use liblastfm
#
# (c) Dominik Schmidt <dev@dominik-schmidt.de>
#

# Include dir
find_path(LIBLASTFM_INCLUDE_DIR
  # Track.h doesn't exist in liblastfm-0.3.1, was called Track back then
  NAMES lastfm/Track.h
  PATHS ${KDE4_INCLUDE_DIR}
)

# Finally the library itself
find_library(LIBLASTFM_LIBRARY
  NAMES lastfm
  PATHS ${KDE4_LIB_DIR}
)

set(LIBLASTFM_LIBRARIES ${LIBLASTFM_LIBRARY})
set(LIBLASTFM_INCLUDE_DIRS ${LIBLASTFM_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibLastFm DEFAULT_MSG LIBLASTFM_LIBRARIES LIBLASTFM_INCLUDE_DIRS)

mark_as_advanced(LIBLASTFM_LIBRARIES LIBLASTFM_INCLUDE_DIRS)
