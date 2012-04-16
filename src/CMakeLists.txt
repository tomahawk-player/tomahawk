PROJECT( tomahawk )
CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )

IF( "${gui}" STREQUAL "no" )
    SET( QT_DONT_USE_QTGUI TRUE )
ENDIF()

IF( UNIX AND NOT APPLE )
  SET( QT_USE_QTDBUS TRUE )
ENDIF( UNIX AND NOT APPLE )
SET( QT_USE_QTSQL TRUE )
SET( QT_USE_QTNETWORK TRUE )
SET( QT_USE_QTXML TRUE )
SET( QT_USE_QTWEBKIT TRUE )

add_definitions( -DQT_SHAREDPOINTER_TRACK_POINTERS )

INCLUDE( ${QT_USE_FILE} )
INCLUDE( AddAppIconMacro )

#SET( CMAKE_BUILD_TYPE "Release" )
SET( CMAKE_VERBOSE_MAKEFILE ON )
SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}" )
SET( CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}" )
SET( CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}" )
IF( NOT CMAKE_BUILD_TYPE STREQUAL "Release" )
    MESSAGE( "Building in debug mode, enabling all debug updates" )
    SET( DEBUG_BUILD ON )
ENDIF()

# build plugins
# use glob, but hardcoded list for now:
#FILE( GLOB plugindirs "src/plugins/*" )
#FOREACH( moddir ${plugindirs} )
#    MESSAGE( status "Building plugin: ${moddir}" )
#    ADD_SUBDIRECTORY( ${moddir} )
#ENDFOREACH( moddir )

SET( tomahawkSources ${tomahawkSources}
     web/api_v1.cpp

     musicscanner.cpp
     shortcuthandler.cpp
     scanmanager.cpp
     ubuntuunityhack.cpp
     tomahawkapp.cpp
     main.cpp
)

IF(LIBLASTFM_FOUND)
    SET(tomahawkSources ${tomahawkSources}
        scrobbler.cpp
    )
ENDIF(LIBLASTFM_FOUND)

SET( tomahawkSourcesGui ${tomahawkSourcesGui}
     sourcetree/sourcesmodel.cpp
     sourcetree/sourcesproxymodel.cpp
     sourcetree/sourcetreeview.cpp
     sourcetree/sourcedelegate.cpp
     sourcetree/animationhelper.cpp
     sourcetree/items/sourcetreeitem.cpp
     sourcetree/items/sourceitem.cpp
     sourcetree/items/playlistitems.cpp
     sourcetree/items/categoryitems.cpp
     sourcetree/items/genericpageitems.cpp
     sourcetree/items/temporarypageitem.cpp
     sourcetree/items/groupitem.cpp
     sourcetree/items/historyitem.cpp

     utils/guihelpers.cpp

     accounts/lastfm/LastFmAccount.cpp
     accounts/lastfm/LastFmConfig.cpp
     accounts/lastfm/lastfmplugin.cpp

     accounts/spotify/SpotifyAccount.cpp
     accounts/spotify/SpotifyAccountConfig.cpp
     accounts/spotify/SpotifyPlaylistUpdater.cpp

     tomahawktrayicon.cpp
     audiocontrols.cpp
     settingsdialog.cpp
     diagnosticsdialog.cpp
     AccountDelegate.cpp
     settingslistdelegate.cpp
     delegateconfigwrapper.cpp
     tomahawkwindow.cpp
     LoadXSPFDialog.cpp
     AccountFactoryWrapper.cpp
     AccountFactoryWrapperDelegate.cpp
     SocialWidget.cpp
)

IF( WITH_BREAKPAD )
    LIST(APPEND tomahawkSourcesGui breakpad/BreakPad.cpp)
ENDIF()

SET( tomahawkUI ${tomahawkUI}
     tomahawkwindow.ui
     diagnosticsdialog.ui
     stackedsettingsdialog.ui
     proxydialog.ui

     accounts/lastfm/LastFmConfig.ui
     accounts/spotify/SpotifyAccountConfig.ui

     audiocontrols.ui
     LoadXSPFDialog.ui
     AccountFactoryWrapper.ui
     SocialWidget.ui
)

INCLUDE_DIRECTORIES(
    .
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/thirdparty/liblastfm2/src

    audio
    database
    playlist
    network
    sourcetree
    topbar
    utils
    libtomahawk
    mac

    ${THIRDPARTY_DIR}/qxt/qxtweb-standalone/qxtweb
    ${THIRDPARTY_DIR}/qtweetlib/qtweetlib/src
    ${THIRDPARTY_DIR}/qtweetlib/tomahawk-custom
    ${THIRDPARTY_DIR}/breakpad

    ${TAGLIB_INCLUDES}
    ${PHONON_INCLUDES}
    ${QJSON_INCLUDE_DIR}
    ${LIBATTICA_INCLUDE_DIR}
    ${LIBECHONEST_INCLUDE_DIR}
    ${LIBECHONEST_INCLUDE_DIR}/..
)

SET( OS_SPECIFIC_LINK_LIBRARIES "" )

IF( WIN32 )
  INCLUDE( "CMakeLists.win32.cmake" )
ENDIF( WIN32 )
IF( UNIX )
  INCLUDE( "CMakeLists.unix.cmake" )
ENDIF( UNIX )

IF( APPLE )
    INCLUDE_DIRECTORIES( ${CMAKE_SOURCE_DIR}/thirdparty/SPMediaKeyTap )

    SET( tomahawkSources ${tomahawkSources} mac/tomahawkapp_mac.mm mac/macshortcuthandler.cpp )
ENDIF( APPLE )

IF(GLOOX_FOUND)
  INCLUDE_DIRECTORIES( ${GLOOX_INCLUDE_DIR} )
  SET( tomahawkSources ${tomahawkSources} xmppbot/xmppbot.cpp )
ENDIF(GLOOX_FOUND)

ADD_SUBDIRECTORY( accounts )

IF(QCA2_FOUND)
    INCLUDE_DIRECTORIES( ${QCA2_INCLUDE_DIR} )
ENDIF(QCA2_FOUND)

INCLUDE(GNUInstallDirs)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/config.h)

include( ${CMAKE_SOURCE_DIR}/lang/translations.cmake )

SET( final_src ${final_src} ${tomahawkMoc} ${tomahawkSources} ${trans_outfile})

IF( BUILD_GUI )
    LIST(APPEND tomahawkSources ${tomahawkSourcesGui})
    qt4_wrap_ui( tomahawkUI_H ${tomahawkUI} )

    IF( WITH_CRASHREPORTER )
        ADD_SUBDIRECTORY( breakpad/CrashReporter )
    ENDIF()
ENDIF()

kde4_add_app_icon( tomahawkSources "${CMAKE_SOURCE_DIR}/data/icons/tomahawk-icon-*.png" )
qt4_add_resources( RC_SRCS "../resources.qrc" )

SET( final_src ${final_src} ${tomahawkUI_H} ${tomahawkMoc} ${tomahawkSources} ${RC_SRCS} )

IF( UNIX AND NOT APPLE )
    ADD_EXECUTABLE( tomahawk ${final_src} )
ENDIF( UNIX AND NOT APPLE )
IF( APPLE )
    ADD_EXECUTABLE( tomahawk MACOSX_BUNDLE ${final_src} )
    SET_TARGET_PROPERTIES(tomahawk PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${CMAKE_BINARY_DIR}/Info.plist")
ENDIF( APPLE )
IF( WIN32 )
    ADD_EXECUTABLE( tomahawk WIN32 ${final_src} )
ENDIF( WIN32 )

SET_TARGET_PROPERTIES(tomahawk PROPERTIES AUTOMOC TRUE)

MESSAGE( STATUS "OS_SPECIFIC_LINK_LIBRARIES: ${OS_SPECIFIC_LINK_LIBRARIES}" )

SET(LINK_LIBRARIES  "")
IF(LIBLASTFM_FOUND)
    SET(LINK_LIBRARIES ${LINK_LIBRARIES} tomahawk_lastfm2 )
ENDIF(LIBLASTFM_FOUND)
IF(GLOOX_FOUND)
    SET(LINK_LIBRARIES ${LINK_LIBRARIES} ${GLOOX_LIBRARIES} )
ENDIF(GLOOX_FOUND)
IF(QCA2_FOUND)
    SET(LINK_LIBRARIES ${LINK_LIBRARIES} ${QCA2_LIBRARIES} )
ENDIF(QCA2_FOUND)
IF(WITH_BREAKPAD)
    SET(LINK_LIBRARIES ${LINK_LIBRARIES} tomahawk_breakpad)
ENDIF()

TARGET_LINK_LIBRARIES( tomahawk
    ${LINK_LIBRARIES}
    ${TOMAHAWK_LIBRARIES}
    ${PHONON_LIBS}
    ${OS_SPECIFIC_LINK_LIBRARIES}
    ${QT_LIBRARIES}
    ${MAC_EXTRA_LIBS}
    ${LIBECHONEST_LIBRARY}
    ${QXTWEB_LIBRARIES}
    ${QJSON_LIBRARIES}
    ${TAGLIB_LIBRARIES}
)

IF( APPLE )
  IF(HAVE_SPARKLE)
    MESSAGE("Sparkle Found, installing framekwork in bundle")
    INSTALL(DIRECTORY "${SPARKLE}/Versions/Current/Resources" DESTINATION "${CMAKE_BINARY_DIR}/tomahawk.app/Contents/Frameworks/Sparkle.framework")

  ENDIF(HAVE_SPARKLE)
ENDIF( APPLE )

INSTALL( TARGETS tomahawk BUNDLE DESTINATION . RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} )

IF( UNIX AND NOT APPLE AND KDE4_INSTALLED AND LEGACY_KDE_INTEGRATION ) #install protocol file
  FILE( READ ${CMAKE_SOURCE_DIR}/admin/unix/tomahawk.protocol protocol )
  STRING( REPLACE "/path/to/binary" # match this
              "${CMAKE_INSTALL_FULL_BINDIR}/tomahawk" # this is linux (kde) so pretty safe I think
              edited_protocol # save in this variable
              "${protocol}" # from the contents of this var
          )
  FILE( WRITE ${CMAKE_BINARY_DIR}/tomahawk.protocol "${edited_protocol}" )
  IF( ${SERVICES_INSTALL_DIR} )
    SET( PROTOCOL_INSTALL_DIR ${SERVICES_INSTALL_DIR} )
  ELSE()
    SET( PROTOCOL_INSTALL_DIR "${CMAKE_INSTALL_FULL_DATADIR}/kde4/services" )
  ENDIF()
  INSTALL( FILES ${CMAKE_BINARY_DIR}/tomahawk.protocol DESTINATION ${PROTOCOL_INSTALL_DIR} )
ENDIF()

#INCLUDE( "CPack.txt" )
