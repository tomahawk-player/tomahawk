include_directories(${TELEPATHY_QT_INCLUDE_DIR})

tomahawk_add_plugin(telepathy
    TYPE configstorage
    EXPORT_MACRO CONFIGSTORAGEDLLEXPORT_PRO
    SOURCES
        TelepathyConfigStorage.cpp
    LINK_LIBRARIES
        ${TOMAHAWK_LIBRARIES}
        ${TELEPATHY_QT_LIBRARIES}
)
