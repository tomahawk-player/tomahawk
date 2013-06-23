include_directories(${TELEPATHY_QT_INCLUDE_DIR})

tomahawk_add_plugin(telepathy
    TYPE configstorage
    EXPORT_MACRO CONFIGSTORAGEDLLEXPORT_PRO
    SOURCES
        TelepathyConfigStorage.cpp
        TelepathyConfigStorageConfigWidgetPlugin.cpp
    LINK_LIBRARIES
        ${TOMAHAWK_LIBRARIES}
        ${TELEPATHY_QT_LIBRARIES}
    SHARED_LIB
)

if( KDE4_FOUND )
    include_directories( ${KDE4_INCLUDES} )
    tomahawk_add_plugin(kde
        TYPE configstorage_telepathy
        EXPORT_MACRO CONFIGSTORAGETELEPATHYDLLEXPORT_PRO
        SOURCES
            KdeTelepathyConfigWidget.cpp
        LINK_LIBRARIES
            tomahawk_configstorage_telepathy
            ${KDE4_KCMUTILS_LIBS}
    )
endif()
