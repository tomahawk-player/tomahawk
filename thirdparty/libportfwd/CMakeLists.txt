PROJECT(libportfwd)
CMAKE_MINIMUM_REQUIRED(VERSION 2.6 FATAL_ERROR)
SET(CMAKE_VERBOSE_MAKEFILE ON)
#SET(CMAKE_INSTALL_PREFIX ".")

SET(MINIUPNP_DIR    "third-party/miniupnpc-1.6/")
SET(NATPMP_DIR      "third-party/libnatpmp-20100808")

ADD_DEFINITIONS(-Wall -O2 -DNDEBUG)
IF(WIN32)
	ADD_DEFINITIONS(-DWIN32 -DMINIUPNP_EXPORTS )
ELSE()
	ADD_DEFINITIONS(-fPIC)
ENDIF()
INCLUDE_DIRECTORIES( ${MINIUPNP_DIR} include ${QT_INCLUDES} )
SET( CMAKE_C_FLAGS ${CLEAN_C_FLAGS} )

ADD_DEFINITIONS( -DPORTFWDDLLEXPORT_PRO )
if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  ADD_DEFINITIONS( -DMACOSX -D_DARWIN_C_SOURCE )
endif ()

ADD_LIBRARY(tomahawk_portfwd SHARED
        # the needed bits of miniupnpc (no python module, no tests, no cli)
        ${MINIUPNP_DIR}/connecthostport.c
        ${MINIUPNP_DIR}/igd_desc_parse.c
        ${MINIUPNP_DIR}/minisoap.c
        ${MINIUPNP_DIR}/minissdpc.c
        ${MINIUPNP_DIR}/miniupnpc.c
        ${MINIUPNP_DIR}/miniwget.c
        ${MINIUPNP_DIR}/minixml.c
        ${MINIUPNP_DIR}/minixmlvalid.c
        ${MINIUPNP_DIR}/portlistingparse.c
        ${MINIUPNP_DIR}/receivedata.c
        #${MINIUPNP_DIR}/minixmlvalid.c - contains main() like upnp.c, so building a shared lib fails
        #${MINIUPNP_DIR}/upnpc.c
        ${MINIUPNP_DIR}/upnpcommands.c
        ${MINIUPNP_DIR}/upnperrors.c
        ${MINIUPNP_DIR}/upnpreplyparse.c

        # Our wrapper:
        src/portfwd.cpp
)

IF(WIN32)
	TARGET_LINK_LIBRARIES( tomahawk_portfwd "ws2_32.dll" "iphlpapi.a" )
ENDIF()

INSTALL( TARGETS tomahawk_portfwd
    EXPORT TomahawkLibraryDepends
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    BUNDLE DESTINATION library
)

#INSTALL(FILES include/portfwd/portfwddllmacro.h include/portfwd/portfwd.h DESTINATION include/portfwd)

#ADD_EXECUTABLE(portfwd-demo
#               src/main.cpp
#              )
#TARGET_LINK_LIBRARIES(portfwd-demo portfwd)

#INSTALL(TARGETS portfwd ARCHIVE DESTINATION lib${LIB_SUFFIX})
#INSTALL(TARGETS portfwd-demo RUNTIME DESTINATION bin)
#INSTALL(DIRECTORY include/portfwd DESTINATION include PATTERN "*~" EXCLUDE)
