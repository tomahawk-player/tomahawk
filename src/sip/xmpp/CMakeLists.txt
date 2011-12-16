project( tomahawk )

include( ${QT_USE_FILE} )
add_definitions( ${QT_DEFINITIONS} )
add_definitions( -DQT_PLUGIN )
add_definitions( -DQT_SHARED )
add_definitions( -DSIPDLLEXPORT_PRO )

<<<<<<< HEAD:src/sip/xmpp/CMakeLists.txt
set( xmppSipSources
    xmppsip.cpp
    tomahawkxmppmessage.cpp
    tomahawkxmppmessagefactory.cpp
=======
set( jabberSources
    jabber.cpp
    tomahawksipmessage.cpp
    tomahawksipmessagefactory.cpp
)
set( jabberSourcesGui
>>>>>>> origin/master:src/sip/jabber/CMakeLists.txt
    avatarmanager.cpp
    xmlconsole.cpp
)

<<<<<<< HEAD:src/sip/xmpp/CMakeLists.txt
set( xmppSipHeaders
    xmppsip.h
=======
set( jabberHeaders
    jabber.h
)
set( jabberHeadersGui
>>>>>>> origin/master:src/sip/jabber/CMakeLists.txt
    avatarmanager.h
    xmlconsole.h
)

set( xmppSipUI
    xmlconsole.ui
)

if(BUILD_GUI)
    list(APPEND jabberSources ${jabberSourcesGui})
    list(APPEND jabberHeaders ${jabberHeadersGui})
endif()

include_directories( . ${CMAKE_CURRENT_BINARY_DIR} ..
   ${QT_INCLUDE_DIR}
   ${LIBJREEN_INCLUDE_DIR}
)

qt4_wrap_ui( xmppSipUI_H ${xmppSipUI} )
qt4_wrap_cpp( xmppSipMoc ${xmppSipHeaders} )
add_library( tomahawk_sip_xmpp SHARED ${xmppSipSources} ${xmppSipMoc} ${xmppSipUI_H} ${RC_SRCS} )

IF( WIN32 )
SET( OS_SPECIFIC_LINK_LIBRARIES
    ${OS_SPECIFIC_LINK_LIBRARIES}
    "secur32.dll"
    "crypt32.dll"
    ${TOMAHAWK_LIBRARIES}
)
ENDIF( WIN32 )

target_link_libraries( tomahawk_sip_xmpp
    ${QT_LIBRARIES}
    ${LIBJREEN_LIBRARY}
    ${OS_SPECIFIC_LINK_LIBRARIES}
    tomahawklib
)

IF( APPLE )
#  SET( CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS} "-undefined dynamic_lookup" )
ENDIF( APPLE )

install( TARGETS tomahawk_sip_xmpp DESTINATION lib${LIB_SUFFIX} )

<<<<<<< HEAD:src/sip/xmpp/CMakeLists.txt

#add_subdirectory(googlewrapper)
=======
if(BUILD_GUI)
    add_subdirectory(googlewrapper)
endif()
>>>>>>> origin/master:src/sip/jabber/CMakeLists.txt
