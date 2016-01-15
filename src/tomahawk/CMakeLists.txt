include( ECMAddAppIcon )

# SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}" )
# SET( CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}" )
# SET( CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}" )
IF( NOT CMAKE_BUILD_TYPE STREQUAL "Release" AND NOT CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    MESSAGE( "Building in debug mode, enabling all debug updates" )
ENDIF()

set(TOMAHAWK_APPLICATION_TARGET ${TOMAHAWK_TARGET_NAME}-bin)

# build plugins
# use glob, but hardcoded list for now:
#FILE( GLOB plugindirs "src/plugins/*" )
#FOREACH( moddir ${plugindirs} )
#    MESSAGE( status "Building plugin: ${moddir}" )
#    ADD_SUBDIRECTORY( ${moddir} )
#ENDFOREACH( moddir )

SET( tomahawkSources ${tomahawkSources}
     AclRegistryImpl.cpp
     ShortcutHandler.cpp
     UbuntuUnityHack.cpp
     TomahawkApp.cpp
     main.cpp
)

IF( LIBLASTFM_FOUND )
    SET(tomahawkSources ${tomahawkSources}
        Scrobbler.cpp
    )
ENDIF( LIBLASTFM_FOUND )

SET( tomahawkSourcesGui ${tomahawkSourcesGui}
     dialogs/DiagnosticsDialog.cpp
     dialogs/LoadPlaylistDialog.cpp
     dialogs/SettingsDialog.cpp

     sourcetree/SourcesModel.cpp
     sourcetree/SourcesProxyModel.cpp
     sourcetree/SourceTreeView.cpp
     sourcetree/SourceDelegate.cpp
     sourcetree/items/ScriptCollectionItem.cpp
     sourcetree/items/SourceTreeItem.cpp
     sourcetree/items/SourceItem.cpp
     sourcetree/items/PlaylistItems.cpp
     sourcetree/items/CategoryItems.cpp
     sourcetree/items/GenericPageItems.cpp
     sourcetree/items/LovedTracksItem.cpp
     sourcetree/items/TemporaryPageItem.cpp
     sourcetree/items/GroupItem.cpp
     sourcetree/items/CollectionItem.cpp
     sourcetree/items/HistoryItem.cpp
     sourcetree/items/InboxItem.cpp
     sourcetree/items/QueueItem.cpp

     TomahawkTrayIcon.cpp
     AudioControls.cpp
     TomahawkWindow.cpp

     widgets/ContainedMenuButton.cpp
     widgets/AccountListView.cpp
     widgets/AccountListWidget.cpp
     widgets/AccountModelFactoryProxy.cpp
     widgets/AccountWidget.cpp
     widgets/AccountsPopupWidget.cpp
     widgets/AccountsToolButton.cpp
     widgets/SlideSwitchButton.cpp
     widgets/SocialWidget.cpp
     widgets/SplashWidget.cpp
)

SET( tomahawkUI ${tomahawkUI}
     dialogs/DiagnosticsDialog.ui
     dialogs/HostDialog.ui
     dialogs/LoadPlaylistDialog.ui
     dialogs/ProxyDialog.ui
     dialogs/Settings_Accounts.ui
     dialogs/Settings_Advanced.ui
     dialogs/Settings_Collection.ui
     dialogs/Settings_Downloads.ui

     TomahawkWindow.ui

     AudioControls.ui

     widgets/SocialWidget.ui
)

INCLUDE_DIRECTORIES(
    .
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/../libtomahawk

    sourcetree
    ../libtomahawk
    mac

    ${THIRDPARTY_DIR}/libcrashreporter-qt/src/
    ${THIRDPARTY_DIR}/kdsingleapplicationguard/

    ${LIBATTICA_INCLUDE_DIR}
    ${ECHONEST_INCLUDE_DIR}
    ${LIBLASTFM_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIR}
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

IF( QCA2_FOUND )
    INCLUDE_DIRECTORIES( ${QCA2_INCLUDE_DIR} )
ENDIF( QCA2_FOUND )

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
    qt_wrap_ui( tomahawkUI_H ${tomahawkUI} )
ENDIF()

file( GLOB_RECURSE TOMAHAWK_ICONS "${CMAKE_SOURCE_DIR}/data/icons/*-tomahawk-icon.png" )
if(APPLE)
    # OUTFILE is currently only supported in my branch, see https://git.reviewboard.kde.org/r/126303/
    # once merged (and released), we can remove this conditional usage
    ecm_add_app_icon(tomahawkSources ICONS ${TOMAHAWK_ICONS} OUTFILE "${TOMAHAWK_APPLICATION_NAME}")
else()
    ecm_add_app_icon(tomahawkSources ICONS ${TOMAHAWK_ICONS})
endif()

qt_add_resources( RC_SRCS "../../resources.qrc" )

SET( final_src ${final_src} ${tomahawkUI_H} ${tomahawkMoc} ${tomahawkSources} ${RC_SRCS} )

if(APPLE OR WIN32)
    set(TOMAHAWK_RUNTIME_OUTPUT_NAME "${TOMAHAWK_APPLICATION_NAME}")
else()
    set(TOMAHAWK_RUNTIME_OUTPUT_NAME "${TOMAHAWK_TARGET_NAME}")
endif()

ADD_EXECUTABLE( ${TOMAHAWK_APPLICATION_TARGET} WIN32 MACOSX_BUNDLE ${final_src} )
SET_TARGET_PROPERTIES( ${TOMAHAWK_APPLICATION_TARGET}
    PROPERTIES
        AUTOMOC TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_BINARY_DIR}/Info.plist"
        RUNTIME_OUTPUT_NAME ${TOMAHAWK_RUNTIME_OUTPUT_NAME}
)


qt5_use_modules(${TOMAHAWK_APPLICATION_TARGET} Core Widgets Network Sql WebKitWidgets)
if(APPLE)
    qt5_use_modules(${TOMAHAWK_APPLICATION_TARGET} MacExtras)
endif()
if(WIN32)
    qt5_use_modules(${TOMAHAWK_APPLICATION_TARGET} WinExtras)
endif()

MESSAGE( STATUS "OS_SPECIFIC_LINK_LIBRARIES: ${OS_SPECIFIC_LINK_LIBRARIES}" )

IF( LIBLASTFM_FOUND )
    LIST(APPEND LINK_LIBRARIES ${LINK_LIBRARIES} ${LIBLASTFM_LIBRARIES} )
ENDIF( LIBLASTFM_FOUND )
IF( QCA2_FOUND )
    LIST(APPEND LINK_LIBRARIES ${LINK_LIBRARIES} ${QCA2_LIBRARIES} )
ENDIF( QCA2_FOUND )
IF( WITH_CRASHREPORTER )
    LIST(APPEND LINK_LIBRARIES ${LINK_LIBRARIES} crashreporter-handler )
ENDIF()

TARGET_LINK_LIBRARIES( ${TOMAHAWK_APPLICATION_TARGET}
    kdsingleapplicationguard
    ${LINK_LIBRARIES}
    ${TOMAHAWK_WIDGETS_LIBRARIES}
    ${TOMAHAWK_PLAYDARAPI_LIBRARIES}
    ${TOMAHAWK_LIBRARIES}
    ${OS_SPECIFIC_LINK_LIBRARIES}
    ${QT_LIBRARIES}
    ${MAC_EXTRA_LIBS}
    ${ECHONEST_LIBRARIES}
    ${Boost_LIBRARIES}
)

IF( APPLE )
  IF( HAVE_SPARKLE )
    MESSAGE("Sparkle Found, installing framekwork in bundle")
    INSTALL(DIRECTORY "${SPARKLE}/Versions/Current/Resources" DESTINATION "${CMAKE_BINARY_DIR}/${TOMAHAWK_APPLICATION_NAME}.app/Contents/Frameworks/Sparkle.framework")
  ENDIF( HAVE_SPARKLE )
ENDIF( APPLE )

INSTALL( TARGETS ${TOMAHAWK_APPLICATION_TARGET} BUNDLE DESTINATION . RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} )

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
