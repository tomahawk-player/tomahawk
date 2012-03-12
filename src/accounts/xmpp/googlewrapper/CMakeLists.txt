
# fake google plugin

set( googleHeaders
    ../xmppaccount.h
    ../xmppconfigwidget.h
    ../sip/xmppsip.h
    ../sip/avatarmanager.h
    ../sip/xmlconsole.h
    googlewrapper.h )

set( googleSources
    ../xmppaccount.cpp
    ../xmppconfigwidget.cpp
    ../sip/xmppsip.cpp
    ../sip/tomahawkxmppmessage.cpp
    ../sip/tomahawkxmppmessagefactory.cpp
    ../sip/avatarmanager.cpp
    ../sip/xmlconsole.cpp

    googlewrapper.cpp )

add_definitions(-DGOOGLE_WRAPPER)

qt4_add_resources( RCX_SRCS "resources.qrc" )

qt4_wrap_cpp( googleMoc ${googleHeaders} )
add_library( tomahawk_account_google SHARED ${googleSources} ${googleMoc} ${googleMoc} ${RCX_SRCS} )

target_link_libraries( tomahawk_account_google
    ${QT_LIBRARIES}
    ${LIBJREEN_LIBRARY}
    ${OS_SPECIFIC_LINK_LIBRARIES}
    tomahawklib
)

install( TARGETS tomahawk_account_google DESTINATION ${CMAKE_INSTALL_LIBDIR} )
