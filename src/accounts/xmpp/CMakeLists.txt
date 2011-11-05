project( tomahawk )

include( ${QT_USE_FILE} )
add_definitions( ${QT_DEFINITIONS} )
add_definitions( -DQT_PLUGIN )
add_definitions( -DQT_SHARED )
add_definitions( -DDLLEXPORT_PRO )

set( xmppAccountSources
    xmppaccount.cpp
    xmppconfigwidget.cpp
)

set( xmppAccountHeaders
    xmppaccount.h
    xmppconfigwidget.h
)

set( xmppAccountUI
    xmppconfigwidget.ui
)

include_directories( . ${CMAKE_CURRENT_BINARY_DIR} ..
   ${QT_INCLUDE_DIR}
   ${LIBJREEN_INCLUDE_DIR}
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
    tomahawk_sipxmpp
    ${TOMAHAWK_LIBRARIES}
    ${JREEN_LIBRARY}
    ${QT_LIBRARIES}
    ${OS_SPECIFIC_LINK_LIBRARIES}
)

IF( APPLE )
#  SET( CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS} "-undefined dynamic_lookup" )
ENDIF( APPLE )

install( TARGETS tomahawk_account_xmpp DESTINATION lib${LIB_SUFFIX} )
