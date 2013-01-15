
tomahawk_add_plugin(zeroconf
    TYPE account
    EXPORT_MACRO ACCOUNTDLLEXPORT_PRO
    SOURCES
        TomahawkZeroconf.h
        Zeroconf.cpp
        ZeroconfAccount.cpp
    UI
        ConfigWidget.ui
    LINK_LIBRARIES
        ${TOMAHAWK_LIBRARIES}
)
