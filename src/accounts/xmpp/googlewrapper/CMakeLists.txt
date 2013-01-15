include_directories( ${JREEN_INCLUDE_DIR} )

tomahawk_add_plugin(google
    TYPE account
    EXPORT_MACRO ACCOUNTDLLEXPORT_PRO
    SOURCES
        ../XmppAccount.cpp
        ../XmppConfigWidget.cpp
        ../sip/XmppSip.cpp
        ../sip/TomahawkXmppMessage.cpp
        ../sip/TomahawkXmppMessageFactory.cpp
        ../sip/AvatarManager.cpp
        ../sip/XmlConsole.cpp
        ../XmppInfoPlugin.cpp
        GoogleWrapper.cpp
    LINK_LIBRARIES
        ${TOMAHAWK_LIBRARIES}
        ${JREEN_LIBRARIES}
    COMPILE_DEFINITIONS
        GOOGLE_WRAPPER
)
