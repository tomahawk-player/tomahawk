PROJECT(libqxtweb-standalone)
CMAKE_MINIMUM_REQUIRED(VERSION 2.8 FATAL_ERROR)

IF( ${CMAKE_VERSION} VERSION_GREATER 2.8.3 )
  CMAKE_POLICY(SET CMP0017 NEW)
ENDIF( ${CMAKE_VERSION} VERSION_GREATER 2.8.3 )

setup_qt()

SET( qxtcore "core" )
SET( qxtnet "network" )
SET( qxtweb "web" )

ADD_DEFINITIONS( -Wall -O2 -DNDEBUG )
IF( NOT WIN32 )
        ADD_DEFINITIONS( -fPIC )
ENDIF()
ADD_DEFINITIONS( -DBUILD_QXT_CORE -DBUILD_QXT_WEB )


INCLUDE_DIRECTORIES( ${qxtweb} ${qxtcore} ${qxtnet} )

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
        ${qxtweb}/qhttpheader.cpp
        ${qxtweb}/qxtwebslotservice.cpp

        # Ripped bits of QxtCore:
        ${qxtcore}/qxtmetaobject.cpp
        ${qxtcore}/qxtnull.cpp
        ${qxtcore}/qxtfifo.cpp

        # QxtNetwork
        ${qxtnet}/qxtsslserver.cpp

        # automoc hack
        ${qxtcore}/qxtboundfunction.h
)


ADD_LIBRARY( qxtweb-standalone STATIC
        ${mocstuff}
        ${headers}
        ${sources}
)


target_link_libraries( qxtweb-standalone
    ${QT_LIBRARIES}
)

qt5_use_modules( qxtweb-standalone Network )

set_target_properties( qxtweb-standalone PROPERTIES AUTOMOC TRUE )
