# this one is important
SET(CMAKE_SYSTEM_NAME Windows)

# specify the cross compiler
SET(CMAKE_C_COMPILER i686-pc-mingw32-gcc)
SET(CMAKE_CXX_COMPILER i686-pc-mingw32-g++)

# where is the target environment containing libraries
SET(CMAKE_FIND_ROOT_PATH  /usr/i686-pc-mingw32/sys-root/mingw)

# windres executable for application icon support
set(WINDRES_EXECUTABLE /usr/bin/i686-pc-mingw32-windres)


# libs with broken find modules
set(TAGLIB_FOUND true)
set(GLOOX_LIBS  ${CMAKE_FIND_ROOT_PATH}/lib/libgloox.dll.a)

