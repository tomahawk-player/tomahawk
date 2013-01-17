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
     web/Api_v1.cpp

     AclRegistryImpl.cpp
     ShortcutHandler.cpp
     UbuntuUnityHack.cpp
     TomahawkApp.cpp
     main.cpp
)

IF(LIBLASTFM_FOUND)
    SET(tomahawkSources ${tomahawkSources}
        Scrobbler.cpp
    )
ENDIF(LIBLASTFM_FOUND)

SET( tomahawkSourcesGui ${tomahawkSourcesGui}
     sourcetree/SourcesModel.cpp
     sourcetree/SourcesProxyModel.cpp
     sourcetree/SourceTreeView.cpp
     sourcetree/SourceDelegate.cpp
     sourcetree/AnimationHelper.cpp
     sourcetree/items/SourceTreeItem.cpp
     sourcetree/items/SourceItem.cpp
     sourcetree/items/PlaylistItems.cpp
     sourcetree/items/CategoryItems.cpp
     sourcetree/items/GenericPageItems.cpp
     sourcetree/items/LovedTracksItem.cpp
     sourcetree/items/TemporaryPageItem.cpp
     sourcetree/items/GroupItem.cpp
     sourcetree/items/HistoryItem.cpp

     TomahawkTrayIcon.cpp
     AudioControls.cpp
     SettingsDialog.cpp
     DiagnosticsDialog.cpp
     TomahawkWindow.cpp
     LoadXSPFDialog.cpp
     SocialWidget.cpp

     widgets/ContainedMenuButton.cpp
     widgets/AccountListView.cpp
     widgets/AccountListWidget.cpp
     widgets/AccountModelFactoryProxy.cpp
     widgets/AccountWidget.cpp
     widgets/AccountsPopupWidget.cpp
     widgets/AccountsToolButton.cpp
     widgets/SlideSwitchButton.cpp
     widgets/UnstyledFrame.cpp
)

IF( WITH_BREAKPAD )
    LIST(APPEND tomahawkSourcesGui breakpad/BreakPad.cpp)
ENDIF()

SET( tomahawkUI ${tomahawkUI}
     TomahawkWindow.ui
     DiagnosticsDialog.ui
     Settings_Accounts.ui
     Settings_Advanced.ui
     Settings_Collection.ui
     ProxyDialog.ui

     AudioControls.ui
     LoadXSPFDialog.ui
     SocialWidget.ui
)

INCLUDE_DIRECTORIES(
    .
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/libtomahawk

    sourcetree
    libtomahawk
    mac

    ${THIRDPARTY_DIR}/qxt/qxtweb-standalone/qxtweb
    ${THIRDPARTY_DIR}/breakpad

    ${TAGLIB_INCLUDES}
    ${PHONON_INCLUDES}
    ${QJSON_INCLUDE_DIR}
    ${LIBATTICA_INCLUDE_DIR}
    ${ECHONEST_INCLUDE_DIR}
    ${LIBLASTFM_INCLUDE_DIRS}
)

SET( OS_SPECIFIC_LINK_LIBRARIES "" )

IF( WIN32 )
  INCLUDE( "CMakeLists.win32.cmake" )
ENDIF( WIN32 )
IF( UNIX )
  INCLUDE( "CMakeLists.unix.cmake" )
ENDIF( UNIX )

IF( APPLE )
    SET( CMAKE_LINKER_FLAGS "-headerpad_max_install_names ${CMAKE_LINKER_FLAGS}" )

    INCLUDE_DIRECTORIES( ${CMAKE_SOURCE_DIR}/thirdparty/SPMediaKeyTap )

    SET( tomahawkSources ${tomahawkSources} mac/TomahawkApp_Mac.mm mac/MacShortcutHandler.cpp )
ENDIF( APPLE )

ADD_SUBDIRECTORY( accounts )
ADD_SUBDIRECTORY( infoplugins )

IF(QCA2_FOUND)
    INCLUDE_DIRECTORIES( ${QCA2_INCLUDE_DIR} )
ENDIF(QCA2_FOUND)

INCLUDE(GNUInstallDirs)

# currently only in libtomahawk, we might want to properly split what's in which config file
# configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Config.h.in
#                ${CMAKE_CURRENT_BINARY_DIR}/config.h)
# configure_file(${CMAKE_CURRENT_SOURCE_DIR}/TomahawkVersion.h.in
#                ${CMAKE_CURRENT_BINARY_DIR}/TomahawkVersion.h)

# translations
include( ${CMAKE_SOURCE_DIR}/lang/translations.cmake )
add_tomahawk_translations( ${TOMAHAWK_TRANSLATION_LANGUAGES} )

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
    SET( LINK_LIBRARIES ${LINK_LIBRARIES} ${LIBLASTFM_LIBRARIES} )
ENDIF(LIBLASTFM_FOUND)
IF(QCA2_FOUND)
    SET( LINK_LIBRARIES ${LINK_LIBRARIES} ${QCA2_LIBRARIES} )
ENDIF(QCA2_FOUND)
IF(WITH_BREAKPAD)
    SET( LINK_LIBRARIES ${LINK_LIBRARIES} tomahawk_breakpad )
ENDIF()

TARGET_LINK_LIBRARIES( tomahawk
    ${LINK_LIBRARIES}
    ${TOMAHAWK_LIBRARIES}
    ${PHONON_LIBS}
    ${OS_SPECIFIC_LINK_LIBRARIES}
    ${QT_LIBRARIES}
    ${MAC_EXTRA_LIBS}
    ${ECHONEST_LIBRARIES}
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
