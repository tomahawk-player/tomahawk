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

set( libSources
    tomahawksettings.cpp
    sourcelist.cpp
    pipeline.cpp

    aclsystem.cpp
    artist.cpp
    album.cpp
    collection.cpp
    playlist.cpp
    resolver.cpp
    query.cpp
    result.cpp
    source.cpp
    sourceplaylistinterface.cpp
    viewpage.cpp
    viewmanager.cpp
    globalactionmanager.cpp
    contextmenu.cpp
    dropjob.cpp

    sip/SipPlugin.cpp
    sip/SipHandler.cpp
    sip/SipModel.cpp
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
    database/databasecommand_loadfile.cpp
    database/databasecommand_logplayback.cpp
    database/databasecommand_addsource.cpp
    database/databasecommand_sourceoffline.cpp
    database/databasecommand_collectionstats.cpp
    database/databasecommand_loadplaylistentries.cpp
    database/databasecommand_modifyplaylist.cpp
    database/databasecommand_playbackhistory.cpp
    database/databasecommand_setplaylistrevision.cpp
    database/databasecommand_loadallplaylists.cpp
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
    database/database.cpp

    infosystem/infosystemcache.cpp
    infosystem/infosystem.cpp
    infosystem/infosystemworker.cpp
    infosystem/infoplugins/generic/echonestplugin.cpp
    infosystem/infoplugins/generic/lastfmplugin.cpp
    infosystem/infoplugins/generic/musixmatchplugin.cpp
    infosystem/infoplugins/generic/musicbrainzPlugin.cpp

    playlist/treemodel.cpp
    playlist/treeproxymodel.cpp
    playlist/treeheader.cpp
    playlist/treeitemdelegate.cpp
    playlist/collectionproxymodel.cpp
    playlist/collectionflatmodel.cpp
    playlist/collectionview.cpp
    playlist/playlistmodel.cpp
    playlist/playlistproxymodel.cpp
    playlist/playlistview.cpp
    playlist/playlistitemdelegate.cpp
    playlist/queueproxymodel.cpp
    playlist/queueview.cpp
    playlist/trackmodel.cpp
    playlist/trackmodelitem.cpp
    playlist/trackproxymodel.cpp
    playlist/trackview.cpp
    playlist/trackheader.cpp
    playlist/treemodelitem.cpp
    playlist/albumitem.cpp
    playlist/albummodel.cpp
    playlist/albumproxymodel.cpp
    playlist/albumitemdelegate.cpp
    playlist/albumview.cpp
    playlist/artistview.cpp
    playlist/customplaylistview.cpp

    playlist/topbar/topbar.cpp
    playlist/topbar/clearbutton.cpp
    playlist/topbar/searchlineedit.cpp
    playlist/topbar/lineedit.cpp
    playlist/topbar/searchbutton.cpp

    playlist/infobar/infobar.cpp

    playlist/dynamic/DynamicPlaylist.cpp
    playlist/dynamic/DynamicControl.cpp
    playlist/dynamic/GeneratorFactory.cpp
    playlist/dynamic/GeneratorInterface.cpp
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
    playlist/dynamic/database/DatabaseControl.cpp
    playlist/dynamic/database/DatabaseGenerator.cpp

    network/bufferiodevice.cpp
    network/msgprocessor.cpp
    network/streamconnection.cpp
    network/dbsyncconnection.cpp
    network/remotecollection.cpp
    network/portfwdthread.cpp
    network/servent.cpp
    network/connection.cpp
    network/controlconnection.cpp

    utils/tomahawkutils.cpp
    utils/querylabel.cpp
    utils/elidedlabel.cpp
    utils/imagebutton.cpp
    utils/logger.cpp
    utils/proxystyle.cpp
    utils/widgetdragfilter.cpp
    utils/animatedsplitter.cpp
    utils/xspfloader.cpp
    utils/xspfgenerator.cpp
    utils/jspfloader.cpp
    utils/spotifyparser.cpp
    utils/rdioparser.cpp
    utils/shortenedlinkparser.cpp

    widgets/newplaylistwidget.cpp
    widgets/searchwidget.cpp
    widgets/SeekSlider.cpp
    widgets/playlisttypeselectordlg.cpp
    widgets/welcomewidget.cpp
    widgets/welcomeplaylistmodel.cpp
    widgets/overlaywidget.cpp
    widgets/HeaderLabel.cpp
    widgets/SocialPlaylistWidget.cpp
    widgets/infowidgets/sourceinfowidget.cpp
    widgets/infowidgets/ArtistInfoWidget.cpp
    widgets/infowidgets/AlbumInfoWidget.cpp

    kdsingleapplicationguard/kdsingleapplicationguard.cpp
    kdsingleapplicationguard/kdsharedmemorylocker.cpp
    kdsingleapplicationguard/kdtoolsglobal.cpp
    kdsingleapplicationguard/kdlockedsharedmemorypointer.cpp
)

set( libHeaders
    tomahawksettings.h
    sourcelist.h
    pipeline.h
    functimeout.h

    aclsystem.h
    collection.h
    query.h
    resolver.h
    result.h
    source.h
    sourceplaylistinterface.h
    viewmanager.h
    globalactionmanager.h
    contextmenu.h
    dropjob.h

    artist.h
    album.h
    playlist.h

    sip/SipPlugin.h
    sip/SipHandler.h
    sip/SipModel.h
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
    database/databasecommand_loadfile.h
    database/databasecommand_logplayback.h
    database/databasecommand_addsource.h
    database/databasecommand_sourceoffline.h
    database/databasecommand_collectionstats.h
    database/databasecommand_loadplaylistentries.h
    database/databasecommand_modifyplaylist.h
    database/databasecommand_playbackhistory.h
    database/databasecommand_setplaylistrevision.h
    database/databasecommand_loadallplaylists.h
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

    infosystem/infosystem.h
    infosystem/infosystemworker.h
    infosystem/infosystemcache.h
    infosystem/infoplugins/generic/echonestplugin.h
    infosystem/infoplugins/generic/lastfmplugin.h
    infosystem/infoplugins/generic/musixmatchplugin.h
    infosystem/infoplugins/generic/musicbrainzPlugin.h

    network/bufferiodevice.h
    network/msgprocessor.h
    network/remotecollection.h
    network/streamconnection.h
    network/dbsyncconnection.h
    network/servent.h
    network/connection.h
    network/controlconnection.h
    network/portfwdthread.h

    playlist/treemodel.h
    playlist/treeproxymodel.h
    playlist/treeheader.h
    playlist/treeitemdelegate.h
    playlist/collectionproxymodel.h
    playlist/collectionflatmodel.h
    playlist/collectionview.h
    playlist/playlistmodel.h
    playlist/playlistproxymodel.h
    playlist/playlistview.h
    playlist/playlistitemdelegate.h
    playlist/queueproxymodel.h
    playlist/queueview.h
    playlist/trackmodel.h
    playlist/trackmodelitem.h
    playlist/trackproxymodel.h
    playlist/trackview.h
    playlist/trackheader.h
    playlist/treemodelitem.h
    playlist/albumitem.h
    playlist/albummodel.h
    playlist/albumproxymodel.h
    playlist/albumitemdelegate.h
    playlist/albumview.h
    playlist/artistview.h
    playlist/customplaylistview.h

    playlist/topbar/topbar.h
    playlist/topbar/clearbutton.h
    playlist/topbar/searchlineedit.h
    playlist/topbar/lineedit.h
    playlist/topbar/lineedit_p.h
    playlist/topbar/searchbutton.h

    playlist/infobar/infobar.h

    playlist/dynamic/DynamicPlaylist.h
    playlist/dynamic/DynamicControl.h
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
    playlist/dynamic/database/DatabaseControl.h
    playlist/dynamic/database/DatabaseGenerator.h

    utils/querylabel.h
    utils/elidedlabel.h
    utils/animatedcounterlabel.h
    utils/imagebutton.h
    utils/widgetdragfilter.h
    utils/animatedsplitter.h
    utils/xspfloader.h
    utils/xspfgenerator.h
    utils/jspfloader.h
    utils/spotifyparser.h
    utils/rdioparser.h
    utils/shortenedlinkparser.h

    widgets/newplaylistwidget.h
    widgets/searchwidget.h
    widgets/SeekSlider.h
    widgets/playlisttypeselectordlg.h
    widgets/welcomewidget.h
    widgets/welcomeplaylistmodel.h
    widgets/overlaywidget.h
    widgets/HeaderLabel.h
    widgets/SocialPlaylistWidget.h
    widgets/infowidgets/sourceinfowidget.h
    widgets/infowidgets/ArtistInfoWidget.h
    widgets/infowidgets/AlbumInfoWidget.h

    kdsingleapplicationguard/kdsingleapplicationguard.h
)

set( libHeaders_NoMOC
    viewpage.h

    infosystem/infoplugins/unix/imageconverter.h

    playlist/dynamic/GeneratorInterface.h
    playlist/dynamic/GeneratorFactory.h

    utils/tomahawkutils.h
)

set( libUI ${libUI}
    widgets/playlisttypeselectordlg.ui
    widgets/newplaylistwidget.ui
    widgets/searchwidget.ui
    widgets/welcomewidget.ui
    widgets/SocialPlaylistWidget.ui
    widgets/infowidgets/sourceinfowidget.ui
    widgets/infowidgets/ArtistInfoWidget.ui
    widgets/infowidgets/AlbumInfoWidget.ui
    playlist/topbar/topbar.ui
    playlist/infobar/infobar.ui
)

include_directories( . ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR}/.. ..
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${QT_INCLUDE_DIR}
    ${QJSON_INCLUDE_DIR}
    ${LIBECHONEST_INCLUDE_DIR}
    ${LIBECHONEST_INCLUDE_DIR}/..
    ${CLUCENE_INCLUDE_DIR}
    ${CLUCENE_LIBRARY_DIR}
    ${PHONON_INCLUDES}
    ${CMAKE_BINARY_DIR}/thirdparty/liblastfm2/src

    playlist

    ${LIBPORTFWD_INCLUDE_DIR}
    ${THIRDPARTY_DIR}/qxt/qxtweb-standalone/qxtweb

)

IF( UNIX AND NOT APPLE )
    SET( libSources ${libSources}
      infosystem/infoplugins/unix/fdonotifyplugin.cpp
      infosystem/infoplugins/unix/imageconverter.cpp )

    SET( libHeaders ${libHeaders}
      infosystem/infoplugins/unix/fdonotifyplugin.h )

    IF( BUILD_GUI AND X11_FOUND )
        INCLUDE_DIRECTORIES( ${THIRDPARTY_DIR}/libqnetwm )
        SET( libSources ${libSources} ${THIRDPARTY_DIR}/libqnetwm/libqnetwm/netwm.cpp )
        SET( LINK_LIBRARIES ${LINK_LIBRARIES} ${X11_LIBRARIES} Xcomposite )
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
      widgets/maclineedit.mm
      utils/tomahawkutils_mac.mm )

    SET( libHeaders ${libHeaders}
      widgets/maclineedit.h
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
ENDIF( APPLE )

IF(LIBLASTFM_FOUND)
    SET(LINK_LIBRARIES ${LINK_LIBRARIES} tomahawk_lastfm2 )
ENDIF(LIBLASTFM_FOUND)

qt4_wrap_ui( libUI_H ${libUI} )
qt4_wrap_cpp( libMoc ${libHeaders} )

SET( libSources ${libSources} ${libUI_H} ${libHeaders_NoMOC} )

add_library( tomahawklib SHARED ${libSources} ${libMoc} )

target_link_libraries( tomahawklib
    # Thirdparty shipped with tomahawk
    ${LIBPORTFWD_LIBRARIES}

    # External deps
    ${QJSON_LIBRARIES}
    ${PHONON_LIBS}
    ${TAGLIB_LIBRARIES}
    ${CLUCENE_LIBRARIES}
    ${LIBECHONEST_LIBRARY}
    ${QT_QTUITOOLS_LIBRARY}
    ${QT_LIBRARIES}
    ${OS_SPECIFIC_LINK_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${LINK_LIBRARIES}
)

INSTALL( TARGETS tomahawklib
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib${LIB_SUFFIX}
    ARCHIVE DESTINATION lib${LIB_SUFFIX}
)
