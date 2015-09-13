include_directories(${LIBATTICA_INCLUDE_DIR})

tomahawk_add_plugin(whatsnew_0_8
    TYPE viewpage
    EXPORT_MACRO TOMAHAWK_VIEWPAGE_EXPORT_PRO
    SOURCES
        WhatsNew_0_8.cpp
    UI
      WhatsNewWidget_0_8.ui
    LINK_LIBRARIES
        tomahawk-widgets
)
