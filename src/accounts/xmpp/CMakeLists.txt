project( tomahawk )

include( ${QT_USE_FILE} )
add_definitions( ${QT_DEFINITIONS} )
add_definitions( -DQT_PLUGIN )
add_definitions( -DQT_SHARED )
add_definitions( -DACCOUNTDLLEXPORT_PRO )

set( xmppAccountSources
    XmppAccount.cpp
    XmppConfigWidget.cpp
    sip/XmppSip.cpp
    sip/TomahawkXmppMessage.cpp
    sip/TomahawkXmppMessageFactory.cpp
    sip/avatarmanager.cpp
    sip/xmlconsole.cpp
    XmppInfoPlugin.cpp
)

set( xmppAccountHeaders
    XmppAccount.h
    XmppConfigWidget.h
    sip/XmppSip.h
    sip/avatarmanager.h
    sip/xmlconsole.h
    XmppInfoPlugin.h
)

set( xmppAccountUI
    XmppConfigWidget.ui
    sip/xmlconsole.ui
)

include_directories( . ${CMAKE_CURRENT_BINARY_DIR} ..
   ${QT_INCLUDE_DIR}
   ${JREEN_INCLUDE_DIR}
)

qt4_wrap_cpp( xmppAccountMoc ${xmppAccountHeaders} )
qt4_wrap_ui( xmppAccountUI_H ${xmppAccountUI} )
qt4_add_resources( RC_SRCS "resources.qrc" )
add_library( tomahawk_account_xmpp SHARED ${xmppAccountUI_H} ${xmppAccountSources} ${xmppAccountMoc} ${RC_SRCS} )

IF( WIN32 )
SET( OS_SPECIFIC_LINK_LIBRARIES
        ${OS_SPECIFIC_LINK_LIBRARIES}
        "winmm.dll"
        "iphlpapi.a"
)
ENDIF( WIN32 )

target_link_libraries( tomahawk_account_xmpp
    ${TOMAHAWK_LIBRARIES}
    ${JREEN_LIBRARIES}
    ${QT_LIBRARIES}
    ${OS_SPECIFIC_LINK_LIBRARIES}
)

IF( APPLE )
#  SET( CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS} "-undefined dynamic_lookup" )
ENDIF( APPLE )

install( TARGETS tomahawk_account_xmpp DESTINATION ${CMAKE_INSTALL_LIBDIR} )

add_subdirectory(googlewrapper)

