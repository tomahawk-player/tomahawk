PROJECT(libqxtweb-standalone)
CMAKE_MINIMUM_REQUIRED(VERSION 2.8 FATAL_ERROR)
SET(CMAKE_VERBOSE_MAKEFILE ON)
SET(CMAKE_INSTALL_PREFIX ".")

IF( ${CMAKE_VERSION} VERSION_GREATER 2.8.3 )
  CMAKE_POLICY(SET CMP0017 NEW)
ENDIF( ${CMAKE_VERSION} VERSION_GREATER 2.8.3 )

FIND_PACKAGE( Qt4 4.6.0 COMPONENTS QtCore QtNetwork REQUIRED )
set(QT_USE_QTNETWORK TRUE)
include( ${QT_USE_FILE} )

SET(qxtweb "qxtweb")

ADD_DEFINITIONS(-Wall -O2 -DNDEBUG)
IF(NOT WIN32)
	ADD_DEFINITIONS(-fPIC)
ENDIF()
ADD_DEFINITIONS( -DBUILD_QXT_CORE -DBUILD_QXT_WEB )


INCLUDE_DIRECTORIES( ${qxtweb} )

macro(create_qxtweb_fancy_header simpleHeader fancyHeader)
    file(WRITE ${CMAKE_BINARY_DIR}/QxtWeb/${fancyHeader} "#include \"${simpleHeader}\"" )
endmacro()

create_qxtweb_fancy_header("qxtabstracthttpconnector.h" "QxtHttpServerConnector")
create_qxtweb_fancy_header("qxthttpsessionmanager.h" "HttpSessionManager")
create_qxtweb_fancy_header("qxthttpsessionmanager.h" "QxtHttpSessionManager")
create_qxtweb_fancy_header("qxtwebcontent.h" "QxtWebContent")
create_qxtweb_fancy_header("qxtwebslotservice.h" "QxtWebSlotService")
create_qxtweb_fancy_header("qxtwebevent.h" "QxtWebPageEvent")

SET( sources
        # QxtWeb:
        ${qxtweb}/qxtabstracthttpconnector.cpp
        ${qxtweb}/qxtabstractwebservice.cpp
        ${qxtweb}/qxtabstractwebsessionmanager.cpp
        ${qxtweb}/qxthtmltemplate.cpp
        ${qxtweb}/qxthttpserverconnector.cpp
        ${qxtweb}/qxthttpsessionmanager.cpp
        ${qxtweb}/qxtscgiserverconnector.cpp
        ${qxtweb}/qxtwebcontent.cpp
        ${qxtweb}/qxtwebevent.cpp
        ${qxtweb}/qxtwebservicedirectory.cpp
        ${qxtweb}/qxtwebslotservice.cpp
        ${qxtweb}/qxtwebcgiservice.cpp
        ${qxtweb}/qhttpheader.cpp

        # Ripped bits of QxtCore:
        ${qxtweb}/qxtmetaobject.cpp
        ${qxtweb}/qxtnull.cpp
        ${qxtweb}/qxtfifo.cpp

        # QxtNetwork
        ${qxtweb}/qxtsslserver.cpp

        # automoc hack
        ${qxtweb}/qxtboundfunction.h
)


# DLL on windows due to linker issues, otherwise static
IF(WIN32)
	ADD_LIBRARY(qxtweb-standalone SHARED
			${mocstuff}
			${headers}
			${sources}
	)
    INSTALL( TARGETS qxtweb-standalone
             RUNTIME DESTINATION bin
             LIBRARY DESTINATION lib
             ARCHIVE DESTINATION lib
    )
ELSE()
	ADD_LIBRARY(qxtweb-standalone STATIC
			${mocstuff}
			${headers}
			${sources}
	)
ENDIF()


target_link_libraries( qxtweb-standalone
	    ${QT_LIBRARIES}
)

set_target_properties(qxtweb-standalone PROPERTIES AUTOMOC TRUE)
