
include_directories(${QTWEETLIB_INCLUDE_DIR})

tomahawk_add_plugin(twitter
    TYPE account
    EXPORT_MACRO ACCOUNTDLLEXPORT_PRO
    SOURCES
        TwitterAccount.cpp
        TwitterInfoPlugin.cpp
        TwitterConfigWidget.cpp
        TomahawkOAuthTwitter.cpp
#        sip/TwitterSip.cpp
    UI
        TwitterConfigWidget.ui
    LINK_LIBRARIES
        ${TOMAHAWK_LIBRARIES}
        ${QTWEETLIB_LIBRARIES}
)

