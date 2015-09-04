FILE( GLOB _icons "${CMAKE_SOURCE_DIR}/data/icons/tomahawk-icon-*.png" )
FOREACH( _file ${_icons} )
 STRING( REPLACE "${CMAKE_SOURCE_DIR}/data/icons/tomahawk-icon-" "" _res ${_file} )
 STRING( REPLACE ".png" "" _res ${_res} )
 INSTALL( FILES ${_file} RENAME tomahawk.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/${_res}/apps )
ENDFOREACH( _file )

IF( WITH_UPOWER )
    qt_add_dbus_interface(tomahawkSources "${CMAKE_CURRENT_SOURCE_DIR}/linux/org.freedesktop.UPower.xml" "UPowerProxy")
    SET( tomahawkSources ${tomahawkSources} linux/UPowerHandler.cpp  )
ENDIF( WITH_UPOWER )

IF( WITH_GNOMESHORTCUTHANDLER )
    qt_add_dbus_interface(tomahawkSources "${CMAKE_CURRENT_SOURCE_DIR}/linux/GnomeSettingsDaemonMediaKeys.xml" "GnomeSettingsDaemonMediaKeysProxy")
    SET( tomahawkSources ${tomahawkSources} linux/GnomeShortcutHandler.cpp )
ENDIF( WITH_GNOMESHORTCUTHANDLER )

INSTALL( FILES ${CMAKE_SOURCE_DIR}/data/icons/tomahawk-icon.svg RENAME tomahawk.svg  DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps )

INSTALL( FILES ${CMAKE_SOURCE_DIR}/admin/unix/tomahawk.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications )
