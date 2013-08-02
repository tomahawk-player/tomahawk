
set(TOMAHAWK_WIDGETS_LIBRARY_TARGET tomahawk-widgets)

list(APPEND ${TOMAHAWK_WIDGETS_LIBRARY_TARGET}_SOURCES
    PlaylistDelegate.cpp
    PlaylistWidget.cpp

    # ViewPages
    NetworkActivityWidget.cpp
    NetworkActivityWorker.cpp
    SocialPlaylistWidget.cpp
)

list(APPEND ${TOMAHAWK_WIDGETS_LIBRARY_TARGET}_UI
    NetworkActivityWidget.ui
    SocialPlaylistWidget.ui
)

tomahawk_add_library(${TOMAHAWK_WIDGETS_LIBRARY_TARGET}
    SOURCES ${${TOMAHAWK_WIDGETS_LIBRARY_TARGET}_SOURCES}
    UI ${${TOMAHAWK_WIDGETS_LIBRARY_TARGET}_UI}
    QT5_MODULES Concurrent
    EXPORT TomahawkLibraryDepends
    VERSION ${TOMAHAWK_VERSION_SHORT}
)
