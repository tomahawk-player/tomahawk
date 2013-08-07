tomahawk_add_plugin(networkactivity
    TYPE viewpage
    EXPORT_MACRO TOMAHAWK_VIEWPAGE_EXPORT_PRO
    SOURCES
        NetworkActivity.cpp
        NetworkActivityWidget.cpp
        NetworkActivityWorker.cpp
    UI
        NetworkActivityWidget.ui
    QT5_MODULES Concurrent
    LINK_LIBRARIES
        tomahawk-widgets
)
