cmake_minimum_required( VERSION 2.6 )

option(WITH_STATIC    "Build static growl library"           OFF)

include_directories ( ./headers )

if(CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS                  "${CMAKE_CXX_FLAGS}  -Wall -Wextra -Wnon-virtual-dtor -fno-strict-aliasing")
  set(CMAKE_CXX_FLAGS_RELEASE          "-O2 ${CMAKE_CXX_FLAGS}")
  set(CMAKE_CXX_FLAGS_RELWITHDEBUGINFO "-g -O2 ${CMAKE_CXX_FLAGS}")
  set(CMAKE_CXX_FLAGS_DEBUG            "-g -ggdb -fno-reorder-blocks -fno-schedule-insns -fno-inline ${CMAKE_CXX_FLAGS}")
  set(CMAKE_CXX_FLAGS_DEBUGFULL        "-g3 ${CMAKE_CXX_FLAGS_DEBUG}")
endif(CMAKE_COMPILER_IS_GNUCXX)

if( WIN32 )
  link_libraries ( ws2_32 )
endif( WIN32 )

set( GROWL_NTP_C_SRC
        source/growl.c
        source/tcp.c
        source/md5.c )

add_library( growl SHARED  ${GROWL_NTP_C_SRC} )
set_target_properties( growl PROPERTIES COMPILE_FLAGS "-DGROWL_DLL" )

if( WITH_STATIC )
  add_library( growl-static  STATIC ${GROWL_NTP_C_SRC} )
  set_target_properties( growl-static PROPERTIES COMPILE_FLAGS "-DGROWL_DLL" )
endif( WITH_STATIC )

add_library( growl++ SHARED source/growl++.cpp )
target_link_libraries ( growl++  growl)
set_target_properties( growl++ PROPERTIES COMPILE_FLAGS "-DGROWL_CPP_DLL" )

if( WITH_STATIC )
add_library( growl++-static STATIC source/growl++.cpp )
target_link_libraries ( growl++-static growl)
set_target_properties( growl++-static PROPERTIES COMPILE_FLAGS "-DGROWL_CPP_DLL" )
endif( WITH_STATIC )

add_executable( gntp-send source/gntp-send.c )
target_link_libraries ( gntp-send growl)

add_executable( gntp-send++ source/gntp-send++.cpp )
target_link_libraries ( gntp-send++ growl growl++)

