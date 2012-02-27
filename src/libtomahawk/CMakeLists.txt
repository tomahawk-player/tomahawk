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
    actioncollection.cpp

    contextmenu.cpp
    dropjob.cpp
    globalactionmanager.cpp
    viewpage.cpp
    viewmanager.cpp
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
    jobview/PipelineStatusItem.cpp
    jobview/TransferStatusItem.cpp
    jobview/LatchedStatusItem.cpp
    jobview/ErrorStatusMessage.cpp

    infobar/infobar.cpp

    playlist/treemodel.cpp
    playlist/treeproxymodel.cpp
    playlist/treeproxymodelplaylistinterface.cpp
    playlist/treeheader.cpp
    playlist/treeitemdelegate.cpp
    playlist/collectionproxymodel.cpp
    playlist/collectionproxymodelplaylistinterface.cpp
    playlist/collectionflatmodel.cpp
    playlist/collectionview.cpp
    playlist/playlistmodel.cpp
    playlist/playlistproxymodel.cpp
    playlist/playlistproxymodelplaylistinterface.cpp
    playlist/playlistview.cpp
    playlist/playlistitemdelegate.cpp
    playlist/queueproxymodel.cpp
    playlist/queueproxymodelplaylistinterface.cpp
    playlist/queueview.cpp
    playlist/trackmodel.cpp
    playlist/trackmodelitem.cpp
    playlist/trackproxymodel.cpp
    playlist/trackproxymodelplaylistinterface.cpp
    playlist/trackview.cpp
    playlist/trackheader.cpp
    playlist/treemodelitem.cpp
    playlist/albumitem.cpp
    playlist/albummodel.cpp
    playlist/albumproxymodel.cpp
    playlist/albumproxymodelplaylistinterface.cpp
    playlist/albumitemdelegate.cpp
    playlist/albumview.cpp
    playlist/artistview.cpp
    playlist/customplaylistview.cpp
    playlist/ViewHeader.cpp

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
    playlist/dynamic/widgets/LoadingSpinner.cpp

    playlist/topbar/topbar.cpp
    playlist/topbar/clearbutton.cpp
    playlist/topbar/searchlineedit.cpp
    playlist/topbar/lineedit.cpp
    playlist/topbar/searchbutton.cpp

    ExternalResolverGui.cpp
    resolvers/scriptresolver.cpp
    resolvers/qtscriptresolver.cpp

    utils/widgetdragfilter.cpp
    utils/xspfgenerator.cpp
    utils/jspfloader.cpp
    utils/spotifyparser.cpp
    utils/m3uloader.cpp
    utils/itunesparser.cpp
    utils/rdioparser.cpp
    utils/shortenedlinkparser.cpp
    utils/stylehelper.cpp
    utils/dropjobnotifier.cpp
    utils/proxystyle.cpp
    utils/tomahawkutilsgui.cpp

    widgets/checkdirtree.cpp
    widgets/querylabel.cpp
    widgets/imagebutton.cpp
    widgets/animatedsplitter.cpp
    widgets/elidedlabel.cpp
    widgets/newplaylistwidget.cpp
    widgets/searchwidget.cpp
    widgets/SeekSlider.cpp
    widgets/playlisttypeselectordlg.cpp
    widgets/welcomewidget.cpp
    widgets/whatshotwidget.cpp
    widgets/ChartDataLoader.cpp
    widgets/RecentlyPlayedPlaylistsModel.cpp
    widgets/RecentPlaylistsModel.cpp
    widgets/OverlayButton.cpp
    widgets/overlaywidget.cpp
    widgets/HeaderLabel.cpp
    widgets/HeaderWidget.cpp
    widgets/combobox.cpp
    widgets/ToggleButton.cpp
    widgets/SocialPlaylistWidget.cpp
    widgets/infowidgets/sourceinfowidget.cpp
    widgets/infowidgets/ArtistInfoWidget.cpp
    widgets/infowidgets/AlbumInfoWidget.cpp
    widgets/Breadcrumb.cpp
    widgets/BreadcrumbButton.cpp
)

IF(QCA2_FOUND)
    set( libGuiSources ${libGuiSources} utils/groovesharkparser.cpp )
ENDIF(QCA2_FOUND)

set( libGuiHeaders
    actioncollection.h

    contextmenu.h
    dropjob.h
    viewmanager.h
    globalactionmanager.h
    LatchManager.h
    TomahawkSettingsGui.h

    context/ContextPage.h
    context/ContextWidget.h
    context/pages/TopTracksContext.h
    context/pages/RelatedArtistsContext.h
    context/pages/WikipediaContext.h
    context/pages/WebContext.h

    infobar/infobar.h

    playlist/topbar/topbar.h
    playlist/topbar/clearbutton.h
    playlist/topbar/searchlineedit.h
    playlist/topbar/lineedit.h
    playlist/topbar/lineedit_p.h
    playlist/topbar/searchbutton.h

    playlist/treemodel.h
    playlist/treeproxymodel.h
    playlist/treeproxymodelplaylistinterface.h
    playlist/treeheader.h
    playlist/treeitemdelegate.h
    playlist/collectionproxymodel.h
    playlist/collectionproxymodelplaylistinterface.h
    playlist/collectionflatmodel.h
    playlist/collectionview.h
    playlist/playlistmodel.h
    playlist/playlistproxymodel.h
    playlist/playlistproxymodelplaylistinterface.h
    playlist/playlistview.h
    playlist/playlistitemdelegate.h
    playlist/queueproxymodel.h
    playlist/queueproxymodelplaylistinterface.h
    playlist/queueview.h
    playlist/trackmodel.h
    playlist/trackmodelitem.h
    playlist/trackproxymodel.h
    playlist/trackproxymodelplaylistinterface.h
    playlist/trackview.h
    playlist/trackheader.h
    playlist/treemodelitem.h
    playlist/albumitem.h
    playlist/albummodel.h
    playlist/albumproxymodel.h
    playlist/albumproxymodelplaylistinterface.h
    playlist/albumitemdelegate.h
    playlist/albumview.h
    playlist/artistview.h
    playlist/customplaylistview.h
    playlist/ViewHeader.h

    playlist/dynamic/DynamicPlaylist.h
    playlist/dynamic/GeneratorInterface.h
    playlist/dynamic/DynamicView.h
    playlist/dynamic/DynamicModel.h
    playlist/dynamic/echonest/EchonestGenerator.h
    playlist/dynamic/echonest/EchonestControl.h
    playlist/dynamic/echonest/EchonestSteerer.h
    playlist/dynamic/widgets/DynamicWidget.h
    playlist/dynamic/widgets/DynamicControlWrapper.h
    playlist/dynamic/widgets/DynamicControlList.h
    playlist/dynamic/widgets/ReadOrWriteWidget.h
    playlist/dynamic/widgets/MiscControlWidgets.h
    playlist/dynamic/widgets/CollapsibleControls.h
    playlist/dynamic/widgets/DynamicSetupWidget.h
    playlist/dynamic/widgets/LoadingSpinner.h

    ExternalResolverGui.h
    resolvers/scriptresolver.h
    resolvers/qtscriptresolver.h

    utils/widgetdragfilter.h
    utils/xspfgenerator.h
    utils/jspfloader.h
    utils/spotifyparser.h
    utils/m3uloader.h
    utils/itunesparser.h
    utils/rdioparser.h
    utils/shortenedlinkparser.h
    utils/dropjobnotifier.h

    widgets/checkdirtree.h
    widgets/querylabel.h
    widgets/animatedcounterlabel.h
    widgets/imagebutton.h
    widgets/animatedsplitter.h
    widgets/elidedlabel.h
    widgets/newplaylistwidget.h
    widgets/searchwidget.h
    widgets/SeekSlider.h
    widgets/playlisttypeselectordlg.h
    widgets/welcomewidget.h
    widgets/whatshotwidget.h
    widgets/whatshotwidget_p.h
    widgets/ChartDataLoader.h
    widgets/RecentlyPlayedPlaylistsModel.h
    widgets/RecentPlaylistsModel.h
    widgets/OverlayButton.h
    widgets/overlaywidget.h
    widgets/HeaderLabel.h
    widgets/HeaderWidget.h
    widgets/combobox.h
    widgets/ToggleButton.h
    widgets/SocialPlaylistWidget.h
    widgets/infowidgets/sourceinfowidget.h
    widgets/infowidgets/ArtistInfoWidget.h
    widgets/infowidgets/ArtistInfoWidget_p.h
    widgets/infowidgets/AlbumInfoWidget.h
    widgets/Breadcrumb.h
    widgets/BreadcrumbButton.h

    jobview/JobStatusView.h
    jobview/JobStatusModel.h
    jobview/JobStatusDelegate.h
    jobview/JobStatusItem.h
    jobview/PipelineStatusItem.h
    jobview/TransferStatusItem.h
    jobview/LatchedStatusItem.h
    jobview/ErrorStatusMessage.h

    thirdparty/Qocoa/qsearchfield.h
)

IF(QCA2_FOUND)
    set( libGuiHeaders ${libGuiHeaders} utils/groovesharkparser.h )
ENDIF(QCA2_FOUND)

set( libSources
    tomahawksettings.cpp
    sourcelist.cpp
    pipeline.cpp

    aclsystem.cpp
    artist.cpp
    artistplaylistinterface.cpp
    album.cpp
    albumplaylistinterface.cpp
    collection.cpp
    playlist.cpp
    playlistplaylistinterface.cpp
    resolver.cpp
    ExternalResolver.cpp
    query.cpp
    result.cpp
    source.cpp
    sourceplaylistinterface.cpp
    playlistinterface.cpp

    EchonestCatalogSynchronizer.cpp

    accounts/AccountManager.cpp
    accounts/Account.cpp
    accounts/AccountModel.cpp
    accounts/AccountModelFilterProxy.cpp
    accounts/ResolverAccount.cpp
    accounts/LastFmAccount.cpp
    accounts/LastFmConfig.cpp

    sip/SipPlugin.cpp
    sip/SipHandler.cpp
    sip/sipinfo.cpp

    audio/audioengine.cpp


    database/database.cpp
    database/fuzzyindex.cpp
    database/databasecollection.cpp
    database/localcollection.cpp
    database/databaseworker.cpp
    database/databaseimpl.cpp
    database/databaseresolver.cpp
    database/databasecommand.cpp
    database/databasecommandloggable.cpp
    database/databasecommand_resolve.cpp
    database/databasecommand_allartists.cpp
    database/databasecommand_allalbums.cpp
    database/databasecommand_alltracks.cpp
    database/databasecommand_addfiles.cpp
    database/databasecommand_deletefiles.cpp
    database/databasecommand_dirmtimes.cpp
    database/databasecommand_filemtimes.cpp
    database/databasecommand_loadfiles.cpp
    database/databasecommand_logplayback.cpp
    database/databasecommand_addsource.cpp
    database/databasecommand_sourceoffline.cpp
    database/databasecommand_collectionstats.cpp
    database/databasecommand_loadplaylistentries.cpp
    database/databasecommand_modifyplaylist.cpp
    database/databasecommand_playbackhistory.cpp
    database/databasecommand_setplaylistrevision.cpp
    database/databasecommand_loadallplaylists.cpp
    database/databasecommand_loadallsortedplaylists.cpp
    database/databasecommand_loadallsources.cpp
    database/databasecommand_createplaylist.cpp
    database/databasecommand_deleteplaylist.cpp
    database/databasecommand_renameplaylist.cpp
    database/databasecommand_loadops.cpp
    database/databasecommand_updatesearchindex.cpp
    database/databasecommand_setdynamicplaylistrevision.cpp
    database/databasecommand_createdynamicplaylist.cpp
    database/databasecommand_loaddynamicplaylist.cpp
    database/databasecommand_loaddynamicplaylistentries.cpp
    database/databasecommand_loadallautoplaylists.cpp
    database/databasecommand_loadallstations.cpp
    database/databasecommand_deletedynamicplaylist.cpp
    database/databasecommand_addclientauth.cpp
    database/databasecommand_clientauthvalid.cpp
    database/databasecommand_socialaction.cpp
    database/databasecommand_loadsocialactions.cpp
    database/databasecommand_genericselect.cpp
    database/databasecommand_setcollectionattributes.cpp
    database/databasecommand_collectionattributes.cpp
    database/databasecommand_trackattributes.cpp
    database/databasecommand_settrackattributes.cpp
    database/database.cpp

    infosystem/infosystem.cpp
    infosystem/infosystemcache.cpp
    infosystem/infosystemworker.cpp

    infosystem/infoplugins/generic/echonestplugin.cpp
    infosystem/infoplugins/generic/lastfmplugin.cpp
    infosystem/infoplugins/generic/chartsplugin.cpp
    infosystem/infoplugins/generic/spotifyPlugin.cpp
    infosystem/infoplugins/generic/hypemPlugin.cpp
    infosystem/infoplugins/generic/musixmatchplugin.cpp
    infosystem/infoplugins/generic/musicbrainzPlugin.cpp
    infosystem/infoplugins/generic/RoviPlugin.cpp

    network/bufferiodevice.cpp
    network/msgprocessor.cpp
    network/streamconnection.cpp
    network/dbsyncconnection.cpp
    network/remotecollection.cpp
    network/portfwdthread.cpp
    network/servent.cpp
    network/connection.cpp
    network/controlconnection.cpp

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

    utils/tomahawkutils.cpp
    utils/logger.cpp
    utils/qnr_iodevicestream.cpp
    utils/xspfloader.cpp

    thirdparty/kdsingleapplicationguard/kdsingleapplicationguard.cpp
    thirdparty/kdsingleapplicationguard/kdsharedmemorylocker.cpp
    thirdparty/kdsingleapplicationguard/kdtoolsglobal.cpp
    thirdparty/kdsingleapplicationguard/kdlockedsharedmemorypointer.cpp
)

set( libHeaders
    tomahawksettings.h
    sourcelist.h
    pipeline.h
    functimeout.h

    playlistinterface.h

    aclsystem.h
    collection.h
    query.h
    resolver.h
    ExternalResolver.h
    result.h
    source.h
    sourceplaylistinterface.h

    artist.h
    artistplaylistinterface.h
    album.h
    albumplaylistinterface.h
    playlist.h
    playlistplaylistinterface.h
    viewpage.h

    accounts/Account.h
    accounts/AccountManager.h
    accounts/AccountModel.h
    accounts/AccountModelFilterProxy.h
    accounts/ResolverAccount.h
    accounts/LastFmAccount.h
    accounts/LastFmConfig.h

    EchonestCatalogSynchronizer.h

    sip/SipPlugin.h
    sip/SipHandler.h
    sip/sipinfo.h

    audio/audioengine.h

    database/database.h
    database/fuzzyindex.h
    database/databaseworker.h
    database/databaseimpl.h
    database/databaseresolver.h
    database/databasecommand.h
    database/databasecommandloggable.h
    database/databasecommand_resolve.h
    database/databasecommand_allartists.h
    database/databasecommand_allalbums.h
    database/databasecommand_alltracks.h
    database/databasecommand_addfiles.h
    database/databasecommand_deletefiles.h
    database/databasecommand_dirmtimes.h
    database/databasecommand_filemtimes.h
    database/databasecommand_loadfiles.h
    database/databasecommand_logplayback.h
    database/databasecommand_addsource.h
    database/databasecommand_sourceoffline.h
    database/databasecommand_collectionstats.h
    database/databasecommand_loadplaylistentries.h
    database/databasecommand_modifyplaylist.h
    database/databasecommand_playbackhistory.h
    database/databasecommand_setplaylistrevision.h
    database/databasecommand_loadallplaylists.h
    database/databasecommand_loadallsortedplaylists.h
    database/databasecommand_loadallsources.h
    database/databasecommand_createplaylist.h
    database/databasecommand_deleteplaylist.h
    database/databasecommand_renameplaylist.h
    database/databasecommand_loadops.h
    database/databasecommand_updatesearchindex.h
    database/databasecollection.h
    database/localcollection.h
    database/databasecommand_setdynamicplaylistrevision.h
    database/databasecommand_createdynamicplaylist.h
    database/databasecommand_loaddynamicplaylist.h
    database/databasecommand_loaddynamicplaylistentries.h
    database/databasecommand_deletedynamicplaylist.h
    database/databasecommand_loadallautoplaylists.h
    database/databasecommand_loadallstations.h
    database/databasecommand_addclientauth.h
    database/databasecommand_clientauthvalid.h
    database/databasecommand_socialaction.h
    database/databasecommand_loadsocialactions.h
    database/databasecommand_genericselect.h
    database/databasecommand_setcollectionattributes.h
    database/databasecommand_collectionattributes.h
    database/databasecommand_trackattributes.h
    database/databasecommand_settrackattributes.h

    infosystem/infosystem.h
    infosystem/infosystem.h
    infosystem/infosystemworker.h
    infosystem/infosystemcache.h

    infosystem/infoplugins/generic/echonestplugin.h
    infosystem/infoplugins/generic/lastfmplugin.h
    infosystem/infoplugins/generic/chartsplugin.h
    infosystem/infoplugins/generic/spotifyPlugin.h
    infosystem/infoplugins/generic/hypemPlugin.h
    infosystem/infoplugins/generic/musixmatchplugin.h
    infosystem/infoplugins/generic/musicbrainzPlugin.h
    infosystem/infoplugins/generic/RoviPlugin.h

    network/bufferiodevice.h
    network/msgprocessor.h
    network/remotecollection.h
    network/streamconnection.h
    network/dbsyncconnection.h
    network/servent.h
    network/connection.h
    network/controlconnection.h
    network/portfwdthread.h

    playlist/PlaylistUpdaterInterface.h
    playlist/dynamic/DynamicPlaylist.h
    playlist/dynamic/GeneratorInterface.h
    playlist/dynamic/GeneratorFactory.h
    playlist/XspfUpdater.h
    playlist/dynamic/database/DatabaseGenerator.h
    playlist/dynamic/database/DatabaseControl.h
    playlist/dynamic/DynamicControl.h

    thirdparty/kdsingleapplicationguard/kdsingleapplicationguard.h

    utils/tomahawkutilsgui.h
    utils/xspfloader.h
    utils/qnr_iodevicestream.h
)

set( libHeaders_NoMOC
    viewpage.h

    accounts/Account.h

    infosystem/infoplugins/unix/imageconverter.h

    taghandlers/tag.h
    taghandlers/apetag.h
    taghandlers/asftag.h
    taghandlers/id3v1tag.h
    taghandlers/id3v2tag.h
    taghandlers/mp4tag.h
    taghandlers/oggtag.h

    utils/tomahawkutils.h
)

set( libUI ${libUI}
    widgets/playlisttypeselectordlg.ui
    widgets/newplaylistwidget.ui
    widgets/searchwidget.ui
    widgets/welcomewidget.ui
    widgets/whatshotwidget.ui
    widgets/SocialPlaylistWidget.ui
    widgets/infowidgets/sourceinfowidget.ui
    widgets/infowidgets/ArtistInfoWidget.ui
    widgets/infowidgets/AlbumInfoWidget.ui
    playlist/topbar/topbar.ui
    playlist/queueview.ui
    context/ContextWidget.ui
    infobar/infobar.ui
    accounts/LastFmConfig.ui
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
    LIST(APPEND libGuiSources
      infosystem/infoplugins/unix/mprispluginrootadaptor.cpp
      infosystem/infoplugins/unix/mprispluginplayeradaptor.cpp
      infosystem/infoplugins/unix/mprisplugin.cpp
      infosystem/infoplugins/unix/fdonotifyplugin.cpp
      infosystem/infoplugins/unix/imageconverter.cpp )

    LIST(APPEND libGuiHeaders
      infosystem/infoplugins/unix/mprispluginrootadaptor.h
      infosystem/infoplugins/unix/mprispluginplayeradaptor.h
      infosystem/infoplugins/unix/mprisplugin.h
      infosystem/infoplugins/unix/fdonotifyplugin.h )

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
      infosystem/infoplugins/mac/adium.mm
      infosystem/infoplugins/mac/adiumplugin.cpp
      utils/tomahawkutils_mac.mm
      thirdparty/Qocoa/qsearchfield_mac.mm )

    SET( libHeaders ${libHeaders}
      infosystem/infoplugins/mac/adium.h
      infosystem/infoplugins/mac/adiumplugin.h )

    SET( OS_SPECIFIC_LINK_LIBRARIES
        ${OS_SPECIFIC_LINK_LIBRARIES}
        # System
        ${COREAUDIO_LIBRARY}
        ${COREFOUNDATION_LIBRARY}
	${FOUNDATION_LIBRARY}
	${SCRIPTINGBRIDGE_LIBRARY}

        /System/Library/Frameworks/AppKit.framework
    )
ELSE( APPLE )
    SET( libGuiSources ${libGuiSources} thirdparty/Qocoa/qsearchfield.cpp )
ENDIF( APPLE )

IF(LIBLASTFM_FOUND)
    SET(LINK_LIBRARIES ${LINK_LIBRARIES} tomahawk_lastfm2 )
ENDIF(LIBLASTFM_FOUND)

IF(BUILD_GUI)
    LIST(APPEND libSources ${libGuiSources} )
    LIST(APPEND libHeaders ${libGuiHeaders} )
ENDIF()

qt4_wrap_ui( libUI_H ${libUI} )
qt4_wrap_cpp( libMoc ${libHeaders} )

SET( libSources ${libSources} ${libUI_H} ${libHeaders_NoMOC} )

ADD_LIBRARY( tomahawklib SHARED ${libSources} ${libMoc} )

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
