# this one is important
SET(CMAKE_SYSTEM_NAME Windows)

# specify the cross compiler
SET(CMAKE_C_COMPILER i686-w64-mingw32-gcc)
SET(CMAKE_CXX_COMPILER i686-w64-mingw32-g++)

# where is the target environment containing libraries
SET(CMAKE_FIND_ROOT_PATH  /usr/i686-w64-mingw32/sys-root/mingw)
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY  ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE  ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM  NEVER)

# windres executable for application icon support
SET(WINDRES_EXECUTABLE /usr/bin/i686-w64-mingw32-windres)

# libs with broken find modules
SET(TAGLIB_FOUND true)
SET(TAGLIB_LIBRARIES  ${CMAKE_FIND_ROOT_PATH}/lib/libtag.dll.a)
SET(TAGLIB_INCLUDES   ${CMAKE_FIND_ROOT_PATH}/include/taglib)
