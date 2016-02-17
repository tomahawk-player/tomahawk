tomahawk_add_plugin(newreleases
    TYPE viewpage
    EXPORT_MACRO TOMAHAWK_VIEWPAGE_EXPORT_PRO
    SOURCES
        NewReleasesWidget.cpp
    UI
        NewReleasesWidget.ui
    LINK_LIBRARIES
        ${TOMAHAWK_WIDGETS_LIBRARIES}
)
