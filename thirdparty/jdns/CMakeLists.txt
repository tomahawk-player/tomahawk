PROJECT(jdns)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6 FATAL_ERROR)
SET(CMAKE_VERBOSE_MAKEFILE ON)

SET( QT_USE_QTNETWORK TRUE )
INCLUDE( ${QT_USE_FILE} )

add_definitions( ${QT_DEFINITIONS} )
add_definitions( -DQT_SHARED )

if(WIN32)
    set(PLATFORM_SPECIFIC_LIBS "ws2_32.dll" "advapi32.dll" )
endif(WIN32)

set(TOMAHAWK_JDNS_SOURCES
    jdns/jdns_util.c
    jdns/jdns_packet.c
    jdns/jdns_mdnsd.c
    jdns/jdns_sys.c
    jdns/jdns.c
    jdns/qjdns_sock.cpp
    jdns/qjdns.cpp
    jdnsshared/jdnsshared.cpp
)

set(TOMAHAWK_JDNS_HEADERS
    jdns/qjdns.h
    jdns/qjdns_helpers.h
    jdnsshared/jdnsshared.h
    jdnsshared/jdnsshared_helpers.h
)

include_directories(
    .
    ${QT_INCLUDE_DIR}
    ${QT_INCLUDES}
    jdns
)

qt4_wrap_cpp( TOMAHAWK_JDNS_MOC ${TOMAHAWK_JDNS_HEADERS} )

ADD_LIBRARY(tomahawk_jdns SHARED ${TOMAHAWK_JDNS_SOURCES} ${TOMAHAWK_JDNS_MOC})

target_link_libraries(tomahawk_jdns
    ${QT_LIBRARIES}
    ${PLATFORM_SPECIFIC_LIBS}
)

SET_TARGET_PROPERTIES( tomahawk_jdns PROPERTIES DEFINE_SYMBOL  MAKE_JDNS_LIB )

INSTALL(TARGETS tomahawk_jdns DESTINATION lib${LIB_SUFFIX})
