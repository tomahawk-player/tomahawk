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
        qt_add_dbus_interface(fdo_srcs fdonotify/org.freedesktop.Notifications.xml
                               FreedesktopNotificationsProxy)

        tomahawk_add_plugin(fdonotify
            TYPE infoplugin EXPORT_MACRO INFOPLUGINDLLEXPORT_PRO
            SOURCES "${fdo_srcs}" LINK_LIBRARIES "${FDO_LINK_LIBRARIES}"
        )
    ENDIF()

endif()

INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR} )
SET(mpris_srcs
    mpris/MprisPlugin.cpp
    )

qt_add_dbus_adaptor(mpris_srcs mpris/MprisPluginRootAdaptor.xml
                     mpris/MprisPlugin.h Tomahawk::InfoSystem::MprisPlugin MprisPluginRootAdaptor MprisPluginRootAdaptor)
qt_add_dbus_adaptor(mpris_srcs mpris/MprisPluginPlayerAdaptor.xml
                     mpris/MprisPlugin.h Tomahawk::InfoSystem::MprisPlugin MprisPluginPlayerAdaptor MprisPluginPlayerAdaptor)

tomahawk_add_plugin(mpris
    TYPE infoplugin EXPORT_MACRO INFOPLUGINDLLEXPORT_PRO
    SOURCES "${mpris_srcs}"
    )
