SET(fdo_srcs
    fdonotify/FdoNotifyPlugin.cpp
    fdonotify/ImageConverter.cpp
)
SET(FDO_LINK_LIBRARIES ${LINK_LIBRARIES})
qt5_add_dbus_interface(fdo_srcs fdonotify/org.freedesktop.Notifications.xml
                       FreedesktopNotificationsProxy)

tomahawk_add_plugin(fdonotify
    TYPE infoplugin EXPORT_MACRO INFOPLUGINDLLEXPORT_PRO
    SOURCES "${fdo_srcs}" LINK_LIBRARIES "${FDO_LINK_LIBRARIES}"
)

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
