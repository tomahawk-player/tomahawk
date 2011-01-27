PROJECT(jdns)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6 FATAL_ERROR)
SET(CMAKE_VERBOSE_MAKEFILE ON)

SET( QT_USE_QTNETWORK TRUE )
INCLUDE( ${QT_USE_FILE} )

if(WIN32)
    SET(PLATFORM_SPECIFIC_LIBS "ws2_32.dll" "advapi32.dll" )
endif(WIN32)

set(JDNS_SOURCES
    jdns_util.c
    jdns_packet.c
    jdns_mdnsd.c
    jdns_sys.c
    jdns.c
    qjdns_sock.cpp
    qjdns.cpp
)

set(JDNS_HEADERS
    qjdns.h
)

include_directories(
    ${QT_INCLUDE_DIR}
    ${QT_INCLUDES}
)

qt4_wrap_cpp( JDNS_MOC ${JDNS_HEADERS} )

if(WIN32)
    ADD_LIBRARY(jdns SHARED ${JDNS_SOURCES} ${JDNS_MOC})
else()
    ADD_LIBRARY(jdns STATIC ${JDNS_SOURCES} ${JDNS_MOC})
endif()

target_link_libraries(jdns
    ${QT_LIBRARIES}
)

INSTALL(TARGETS jdns ARCHIVE DESTINATION lib)
