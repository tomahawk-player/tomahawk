# - Find libechonest
# Find the libechonest includes and the libechonest libraries
# This module defines
# LIBECHONEST_INCLUDE_DIR, root echonest include dir. Include echonest includes with echonest/foo.h
# LIBECHONEST_LIBRARY, the path to libechonest
# LIBECHONEST_FOUND, whether libechonest was found


find_path(LIBECHONEST_INCLUDE_DIR NAMES echonest_export.h
   HINTS
   ~/usr/include
   /opt/local/include
   /usr/include
   /usr/local/include
   /opt/kde4/include
   ${KDE4_INCLUDE_DIR}
   PATH_SUFFIXES echonest
)

find_library( LIBECHONEST_LIBRARY NAMES echonest
    PATHS
    ~/usr/lib
   /opt/local/lib
   /usr/lib
   /usr/lib64
   /usr/local/lib
   /opt/kde4/lib
   ${KDE4_LIB_DIR}
)


if(LIBECHONEST_INCLUDE_DIR AND LIBECHONEST_LIBRARY)
   set(LIBECHONEST_FOUND TRUE)
   message(STATUS "Found libechonest: ${LIBECHONEST_INCLUDE_DIR}, ${LIBECHONEST_LIBRARY}")
else(LIBECHONEST_INCLUDE_DIR AND LIBECHONEST_LIBRARY)
   set(LIBECHONEST_FOUND FALSE)   
   if (LIBECHONEST_FIND_REQUIRED)
      message(FATAL_ERROR "Could NOT find required package libechonest")
   endif(LIBECHONEST_FIND_REQUIRED)
endif(LIBECHONEST_INCLUDE_DIR AND LIBECHONEST_LIBRARY)

mark_as_advanced(LIBECHONEST_INCLUDE_DIR LIBECHONEST_LIBRARY)
