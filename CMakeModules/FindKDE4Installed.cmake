# Simple hack to detect wether KDE4 is *installed* -- not anything about the development environment!

FILE(TO_CMAKE_PATH "$ENV{KDEDIRS}" _KDEDIRS)

# For KDE4 kde-config has been renamed to kde4-config
FIND_PROGRAM(KDE4_KDECONFIG_EXECUTABLE NAMES kde4-config
   # the suffix must be used since KDEDIRS can be a list of directories which don't have bin/ appended
   PATH_SUFFIXES bin               
   HINTS
   ${CMAKE_INSTALL_PREFIX}
   ${_KDEDIRS}
   /opt/kde4
   ONLY_CMAKE_FIND_ROOT_PATH
   )

IF (KDE4_KDECONFIG_EXECUTABLE)
   SET (KDE4_INSTALLED TRUE)
   message(STATUS "KDE4 is installed, will install protocol file")
ENDIF (KDE4_KDECONFIG_EXECUTABLE)

