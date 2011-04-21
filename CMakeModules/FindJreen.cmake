# - Find libjreen
# Find the libjreen includes and the libjreen libraries
# This module defines
# LIBJREEN_INCLUDE_DIR, root jreen include dir. Include jreen includes with jreen/foo.h
# LIBJREEN_LIBRARY, the path to libjreen
# LIBJREEN_FOUND, whether libjreen was found


find_path(LIBJREEN_INCLUDE_DIR NAMES jreen/jreen.h
   HINTS
   ~/usr/include
   /opt/local/include
   /usr/include
   /usr/local/include
   /opt/kde4/include
   ${CMAKE_INSTALL_PREFIX}/include
   ${KDE4_INCLUDE_DIR}
)

find_library( LIBJREEN_LIBRARY NAMES jreen
    PATHS
    ~/usr/lib
   /opt/local/lib
   /usr/lib
   /usr/lib64
   /usr/local/lib
   /opt/kde4/lib
   ${CMAKE_INSTALL_PREFIX}/lib
   ${CMAKE_INSTALL_PREFIX}/lib64
   ${KDE4_LIB_DIR}
)


if(LIBJREEN_INCLUDE_DIR AND LIBJREEN_LIBRARY)
   set(LIBJREEN_FOUND TRUE)
   message(STATUS "Found libjreen: ${LIBJREEN_INCLUDE_DIR}, ${LIBJREEN_LIBRARY}")
else(LIBJREEN_INCLUDE_DIR AND LIBJREEN_LIBRARY)
   set(LIBJREEN_FOUND FALSE)
   if (LIBJREEN_FIND_REQUIRED)
      message(FATAL_ERROR "Could NOT find required package libjreen")
   endif(LIBJREEN_FIND_REQUIRED)
endif(LIBJREEN_INCLUDE_DIR AND LIBJREEN_LIBRARY)

mark_as_advanced(LIBJREEN_INCLUDE_DIR LIBJREEN_LIBRARY)
