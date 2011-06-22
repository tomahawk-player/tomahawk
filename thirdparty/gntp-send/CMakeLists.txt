cmake_minimum_required( VERSION 2.6 )

include_directories ( ${CMAKE_CURRENT_SOURCE_DIR}/gntp-send/headers )

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
        gntp-send/source/growl.c
        gntp-send/source/tcp.c
        gntp-send/source/md5.c )

add_library( tomahawk_growl SHARED ${GROWL_NTP_C_SRC} gntp-send/source/growl++.cpp )
set_target_properties( tomahawk_growl PROPERTIES COMPILE_FLAGS "-DGROWL_CPP_DLL -DGROWL_DLL" )

