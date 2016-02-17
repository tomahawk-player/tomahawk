tomahawk_add_plugin(dashboard
    TYPE viewpage
    EXPORT_MACRO TOMAHAWK_VIEWPAGE_EXPORT_PRO
    SOURCES
        Dashboard.cpp
    UI
        DashboardWidget.ui
    LINK_LIBRARIES
        ${TOMAHAWK_WIDGETS_LIBRARIES}
)
