include_directories(${JREEN_INCLUDE_DIR})

tomahawk_add_plugin(xmpp
    TYPE account
    EXPORT_MACRO ACCOUNTDLLEXPORT_PRO
    SOURCES
        XmppAccount.cpp
        XmppConfigWidget.cpp
        sip/XmppSip.cpp
        sip/TomahawkXmppMessage.cpp
        sip/TomahawkXmppMessageFactory.cpp
        sip/AvatarManager.cpp
        sip/XmlConsole.cpp
        XmppInfoPlugin.cpp
    UI
        XmppConfigWidget.ui
        sip/XmlConsole.ui
    LINK_LIBRARIES
        ${TOMAHAWK_LIBRARIES}
        ${JREEN_LIBRARIES}
)

add_subdirectory(googlewrapper)
