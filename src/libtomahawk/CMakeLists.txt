project( tomahawklib )

SET( QT_USE_QTGUI TRUE )
SET( QT_USE_QTSQL TRUE )
SET( QT_USE_QTNETWORK TRUE )
SET( QT_USE_QTXML TRUE )
SET( QT_USE_QTUITOOLS TRUE )

include( ${QT_USE_FILE} )

add_definitions( ${QT_DEFINITIONS} )
add_definitions( -DQT_SHARED )
add_definitions( -DDLLEXPORT_PRO )
add_definitions( -DQT_SHAREDPOINTER_TRACK_POINTERS )


set( libGuiSources
    ActionCollection.cpp

    ContextMenu.cpp
    DropJob.cpp
    GlobalActionManager.cpp
    ViewPage.cpp
    ViewManager.cpp
    LatchManager.cpp
    TomahawkSettingsGui.cpp

    context/ContextPage.cpp
    context/ContextWidget.cpp
    context/pages/TopTracksContext.cpp
    context/pages/RelatedArtistsContext.cpp
    context/pages/WikipediaContext.cpp
    context/pages/WebContext.cpp

    jobview/JobStatusView.cpp
    jobview/JobStatusModel.cpp
    jobview/JobStatusDelegate.cpp
    jobview/JobStatusItem.cpp
    jobview/AclJobItem.cpp
    jobview/PipelineStatusItem.cpp
    jobview/TransferStatusItem.cpp
    jobview/LatchedStatusItem.cpp
    jobview/ErrorStatusMessage.cpp
    jobview/IndexingJobItem.cpp

    infobar/InfoBar.cpp

    playlist/TreeModel.cpp
    playlist/TreeProxyModel.cpp
    playlist/TreeProxyModelPlaylistInterface.cpp
    playlist/TreeHeader.cpp
    playlist/TreeItemDelegate.cpp
    playlist/CollectionProxyModel.cpp
    playlist/CollectionProxyModelPlaylistInterface.cpp
    playlist/CollectionFlatModel.cpp
    playlist/CollectionView.cpp
    playlist/PlaylistModel.cpp
    playlist/PlaylistProxyModel.cpp
    playlist/PlaylistProxyModelPlaylistInterface.cpp
    playlist/PlaylistView.cpp
    playlist/PlaylistItemDelegate.cpp
    playlist/QueueProxyModel.cpp
    playlist/QueueProxyModelPlaylistInterface.cpp
    playlist/QueueView.cpp
    playlist/TrackModel.cpp
    playlist/TrackModelItem.cpp
    playlist/TrackProxyModel.cpp
    playlist/TrackProxyModelPlaylistInterface.cpp
    playlist/TrackView.cpp
    playlist/TrackHeader.cpp
    playlist/TreeModelItem.cpp
    playlist/AlbumItem.cpp
    playlist/AlbumModel.cpp
    playlist/AlbumProxyModel.cpp
    playlist/AlbumProxyModelPlaylistInterface.cpp
    playlist/AlbumItemDelegate.cpp
    playlist/AlbumView.cpp
    playlist/ArtistView.cpp
    playlist/CustomPlaylistView.cpp
    playlist/ViewHeader.cpp
    playlist/RecentlyAddedModel.cpp
    playlist/RecentlyPlayedModel.cpp
    playlist/PlaylistLargeItemDelegate.cpp
    playlist/PlaylistChartItemDelegate.cpp

    playlist/dynamic/DynamicPlaylist.cpp
    playlist/dynamic/DynamicView.cpp
    playlist/dynamic/DynamicModel.cpp
    playlist/dynamic/echonest/EchonestGenerator.cpp
    playlist/dynamic/echonest/EchonestControl.cpp
    playlist/dynamic/echonest/EchonestSteerer.cpp
    playlist/dynamic/widgets/DynamicWidget.cpp
    playlist/dynamic/widgets/DynamicControlWrapper.cpp
    playlist/dynamic/widgets/DynamicControlList.cpp
    playlist/dynamic/widgets/ReadOrWriteWidget.cpp
    playlist/dynamic/widgets/MiscControlWidgets.cpp
    playlist/dynamic/widgets/CollapsibleControls.cpp
    playlist/dynamic/widgets/DynamicSetupWidget.cpp

    playlist/topbar/TopBar.cpp
    playlist/topbar/ClearButton.cpp
    playlist/topbar/SearchLineEdit.cpp
    playlist/topbar/LineEdit.cpp
    playlist/topbar/SearchButton.cpp

    ExternalResolverGui.cpp
    resolvers/ScriptResolver.cpp
    resolvers/QtScriptResolver.cpp

    utils/WidgetDragFilter.cpp
    utils/XspfGenerator.cpp
    utils/JspfLoader.cpp
    utils/SpotifyParser.cpp
    utils/M3uLoader.cpp
    utils/ItunesParser.cpp
    utils/RdioParser.cpp
    utils/ShortenedLinkParser.cpp
    utils/StyleHelper.cpp
    utils/DropJobNotifier.cpp
    utils/ProxyStyle.cpp
    utils/TomahawkUtilsGui.cpp
    utils/Closure.cpp
    utils/PixmapDelegateFader.cpp
    utils/SmartPointerList.h
    utils/AnimatedSpinner.cpp

    widgets/AnimatedCounterLabel.cpp
    widgets/CheckDirTree.cpp
    widgets/QueryLabel.cpp
    widgets/ImageButton.cpp
    widgets/AnimatedSplitter.cpp
    widgets/ElidedLabel.cpp
    widgets/NewPlaylistWidget.cpp
    widgets/SearchWidget.cpp
    widgets/SeekSlider.cpp
    widgets/PlaylistTypeSelectorDialog.cpp
    widgets/WelcomeWidget.cpp
    widgets/WhatsHotWidget.cpp
    widgets/NewReleasesWidget.cpp
    widgets/ChartDataLoader.cpp
    widgets/RecentlyPlayedPlaylistsModel.cpp
    widgets/RecentPlaylistsModel.cpp
    widgets/OverlayButton.cpp
    widgets/OverlayWidget.cpp
    widgets/HeaderLabel.cpp
    widgets/HeaderWidget.cpp
    widgets/ComboBox.cpp
    widgets/ToggleButton.cpp
    widgets/FadingPixmap.cpp
    widgets/SocialPlaylistWidget.cpp
    widgets/infowidgets/SourceInfoWidget.cpp
    widgets/infowidgets/ArtistInfoWidget.cpp
    widgets/infowidgets/AlbumInfoWidget.cpp
    widgets/infowidgets/TrackInfoWidget.cpp
    widgets/Breadcrumb.cpp
    widgets/BreadcrumbButton.cpp
)

IF(QCA2_FOUND)
    set( libGuiSources ${libGuiSources} utils/GroovesharkParser.cpp )
    set( libGuiHeaders ${libGuiHeaders} utils/GroovesharkParser.h )
ENDIF(QCA2_FOUND)

set( libSources
    TomahawkSettings.cpp
    SourceList.cpp
    Pipeline.cpp

    AclRegistry.cpp
    Artist.cpp
    ArtistPlaylistInterface.cpp
    Album.cpp
    AlbumPlaylistInterface.cpp
    Collection.cpp
    FuncTimeout.cpp
    Playlist.cpp
    PlaylistPlaylistInterface.cpp
    Resolver.cpp
    ExternalResolver.cpp
    Query.cpp
    Result.cpp
    Source.cpp
    SourcePlaylistInterface.cpp
    PlaylistInterface.cpp

    EchonestCatalogSynchronizer.cpp

    accounts/AccountManager.cpp
    accounts/Account.cpp
    accounts/AccountModel.cpp
    accounts/AccountModelFilterProxy.cpp
    accounts/ResolverAccount.cpp
    sip/SipPlugin.cpp
    sip/SipHandler.cpp
    sip/SipInfo.cpp

    audio/AudioEngine.cpp


    database/Database.cpp
    database/FuzzyIndex.cpp
    database/DatabaseCollection.cpp
    database/LocalCollection.cpp
    database/DatabaseWorker.cpp
    database/DatabaseImpl.cpp
    database/DatabaseResolver.cpp
    database/DatabaseCommand.cpp
    database/DatabaseCommandLoggable.cpp
    database/DatabaseCommand_Resolve.cpp
    database/DatabaseCommand_AllArtists.cpp
    database/DatabaseCommand_AllAlbums.cpp
    database/DatabaseCommand_AllTracks.cpp
    database/DatabaseCommand_AddFiles.cpp
    database/DatabaseCommand_DeleteFiles.cpp
    database/DatabaseCommand_DirMtimes.cpp
    database/DatabaseCommand_FileMTimes.cpp
    database/DatabaseCommand_LoadFiles.cpp
    database/DatabaseCommand_LogPlayback.cpp
    database/DatabaseCommand_AddSource.cpp
    database/DatabaseCommand_SourceOffline.cpp
    database/DatabaseCommand_CollectionStats.cpp
    database/DatabaseCommand_TrackStats.cpp
    database/DatabaseCommand_LoadPlaylistEntries.cpp
    database/DatabaseCommand_ModifyPlaylist.cpp
    database/DatabaseCommand_PlaybackHistory.cpp
    database/DatabaseCommand_SetPlaylistRevision.cpp
    database/DatabaseCommand_LoadAllPlaylists.cpp
    database/DatabaseCommand_LoadAllSortedPlaylists.cpp
    database/DatabaseCommand_LoadAllSources.cpp
    database/DatabaseCommand_CreatePlaylist.cpp
    database/DatabaseCommand_DeletePlaylist.cpp
    database/DatabaseCommand_RenamePlaylist.cpp
    database/DatabaseCommand_LoadOps.cpp
    database/DatabaseCommand_UpdateSearchIndex.cpp
    database/DatabaseCommand_SetDynamicPlaylistRevision.cpp
    database/DatabaseCommand_CreateDynamicPlaylist.cpp
    database/DatabaseCommand_LoadDynamicPlaylist.cpp
    database/DatabaseCommand_LoadDynamicPlaylistEntries.cpp
    database/DatabaseCommand_LoadAllAutoPlaylists.cpp
    database/DatabaseCommand_LoadAllStations.cpp
    database/DatabaseCommand_DeleteDynamicPlaylist.cpp
    database/DatabaseCommand_AddClientAuth.cpp
    database/DatabaseCommand_ClientAuthValid.cpp
    database/DatabaseCommand_SocialAction.cpp
    database/DatabaseCommand_LoadSocialActions.cpp
    database/DatabaseCommand_GenericSelect.cpp
    database/DatabaseCommand_SetCollectionAttributes.cpp
    database/DatabaseCommand_CollectionAttributes.cpp
    database/DatabaseCommand_TrackAttributes.cpp
    database/DatabaseCommand_SetTrackAttributes.cpp
    database/Database.cpp

    infosystem/InfoSystem.cpp
    infosystem/InfoSystemCache.cpp
    infosystem/InfoSystemWorker.cpp

    network/BufferIoDevice.cpp
    network/MsgProcessor.cpp
    network/StreamConnection.cpp
    network/DbSyncConnection.cpp
    network/RemoteCollection.cpp
    network/PortFwdThread.cpp
    network/Servent.cpp
    network/Connection.cpp
    network/ControlConnection.cpp

    playlist/PlaylistUpdaterInterface.cpp
    playlist/dynamic/DynamicPlaylist.cpp
    playlist/dynamic/GeneratorFactory.cpp
    playlist/dynamic/GeneratorInterface.cpp
    playlist/dynamic/DynamicPlaylistRevision.cpp
    playlist/XspfUpdater.cpp
    playlist/dynamic/database/DatabaseGenerator.cpp
    playlist/dynamic/database/DatabaseControl.cpp
    playlist/dynamic/DynamicControl.cpp

    taghandlers/tag.cpp
    taghandlers/apetag.cpp
    taghandlers/asftag.cpp
    taghandlers/id3v1tag.cpp
    taghandlers/id3v2tag.cpp
    taghandlers/mp4tag.cpp
    taghandlers/oggtag.cpp

    utils/TomahawkUtils.cpp
    utils/Logger.cpp
    utils/Qnr_IoDeviceStream.cpp
    utils/XspfLoader.cpp
    utils/TomahawkCache.cpp

    thirdparty/kdsingleapplicationguard/kdsingleapplicationguard.cpp
    thirdparty/kdsingleapplicationguard/kdsharedmemorylocker.cpp
    thirdparty/kdsingleapplicationguard/kdtoolsglobal.cpp
    thirdparty/kdsingleapplicationguard/kdlockedsharedmemorypointer.cpp
)

set( libUI ${libUI}
    widgets/PlaylistTypeSelectorDialog.ui
    widgets/NewPlaylistWidget.ui
    widgets/SearchWidget.ui
    widgets/WelcomeWidget.ui
    widgets/WhatsHotWidget.ui
    widgets/NewReleasesWidget.ui
    widgets/SocialPlaylistWidget.ui
    widgets/infowidgets/SourceInfoWidget.ui
    widgets/infowidgets/ArtistInfoWidget.ui
    widgets/infowidgets/AlbumInfoWidget.ui
    widgets/infowidgets/TrackInfoWidget.ui
    playlist/topbar/TopBar.ui
    playlist/QueueView.ui
    context/ContextWidget.ui
    infobar/InfoBar.ui
)

include_directories( . ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR}/.. ..
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${QT_INCLUDE_DIR}
    ${QJSON_INCLUDE_DIR}
    ${LIBECHONEST_INCLUDE_DIR}
    ${LIBECHONEST_INCLUDE_DIR}/..
    ${CLUCENE_INCLUDE_DIRS}
    ${PHONON_INCLUDES}
    ${CMAKE_BINARY_DIR}/thirdparty/liblastfm2/src

    playlist

    ${LIBPORTFWD_INCLUDE_DIR}
    ${THIRDPARTY_DIR}/qxt/qxtweb-standalone/qxtweb
    ${QuaZip_INCLUDE_DIR}
)

IF(QCA2_FOUND)
    INCLUDE_DIRECTORIES( ${QCA2_INCLUDE_DIR} )
ENDIF(QCA2_FOUND)

IF(LIBATTICA_FOUND)
    SET( libGuiSources ${libGuiSources} AtticaManager.cpp )
    SET( libGuiHeaders ${libGuiHeaders} AtticaManager.h )
    INCLUDE_DIRECTORIES( ${LIBATTICA_INCLUDE_DIR} )
ENDIF(LIBATTICA_FOUND)

IF( UNIX AND NOT APPLE )
    IF( BUILD_GUI AND X11_FOUND )
        INCLUDE_DIRECTORIES( ${THIRDPARTY_DIR}/libqnetwm )
        SET( libSources ${libSources} ${THIRDPARTY_DIR}/libqnetwm/libqnetwm/netwm.cpp )
        SET( LINK_LIBRARIES ${LINK_LIBRARIES} ${X11_LIBRARIES} )
    ENDIF()
ENDIF( UNIX AND NOT APPLE )

IF( WIN32 )
    SET( OS_SPECIFIC_LINK_LIBRARIES
        ${OS_SPECIFIC_LINK_LIBRARIES}
    # System
        "iphlpapi.a"
        "ws2_32.dll"
        "dnsapi.dll"
        "dsound.dll"
        "winmm.dll"
        "advapi32.dll"
	"shlwapi.dll"
    )
ENDIF( WIN32 )

IF( APPLE )
    FIND_LIBRARY( COREAUDIO_LIBRARY CoreAudio )
    FIND_LIBRARY( COREFOUNDATION_LIBRARY CoreFoundation )
    FIND_LIBRARY( FOUNDATION_LIBRARY Foundation )
    FIND_LIBRARY( SCRIPTINGBRIDGE_LIBRARY ScriptingBridge )
    MARK_AS_ADVANCED( COREAUDIO_LIBRARY COREFOUNDATION_LIBRARY FOUNDATION_LIBRARY SCRIPTINGBRIDGE_LIBRARY )

    SET( libSources ${libSources}
      utils/TomahawkUtils_Mac.mm
      mac/FileHelpers.mm
      thirdparty/Qocoa/qsearchfield_mac.mm )

    SET_SOURCE_FILES_PROPERTIES(utils/TomahawkUtils_Mac.mm PROPERTIES COMPILE_FLAGS "-fvisibility=default")

    SET( OS_SPECIFIC_LINK_LIBRARIES
        ${OS_SPECIFIC_LINK_LIBRARIES}
        # System
        ${COREAUDIO_LIBRARY}
        ${COREFOUNDATION_LIBRARY}
        ${FOUNDATION_LIBRARY}
	${SCRIPTINGBRIDGE_LIBRARY}

        /System/Library/Frameworks/AppKit.framework
        /System/Library/Frameworks/Security.framework
    )
ELSE( APPLE )
    SET( libGuiSources ${libGuiSources} thirdparty/Qocoa/qsearchfield.cpp )
ENDIF( APPLE )

IF(LIBLASTFM_FOUND)
    SET(LINK_LIBRARIES ${LINK_LIBRARIES} tomahawk_lastfm2 )
ENDIF(LIBLASTFM_FOUND)

IF(BUILD_GUI)
    LIST(APPEND libSources ${libGuiSources} )
ENDIF()

qt4_wrap_ui( libUI_H ${libUI} )

SET( libSources ${libSources} ${libUI_H} )

ADD_LIBRARY( tomahawklib SHARED ${libSources})
set_target_properties(tomahawklib PROPERTIES AUTOMOC TRUE)

IF(QCA2_FOUND)
    SET(LINK_LIBRARIES ${LINK_LIBRARIES} ${QCA2_LIBRARIES} )
ENDIF(QCA2_FOUND)

IF(LIBATTICA_FOUND)
    SET( LINK_LIBRARIES ${LINK_LIBRARIES} ${LIBATTICA_LIBRARIES} ${QuaZip_LIBRARIES} )
ENDIF(LIBATTICA_FOUND)

TARGET_LINK_LIBRARIES( tomahawklib
    # Thirdparty shipped with tomahawk
    ${LIBPORTFWD_LIBRARIES}

    # External deps
    ${QJSON_LIBRARIES}
    ${PHONON_LIBS}
    ${TAGLIB_LIBRARIES}
    ${CLUCENE_LIBRARIES}
    ${LIBECHONEST_LIBRARY}
    ${QT_QTSQL_LIBRARY}
    ${QT_QTUITOOLS_LIBRARY}
    ${QT_QTGUI_LIBRARY}
    ${QT_QTSCRIPT_LIBRARY}
    ${OS_SPECIFIC_LINK_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${LINK_LIBRARIES}
)

INSTALL( TARGETS tomahawklib
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
