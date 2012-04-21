
# fake google plugin

set( googleHeaders
    ../XmppAccount.h
    ../XmppConfigWidget.h
    ../sip/XmppSip.h
    ../sip/avatarmanager.h
    ../sip/xmlconsole.h
    ../XmppInfoPlugin.h
    googlewrapper.h )

set( googleSources
    ../XmppAccount.cpp
    ../XmppConfigWidget.cpp
    ../sip/XmppSip.cpp
    ../sip/TomahawkXmppMessage.cpp
    ../sip/TomahawkXmppMessageFactory.cpp
    ../sip/avatarmanager.cpp
    ../sip/xmlconsole.cpp
    ../XmppInfoPlugin.cpp

    googlewrapper.cpp )

add_definitions(-DGOOGLE_WRAPPER)

qt4_add_resources( RCX_SRCS "resources.qrc" )

qt4_wrap_cpp( googleMoc ${googleHeaders} )
add_library( tomahawk_account_google SHARED ${googleSources} ${googleMoc} ${googleMoc} ${RCX_SRCS} )

include_directories( ${JREEN_INCLUDE_DIR} )

target_link_libraries( tomahawk_account_google
    ${QT_LIBRARIES}
    ${JREEN_LIBRARIES}
    ${OS_SPECIFIC_LINK_LIBRARIES}
    tomahawklib
)

install( TARGETS tomahawk_account_google DESTINATION ${CMAKE_INSTALL_LIBDIR} )
