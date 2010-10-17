# - Find LibLastFM
# Find the liblastfm includes and the liblastfm libraries
# This module defines
# LIBLASTFM_INCLUDE_DIR, root lastfm include dir
# LIBLASTFM_LIBRARY, the path to liblastfm
# LIBLASTFM_FOUND, whether liblastfm was found


find_path(LIBLASTFM_INCLUDE_DIR NAMES Audioscrobbler
   HINTS
   ~/usr/include
   /opt/local/include
   /usr/include
   /usr/local/include
   /opt/kde4/include
   ${KDE4_INCLUDE_DIR}
   PATH_SUFFIXES lastfm
)

find_library( LIBLASTFM_LIBRARY NAMES lastfm
    PATHS
    ~/usr/lib
   /opt/local/lib
   /usr/lib
   /usr/lib64
   /usr/local/lib
   /opt/kde4/lib
   ${KDE4_LIB_DIR}
)


if(LIBLASTFM_INCLUDE_DIR AND LIBLASTFM_LIBRARY)
   set(LIBLASTFM_FOUND TRUE)
   message(STATUS "Found liblastfm: ${LIBLASTFM_INCLUDE_DIR}, ${LIBLASTFM_LIBRARY}")
else(LIBLASTFM_INCLUDE_DIR AND LIBLASTFM_LIBRARY)
   set(LIBLASTFM_FOUND FALSE)   
   if (LIBLASTFM_FIND_REQUIRED)
      message(FATAL_ERROR "Could NOT find required package LibLastFm")
   endif(LIBLASTFM_FIND_REQUIRED)
endif(LIBLASTFM_INCLUDE_DIR AND LIBLASTFM_LIBRARY)

mark_as_advanced(LIBLASTFM_INCLUDE_DIR LIBLASTFM_LIBRARY)
