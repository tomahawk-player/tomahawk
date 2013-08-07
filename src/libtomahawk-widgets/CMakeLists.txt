
set(TOMAHAWK_WIDGETS_LIBRARY_TARGET tomahawk-widgets)

list(APPEND ${TOMAHAWK_WIDGETS_LIBRARY_TARGET}_SOURCES
    PlaylistDelegate.cpp
    PlaylistWidget.cpp
    SocialPlaylistWidget.cpp
)

list(APPEND ${TOMAHAWK_WIDGETS_LIBRARY_TARGET}_UI
    SocialPlaylistWidget.ui
)

tomahawk_add_library(${TOMAHAWK_WIDGETS_LIBRARY_TARGET}
    SOURCES ${${TOMAHAWK_WIDGETS_LIBRARY_TARGET}_SOURCES}
    UI ${${TOMAHAWK_WIDGETS_LIBRARY_TARGET}_UI}
    EXPORT TomahawkLibraryDepends
    VERSION ${TOMAHAWK_VERSION_SHORT}
)
