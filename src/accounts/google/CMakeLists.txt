include_directories( ${JREEN_INCLUDE_DIR} )


# terrible hack to access ui_XmppConfigWidget.h
include_directories( ${CMAKE_CURRENT_BINARY_DIR}/../xmpp/ )

tomahawk_add_plugin(google
    TYPE account
    EXPORT_MACRO ACCOUNTDLLEXPORT_PRO
    SOURCES
        GoogleWrapper.cpp
    LINK_LIBRARIES
        tomahawk_account_xmpp
)
