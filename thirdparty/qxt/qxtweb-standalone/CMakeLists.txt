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

        # Ripped bits of QxtCore:
        ${qxtweb}/qxtmetaobject.cpp
        ${qxtweb}/qxtnull.cpp
)

SET( headers
        # QxtWeb:
        ${qxtweb}/qxtabstracthttpconnector.h
        ${qxtweb}/qxtabstractwebservice.h
        ${qxtweb}/qxtabstractwebsessionmanager.h
        ${qxtweb}/qxtabstractwebsessionmanager_p.h
#        ${qxtweb}/qxthtmltemplate.h
        ${qxtweb}/qxthttpsessionmanager.h
#        ${qxtweb}/qxtweb.h
        ${qxtweb}/qxtwebcontent.h
#        ${qxtweb}/qxtwebevent.h
        ${qxtweb}/qxtwebservicedirectory.h
        ${qxtweb}/qxtwebservicedirectory_p.h
        ${qxtweb}/qxtwebslotservice.h
        ${qxtweb}/qxtwebcgiservice.h
        ${qxtweb}/qxtwebcgiservice_p.h

        # Ripped bits of QxtCore:
#        ${qxtweb}/qxtmetaobject.h
#        ${qxtweb}/qxtnullable.h
#        ${qxtweb}/qxtnull.h
        ${qxtweb}/qxtboundfunction.h
#        ${qxtweb}/qxtboundfunctionbase.h
#        ${qxtweb}/qxtboundcfunction.h
#        ${qxtweb}/qxtmetatype.h
)

qt4_wrap_cpp( mocstuff ${headers} )

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

# Also build small example app from qxt demos:

#qt4_wrap_cpp( mocex "example/myservice.h" )

#ADD_EXECUTABLE( example-webserver
#    ${mocex}
#    example/main.cpp
#    example/myservice.h
#    )

#TARGET_LINK_LIBRARIES( example-webserver
#    ${QT_LIBRARIES}
#    "${CMAKE_CURRENT_SOURCE_DIR}/libqxtweb-standalone.a"
#    )
