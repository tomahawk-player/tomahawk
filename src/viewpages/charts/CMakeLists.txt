tomahawk_add_plugin(charts
    TYPE viewpage
    EXPORT_MACRO TOMAHAWK_VIEWPAGE_EXPORT_PRO
    SOURCES
        ChartsWidget.cpp
    UI
        ChartsWidget.ui
    LINK_LIBRARIES
        ${TOMAHAWK_WIDGETS_LIBRARIES}
)
