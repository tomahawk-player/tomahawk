project( tomahawklib )

add_definitions( ${QT_DEFINITIONS} )
add_definitions( -DQT_SHARED )
add_definitions( -DDLLEXPORT_PRO )
add_definitions( -DQT_SHAREDPOINTER_TRACK_POINTERS )
add_definitions( -DPORTFWDDLLEXPORT_STATIC )

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../tomahawk/Config.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/config.h)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../tomahawk/TomahawkVersion.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/TomahawkVersion.h)

set( libGuiSources
    ActionCollection.cpp

    ContextMenu.cpp
    DownloadManager.cpp
    DownloadJob.cpp
    DropJob.cpp
    GlobalActionManager.cpp
    ViewPage.cpp
    ViewPageLazyLoader.cpp
    ViewPagePlugin.cpp
    ViewManager.cpp
    LatchManager.cpp

    jobview/JobStatusView.cpp
    jobview/JobStatusModel.cpp
    jobview/JobStatusDelegate.cpp
    jobview/JobStatusItem.cpp
    jobview/AclJobItem.cpp
    jobview/PipelineStatusItem.cpp
    jobview/ScannerStatusItem.cpp
    jobview/TransferStatusItem.cpp
    jobview/LatchedStatusItem.cpp
    jobview/ErrorStatusMessage.cpp
    jobview/IndexingJobItem.cpp
    jobview/InboxJobItem.cpp

    playlist/InboxModel.cpp
    playlist/InboxView.cpp
    playlist/ContextView.cpp
    playlist/TreeModel.cpp
    playlist/TreeProxyModel.cpp
    playlist/TreeProxyModelPlaylistInterface.cpp
    playlist/ColumnItemDelegate.cpp
    playlist/ColumnViewPreviewWidget.cpp
    playlist/PlaylistModel.cpp
    playlist/PlaylistItemDelegate.cpp
    playlist/QueueProxyModel.cpp
    playlist/QueueView.cpp
    playlist/PlayableModel.cpp
    playlist/PlayableProxyModel.cpp
    playlist/PlayableProxyModelPlaylistInterface.cpp
    playlist/TrackView.cpp
    playlist/AlbumModel.cpp
    playlist/GridItemDelegate.cpp
    playlist/GridView.cpp
    playlist/ColumnView.cpp
    playlist/ViewHeader.cpp
    playlist/LovedTracksModel.cpp
    playlist/RecentlyLovedTracksModel.cpp
    playlist/TopLovedTracksModel.cpp
    playlist/RecentlyAddedModel.cpp
    playlist/RecentlyPlayedModel.cpp
    playlist/TrackItemDelegate.cpp
    playlist/PlayableItem.cpp
    playlist/SingleTrackPlaylistInterface.cpp
    playlist/RevisionQueueItem.cpp
    playlist/TrackDetailView.cpp

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

    resolvers/ExternalResolverGui.cpp
    resolvers/ScriptResolver.cpp
    resolvers/JSResolver.cpp
    resolvers/JSResolverHelper.cpp
    resolvers/ScriptEngine.cpp
    resolvers/JSAccount.cpp
    resolvers/ScriptJob.cpp
    resolvers/SyncScriptJob.cpp
    resolvers/ScriptObject.cpp
    resolvers/ScriptLinkGeneratorPlugin.cpp
    resolvers/ScriptAccount.cpp
    resolvers/ScriptPlugin.cpp

    utils/DpiScaler.cpp
    utils/ImageRegistry.cpp
    utils/WidgetDragFilter.cpp
    utils/XspfGenerator.cpp
    utils/JspfLoader.cpp
    utils/SpotifyParser.cpp
    utils/M3uLoader.cpp
    utils/ItunesParser.cpp
    utils/ItunesLoader.cpp
    utils/ShortenedLinkParser.cpp
    utils/TomahawkStyle.cpp
    utils/DropJobNotifier.cpp
    utils/ProxyStyle.cpp
    utils/TomahawkUtilsGui.cpp
    utils/Closure.cpp
    utils/PixmapDelegateFader.cpp
    utils/SmartPointerList.h
    utils/AnimatedSpinner.cpp
    utils/BinaryInstallerHelper.cpp
    utils/BinaryExtractWorker.cpp
    utils/SharedTimeLine.cpp
    utils/ResultUrlChecker.cpp
    utils/NetworkReply.cpp
    utils/NetworkProxyFactory.cpp
    utils/NetworkAccessManager.cpp
    utils/ShortLinkHelper.cpp
    utils/LinkGenerator.cpp
    utils/LinkGeneratorPlugin.cpp
    utils/TomaHkLinkGeneratorPlugin.cpp

    viewpages/SearchViewPage.cpp
    viewpages/SourceViewPage.cpp
    viewpages/ArtistViewPage.cpp
    viewpages/AlbumViewPage.cpp
    viewpages/TrackViewPage.cpp
    viewpages/PlaylistViewPage.cpp
    viewpages/CollectionViewPage.cpp

    widgets/AnimatedSplitter.cpp
    widgets/BasicHeader.cpp
    widgets/HoverControls.cpp
    widgets/BackgroundWidget.cpp
    widgets/Breadcrumb.cpp
    widgets/BreadcrumbButton.cpp
    widgets/ChartDataLoader.cpp
    widgets/ClickableLabel.cpp
    widgets/ComboBox.cpp
    widgets/DropDownButton.cpp
    widgets/ElidedLabel.cpp
    widgets/FilterHeader.cpp
    widgets/CaptionLabel.cpp
    widgets/HeaderLabel.cpp
    widgets/HistoryWidget.cpp
    widgets/ImageButton.cpp
    widgets/OverlayButton.cpp
    widgets/OverlayWidget.cpp
    widgets/PlayableCover.cpp
    widgets/PlaylistsModel.cpp
    widgets/QueryLabel.cpp
    widgets/RecentPlaylistsModel.cpp
    widgets/RecentlyPlayedPlaylistsModel.cpp
    widgets/ScriptCollectionHeader.cpp
    widgets/ScrollingLabel.cpp
    widgets/SeekSlider.cpp
    widgets/SourceTreePopupDialog.cpp
    widgets/ToggleButton.cpp

    widgets/searchlineedit/ClearButton.cpp
    widgets/searchlineedit/LineEdit.cpp
    widgets/searchlineedit/SearchButton.cpp
    widgets/searchlineedit/SearchLineEdit.cpp
)

if(QCA2_FOUND)
    set( libGuiSources ${libGuiSources} utils/GroovesharkParser.cpp )
endif()

if(UNIX AND NOT APPLE AND NOT Qt5Core_DIR)
    if(BUILD_GUI AND X11_FOUND)
        include_directories( ${THIRDPARTY_DIR}/libqnetwm )
        list(APPEND libSources ${libSources} ${THIRDPARTY_DIR}/libqnetwm/libqnetwm/netwm.cpp)
        list(APPEND LINK_LIBRARIES ${X11_LIBRARIES})
    endif()
endif()

list(APPEND libSources
    TomahawkSettings.cpp
    SourceList.cpp
    Pipeline.cpp

    Artist.cpp
    ArtistPlaylistInterface.cpp
    Album.cpp
    AlbumPlaylistInterface.cpp
    CountryUtils.cpp
    FuncTimeout.cpp
    Playlist.cpp
    PlaylistEntry.cpp
    PlaylistPlaylistInterface.cpp
    MetaPlaylistInterface.cpp
    Query.cpp
    Result.cpp
    Source.cpp
    Track.cpp
    TrackData.cpp
    SourcePlaylistInterface.cpp
    PlaylistInterface.cpp
    UrlHandler.cpp

    EchonestCatalogSynchronizer.cpp

    accounts/AccountManager.cpp
    accounts/Account.cpp
    accounts/AccountModel.cpp
    accounts/AccountModelFilterProxy.cpp
    accounts/CredentialsManager.cpp
    accounts/ResolverAccount.cpp
    accounts/AccountDelegate.cpp
    accounts/DelegateConfigWrapper.cpp
    accounts/AccountFactoryWrapper.cpp
    accounts/AccountFactoryWrapperDelegate.cpp
    accounts/AccountConfigWidget.cpp
    accounts/ConfigStorage.cpp
    accounts/LocalConfigStorage.cpp

    accounts/spotify/SpotifyAccount.cpp
    accounts/spotify/SpotifyAccountConfig.cpp
#    accounts/spotify/SpotifyPlaylistUpdater.cpp
    accounts/spotify/SpotifyInfoPlugin.cpp

    audio/AudioEngine.cpp
    audio/AudioOutput.cpp
    audio/MediaStream.cpp
    audio/Qnr_IoDeviceStream.cpp

    collection/Collection.cpp
    collection/ArtistsRequest.cpp
    collection/AlbumsRequest.cpp
    collection/TracksRequest.cpp

    database/Database.cpp
    database/fuzzyindex/FuzzyIndex.cpp
    database/fuzzyindex/DatabaseFuzzyIndex.cpp
    database/DatabaseCollection.cpp
    database/LocalCollection.cpp
    database/DatabaseWorker.cpp
    database/DatabaseImpl.cpp
    database/DatabaseResolver.cpp
    database/DatabaseCommand.cpp
    database/DatabaseCommand_AddClientAuth.cpp
    database/DatabaseCommand_AddFiles.cpp
    database/DatabaseCommand_AddSource.cpp
    database/DatabaseCommand_AllAlbums.cpp
    database/DatabaseCommand_AllArtists.cpp
    database/DatabaseCommand_AllTracks.cpp
    database/DatabaseCommand_ArtistStats.cpp
    database/DatabaseCommand_CalculatePlaytime.cpp
    database/DatabaseCommand_ClientAuthValid.cpp
    database/DatabaseCommand_CollectionAttributes.cpp
    database/DatabaseCommand_CollectionStats.cpp
    database/DatabaseCommand_CreateDynamicPlaylist.cpp
    database/DatabaseCommand_CreatePlaylist.cpp
    database/DatabaseCommand_DeleteDynamicPlaylist.cpp
    database/DatabaseCommand_DeleteFiles.cpp
    database/DatabaseCommand_DeleteInboxEntry.cpp
    database/DatabaseCommand_DeletePlaylist.cpp
    database/DatabaseCommand_DirMtimes.cpp
    database/DatabaseCommand_FileMTimes.cpp
    database/DatabaseCommand_GenericSelect.cpp
    database/DatabaseCommand_LoadAllAutoPlaylists.cpp
    database/DatabaseCommand_LoadAllPlaylists.cpp
    database/DatabaseCommand_LoadAllSortedPlaylists.cpp
    database/DatabaseCommand_LoadAllSources.cpp
    database/DatabaseCommand_LoadAllStations.cpp
    database/DatabaseCommand_LoadDynamicPlaylist.cpp
    database/DatabaseCommand_LoadDynamicPlaylistEntries.cpp
    database/DatabaseCommand_LoadFiles.cpp
    database/DatabaseCommand_LoadInboxEntries.cpp
    database/DatabaseCommand_LoadOps.cpp
    database/DatabaseCommand_LoadPlaylistEntries.cpp
    database/DatabaseCommand_LoadSocialActions.cpp
    database/DatabaseCommand_LoadTrackAttributes.cpp
    database/DatabaseCommand_LogPlayback.cpp
    database/DatabaseCommand_ModifyInboxEntry.cpp
    database/DatabaseCommand_ModifyPlaylist.cpp
    database/DatabaseCommand_NetworkCharts.cpp
    database/DatabaseCommand_PlaybackCharts.cpp
    database/DatabaseCommand_PlaybackHistory.cpp
    database/DatabaseCommand_RenamePlaylist.cpp
    database/DatabaseCommand_Resolve.cpp
    database/DatabaseCommand_SetCollectionAttributes.cpp
    database/DatabaseCommand_SetDynamicPlaylistRevision.cpp
    database/DatabaseCommand_SetPlaylistRevision.cpp
    database/DatabaseCommand_SetTrackAttributes.cpp
    database/DatabaseCommand_ShareTrack.cpp
    database/DatabaseCommand_SocialAction.cpp
    database/DatabaseCommand_SourceOffline.cpp
    database/DatabaseCommand_TrackAttributes.cpp
    database/DatabaseCommand_TrackStats.cpp
    database/DatabaseCommand_TrendingArtists.cpp
    database/DatabaseCommand_TrendingTracks.cpp
    database/DatabaseCommand_UpdateSearchIndex.cpp
    database/DatabaseCommandLoggable.cpp
    database/IdThreadWorker.cpp
    database/TomahawkSqlQuery.cpp

    infosystem/InfoSystem.cpp
    infosystem/InfoSystemCache.cpp
    infosystem/InfoSystemWorker.cpp

    filemetadata/MusicScanner.cpp
    filemetadata/ScanManager.cpp
    filemetadata/taghandlers/tag.cpp
    filemetadata/taghandlers/apetag.cpp
    filemetadata/taghandlers/asftag.cpp
    filemetadata/taghandlers/id3v1tag.cpp
    filemetadata/taghandlers/id3v2tag.cpp
    filemetadata/taghandlers/mp4tag.cpp
    filemetadata/taghandlers/oggtag.cpp
    filemetadata/MetadataEditor.cpp

    network/acl/AclRegistry.cpp
    network/acl/AclRequest.cpp
    network/BufferIoDevice.cpp
    network/Msg.cpp
    network/MsgProcessor.cpp
    network/StreamConnection.cpp
    network/DbSyncConnection.cpp
    network/RemoteCollection.cpp
    network/PortFwdThread.cpp
    network/Servent.cpp
    network/Connection.cpp
    network/ControlConnection.cpp
    network/QTcpSocketExtra.cpp
    network/ConnectionManager.cpp

    playlist/PlaylistUpdaterInterface.cpp
    playlist/PlaylistTemplate.cpp
    playlist/XspfPlaylistTemplate.cpp
    playlist/dynamic/DynamicPlaylist.cpp
    playlist/dynamic/GeneratorFactory.cpp
    playlist/dynamic/GeneratorInterface.cpp
    playlist/dynamic/DynamicPlaylistRevision.cpp
    playlist/XspfUpdater.cpp
    playlist/dynamic/database/DatabaseGenerator.cpp
    playlist/dynamic/database/DatabaseControl.cpp
    playlist/dynamic/DynamicControl.cpp

    resolvers/ExternalResolver.cpp
    resolvers/Resolver.cpp
    resolvers/ScriptCommand_AllArtists.cpp
    resolvers/ScriptCommand_AllAlbums.cpp
    resolvers/ScriptCommand_AllTracks.cpp
    resolvers/ScriptCommand_LookupUrl.cpp
    resolvers/ScriptCommandQueue.cpp
    resolvers/ScriptPluginFactory.cpp

    # ScriptPlugins
    resolvers/ScriptCollection.cpp
    resolvers/plugins/ScriptCollectionFactory.cpp
    resolvers/ScriptInfoPlugin.cpp
    resolvers/plugins/ScriptInfoPluginFactory.cpp


    sip/SipPlugin.cpp
    sip/SipInfo.cpp
    sip/PeerInfo.cpp
    sip/SipStatusMessage.cpp

    utils/Cloudstream.cpp
    utils/Json.cpp
    utils/TomahawkUtils.cpp
    utils/Logger.cpp
    utils/XspfLoader.cpp
    utils/TomahawkCache.cpp
    utils/GuiHelpers.cpp
    utils/WeakObjectHash.cpp
    utils/WeakObjectList.cpp
    utils/WebPopup.cpp
    utils/PluginLoader.cpp
)

add_subdirectory( accounts/configstorage )

IF(LIBLASTFM_FOUND)
    include_directories( ${LIBLASTFM_INCLUDE_DIRS} )
    list(APPEND LINK_LIBRARIES ${LIBLASTFM_LIBRARIES} )
    list(APPEND libSources
        accounts/lastfm/LastFmAccount.cpp
        accounts/lastfm/LastFmConfig.cpp
        accounts/lastfm/LastFmInfoPlugin.cpp
    )
ENDIF(LIBLASTFM_FOUND)

set( libUI ${libUI}
    widgets/HeaderWidget.ui
    viewpages/SearchViewPage.ui
    viewpages/SourceViewPage.ui
    viewpages/ArtistViewPage.ui
    viewpages/AlbumViewPage.ui
    viewpages/TrackViewPage.ui
    playlist/QueueView.ui
    filemetadata/MetadataEditor.ui
    accounts/AccountFactoryWrapper.ui
    accounts/spotify/SpotifyAccountConfig.ui
    accounts/lastfm/LastFmConfig.ui
)

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}

    ${QT_INCLUDE_DIR}
    ${QJSON_INCLUDE_DIR}
    ${ECHONEST_INCLUDE_DIR}
    ${LUCENEPP_INCLUDE_DIRS}
    ${LIBVLC_INCLUDE_DIR}
    ${Boost_INCLUDE_DIR}

    ${LIBPORTFWD_INCLUDE_DIR}
    ${QUAZIP_INCLUDE_DIR}
    ${QTKEYCHAIN_INCLUDE_DIRS}
)

IF(LIBATTICA_FOUND)
    SET( libGuiSources ${libGuiSources} AtticaManager.cpp )
    INCLUDE_DIRECTORIES( ${LIBATTICA_INCLUDE_DIR} )
    LIST(APPEND PRIVATE_LINK_LIBRARIES ${LIBATTICA_LIBRARIES} ${LibAttica_LIBRARIES} ${QUAZIP_LIBRARIES})
    IF( TOMAHAWK_QT5 )
        LIST(APPEND LINK_LIBRARIES KF5::Attica )
    ENDIF( TOMAHAWK_QT5 )
ENDIF(LIBATTICA_FOUND)

if(HAVE_X11)
    list(APPEND LINK_LIBRARIES Qt5::X11Extras)
endif()

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

    SET( CMAKE_SHARED_LINKER_FLAGS "-headerpad_max_install_names ${CMAKE_SHARED_LINKER_FLAGS}" )

    SET( libSources ${libSources}
      utils/TomahawkUtils_Mac.mm
      mac/FileHelpers.mm
      widgets/SourceTreePopupDialog_mac.mm )

    list(APPEND libSources
      thirdparty/Qocoa/qbutton_mac.mm
      thirdparty/Qocoa/qsearchfield_mac.mm
      thirdparty/Qocoa/qtoolbartabdialog.cpp
    )

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
    SET( libGuiSources ${libGuiSources} thirdparty/Qocoa/qbutton.cpp thirdparty/Qocoa/qsearchfield.cpp thirdparty/Qocoa/qtoolbartabdialog.cpp )
ENDIF( APPLE )

IF(BUILD_GUI)
    LIST(APPEND libSources ${libGuiSources} )
ENDIF()

qt_wrap_ui(libUI_H ${libUI})

SET( libSources ${libSources} ${libUI_H} )

add_library( ${TOMAHAWK_LIBRARY} SHARED ${libSources})
set_target_properties(
    ${TOMAHAWK_LIBRARY}
        PROPERTIES
            AUTOMOC TRUE
            VERSION ${TOMAHAWK_VERSION_SHORT}
            SOVERSION ${TOMAHAWK_VERSION_SHORT}
            OUTPUT_NAME ${TOMAHAWK_BASE_TARGET_NAME}
)


qt5_use_modules(${TOMAHAWK_LIBRARY} Widgets Network Sql WebKitWidgets Concurrent Xml UiTools Svg)
IF(APPLE)
       qt5_use_modules(${TOMAHAWK_LIBRARY} MacExtras)
ENDIF()


IF(QCA2_FOUND)
    INCLUDE_DIRECTORIES( ${QCA2_INCLUDE_DIR} )
    LIST(APPEND LINK_LIBRARIES ${QCA2_LIBRARIES} )
ENDIF(QCA2_FOUND)

IF( UNIX AND NOT APPLE )
    LIST(APPEND LINK_LIBRARIES ${QT_QTDBUS_LIBRARIES} )
    qt5_use_modules(${TOMAHAWK_LIBRARY} DBus)
ENDIF( UNIX AND NOT APPLE )

TARGET_LINK_LIBRARIES( ${TOMAHAWK_LIBRARY}
    LINK_PRIVATE
    ${LIBVLC_LIBRARY}

    # Thirdparty shipped with tomahawk
    ${LIBPORTFWD_LIBRARIES}
    ${QTKEYCHAIN_LIBRARIES}
    ${PRIVATE_LINK_LIBRARIES}

    LINK_PUBLIC
    # External deps
    ${QJSON_LIBRARIES}
    ${LUCENEPP_LIBRARIES}
    ${ECHONEST_LIBRARIES}
    ${QT_QTSQL_LIBRARY}
    ${QT_QTUITOOLS_LIBRARY}
    ${QT_QTGUI_LIBRARY}
    ${QT_QTWEBKIT_LIBRARY}
    ${QT_QTSCRIPT_LIBRARY}
    ${QT_QTNETWORK_LIBRARY}
    ${QT_QTXML_LIBRARY}
    ${QT_QTSVG_LIBRARY}
    ${QT_QTCORE_LIBRARY}
    ${OS_SPECIFIC_LINK_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${LINK_LIBRARIES}
)

# TagLib
TARGET_LINK_LIBRARIES(${TOMAHAWK_LIBRARY} LINK_PUBLIC ${TAGLIB_LIBRARIES})
TARGET_INCLUDE_DIRECTORIES(${TOMAHAWK_LIBRARY} PUBLIC ${TAGLIB_INCLUDES})

INSTALL( TARGETS ${TOMAHAWK_LIBRARY}
    EXPORT TomahawkLibraryDepends
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# Install header files
file( GLOB rootHeaders "*.h" )
file( GLOB accountsHeaders "accounts/*.h" )
file( GLOB accountsLastfmHeaders "accounts/lastfm/*.h" )
file( GLOB accountsSpotifyHeaders "accounts/spotify/*.h" )
file( GLOB audioHeaders "audio/*.h" )
file( GLOB collectionHeaders "collection/*.h" )
file( GLOB contextHeaders "context/*.h" )
file( GLOB contextPagesHeaders "context/pages/*.h" )
file( GLOB databaseHeaders "database/*.h" )
file( GLOB filemetadataHeaders "filemetadata/*.h" )
file( GLOB filemetadataTaghandlersHeaders "filemetadata/taghandlers/*.h" )
file( GLOB infosystemHeaders "infosystem/*.h" )
file( GLOB jobviewHeaders "jobview/*.h" )
file( GLOB networkHeaders "network/*.h" )
file( GLOB playlistHeaders "playlist/*.h" )
file( GLOB playlistDynamicHeaders "playlist/dynamic/*.h" )
file( GLOB playlistDynamicDatabaseHeaders "playlist/dynamic/database/*.h" )
file( GLOB playlistDynamicEchonestHeaders "playlist/dynamic/echonest/*.h" )
file( GLOB playlistDynamicWidgetsHeaders "playlist/dynamic/widgets/*.h" )
file( GLOB resolversHeaders "resolvers/*.h" )
file( GLOB sipHeaders "sip/*.h" )
file( GLOB thirdpartyKdsingleapplicationguardHeaders "thirdparty/kdsingleapplicationguard/*.h" )
file( GLOB thirdpartyQocoaHeaders "thirdparty/Qocoa/*.h" )
file( GLOB utilsHeaders "utils/*.h" )
file( GLOB widgetsHeaders "widgets/*.h" )
file( GLOB viewpagesHeaders "viewpages/*.h" )
file( GLOB widgetsSearchlineeditHeaders "widgets/searchlineedit/*.h" )

install( FILES ${CMAKE_CURRENT_BINARY_DIR}/config.h DESTINATION include/libtomahawk )
install( FILES ${rootHeaders} DESTINATION include/libtomahawk )
install( FILES ${accountsHeaders} DESTINATION include/libtomahawk/accounts )
install( FILES ${accountsLastfmHeaders} DESTINATION include/libtomahawk/accounts/lastfm )
install( FILES ${accountsSpotifyHeaders} DESTINATION include/libtomahawk/accounts/spotify )
install( FILES ${audioHeaders} DESTINATION include/libtomahawk/audio )
install( FILES ${collectionHeaders} DESTINATION include/libtomahawk/collection )
install( FILES ${contextHeaders} DESTINATION include/libtomahawk/context )
install( FILES ${contextPagesHeaders} DESTINATION include/libtomahawk/context/pages )
install( FILES ${databaseHeaders} DESTINATION include/libtomahawk/database )
install( FILES ${filemetadataHeaders} DESTINATION include/libtomahawk/filemetadata )
install( FILES ${filemetadataTaghandlersHeaders} DESTINATION include/libtomahawk/filemetadata/taghandlers )
install( FILES ${infosystemHeaders} DESTINATION include/libtomahawk/infosystem )
install( FILES ${jobviewHeaders} DESTINATION include/libtomahawk/jobview )
install( FILES ${networkHeaders} DESTINATION include/libtomahawk/network )
install( FILES ${playlistHeaders} DESTINATION include/libtomahawk/playlist )
install( FILES ${playlistDynamicHeaders} DESTINATION include/libtomahawk/playlist/dynamic )
install( FILES ${playlistDynamicDatabaseHeaders} DESTINATION include/libtomahawk/playlist/dynamic/database )
install( FILES ${playlistDynamicEchonestHeaders} DESTINATION include/libtomahawk/playlist/dynamic/echonest )
install( FILES ${playlistDynamicWidgetsHeaders} DESTINATION include/libtomahawk/playlist/dynamic/widgets )
install( FILES ${resolversHeaders} DESTINATION include/libtomahawk/resolvers )
install( FILES ${sipHeaders} DESTINATION include/libtomahawk/sip )
install( FILES ${thirdpartyKdsingleapplicationguardHeaders} DESTINATION include/libtomahawk/thirdparty/kdsingleapplicationguard )
install( FILES ${thirdpartyQocoaHeaders} DESTINATION include/libtomahawk/thirdparty/Qocoa )
install( FILES ${utilsHeaders} DESTINATION include/libtomahawk/utils )
install( FILES ${widgetsHeaders} DESTINATION include/libtomahawk/widgets )
install( FILES ${viewpagesHeaders} DESTINATION include/libtomahawk/viewpages )
install( FILES ${widgetsSearchlineeditHeaders} DESTINATION include/libtomahawk/widgets/searchlineedit )
