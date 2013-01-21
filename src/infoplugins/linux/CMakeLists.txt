# don't build with Qt5 because QX11Info is missing there
if(NOT Qt5Core_DIR)

    IF(BUILD_GUI AND X11_FOUND)
        INCLUDE_DIRECTORIES( ${THIRDPARTY_DIR}/libqnetwm )
        SET(fdo_srcs
            fdonotify/FdoNotifyPlugin.cpp
            fdonotify/ImageConverter.cpp
            ${THIRDPARTY_DIR}/libqnetwm/libqnetwm/netwm.cpp
        )
        SET(FDO_LINK_LIBRARIES ${LINK_LIBRARIES} ${X11_LIBRARIES})

        tomahawk_add_plugin(fdonotify
            TYPE infoplugin EXPORT_MACRO INFOPLUGINDLLEXPORT_PRO
            SOURCES "${fdo_srcs}" LINK_LIBRARIES "${FDO_LINK_LIBRARIES}"
        )
    ENDIF()

endif()

SET(mpris_srcs
    mpris/MprisPluginRootAdaptor.cpp
    mpris/MprisPluginPlayerAdaptor.cpp
    mpris/MprisPlugin.cpp
    )

tomahawk_add_plugin(mpris
    TYPE infoplugin EXPORT_MACRO INFOPLUGINDLLEXPORT_PRO
    SOURCES "${mpris_srcs}"
    )
