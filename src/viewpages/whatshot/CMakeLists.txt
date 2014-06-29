tomahawk_add_plugin(whatshot
    TYPE viewpage
    EXPORT_MACRO TOMAHAWK_VIEWPAGE_EXPORT_PRO
    SOURCES
        WhatsHotWidget.cpp
    UI
        WhatsHotWidget.ui
    LINK_LIBRARIES
        tomahawk-widgets
)
