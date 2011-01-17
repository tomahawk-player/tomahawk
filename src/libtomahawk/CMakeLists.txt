project( tomahawklib )

SET( QT_USE_QTGUI TRUE )
SET( QT_USE_QTSQL TRUE )
SET( QT_USE_QTNETWORK TRUE )
SET( QT_USE_QTXML TRUE )

include( ${QT_USE_FILE} )

add_definitions( ${QT_DEFINITIONS} )
add_definitions( -DQT_SHARED )
add_definitions( -DDLLEXPORT_PRO )

set( libSources
    tomahawksettings.cpp
    sourcelist.cpp
    pipeline.cpp
    artist.cpp
    album.cpp
    collection.cpp
    playlist.cpp
    pluginapi.cpp
    query.cpp
    result.cpp
    source.cpp

    sip/SipPlugin.cpp

    audio/madtranscode.cpp
    audio/vorbistranscode.cpp
    audio/flactranscode.cpp
    audio/audioengine.cpp

    database/database.cpp
    database/databasecollection.cpp
    database/databaseworker.cpp
    database/databaseimpl.cpp
    database/databaseresolver.cpp
    database/databasecommand.cpp
    database/databasecommandloggable.cpp
    database/databasecommand_resolve.cpp
    database/databasecommand_allalbums.cpp
    database/databasecommand_alltracks.cpp
    database/databasecommand_addfiles.cpp
    database/databasecommand_dirmtimes.cpp
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
    database/databasecommand_createplaylist.cpp
    database/databasecommand_deleteplaylist.cpp
    database/databasecommand_renameplaylist.cpp
    database/databasecommand_loadops.cpp
    database/databasecommand_updatesearchindex.cpp
    database/fuzzyindex.cpp

    playlist/collectionmodel.cpp
    playlist/collectionproxymodel.cpp
    playlist/collectionflatmodel.cpp
    playlist/collectionview.cpp
    playlist/playlistmanager.cpp
    playlist/plitem.cpp
    playlist/playlistmodel.cpp
    playlist/playlistproxymodel.cpp
    playlist/playlistview.cpp
    playlist/playlistitemdelegate.cpp
    playlist/queueproxymodel.cpp
    playlist/queueview.cpp
    playlist/trackmodel.cpp
    playlist/trackproxymodel.cpp
    playlist/trackview.cpp
    playlist/trackheader.cpp
    playlist/albumitem.cpp
    playlist/albummodel.cpp
    playlist/albumproxymodel.cpp
    playlist/albumitemdelegate.cpp
    playlist/albumview.cpp

    network/bufferiodevice.cpp
    network/msgprocessor.cpp
    network/filetransferconnection.cpp
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
    utils/progresstreeview.cpp
    utils/proxystyle.cpp
    utils/widgetdragfilter.cpp
    utils/animatedsplitter.cpp
    utils/xspfloader.cpp

    widgets/newplaylistwidget.cpp
    widgets/welcomewidget.cpp
    widgets/infowidgets/sourceinfowidget.cpp
)

set( libHeaders
    tomahawksettings.h
    sourcelist.h
    pipeline.h
    functimeout.h
    collection.h
    pluginapi.h
    query.h
    resolver.h
    result.h
    source.h
    artist.h
    album.h
    track.h
    playlist.h

    sip/SipPlugin.h

    audio/transcodeinterface.h
    audio/madtranscode.h
    audio/vorbistranscode.h
    audio/flactranscode.h
    audio/audioengine.h

    database/database.h
    database/databasecollection.h
    database/databaseworker.h
    database/databaseimpl.h
    database/databaseresolver.h
    database/databasecommand.h
    database/databasecommandloggable.h
    database/databasecommand_resolve.h
    database/databasecommand_allalbums.h
    database/databasecommand_alltracks.h
    database/databasecommand_addfiles.h
    database/databasecommand_dirmtimes.h
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
    database/databasecommand_createplaylist.h
    database/databasecommand_deleteplaylist.h
    database/databasecommand_renameplaylist.h
    database/databasecommand_loadops.h
    database/databasecommand_updatesearchindex.h
    database/fuzzyindex.h

    network/bufferiodevice.h
    network/msgprocessor.h
    network/remotecollection.h
    network/filetransferconnection.h
    network/dbsyncconnection.h
    network/servent.h
    network/connection.h
    network/controlconnection.h
    network/portfwdthread.h

    playlist/collectionmodel.h
    playlist/collectionproxymodel.h
    playlist/collectionflatmodel.h
    playlist/collectionview.h
    playlist/playlistmanager.h
    playlist/plitem.h
    playlist/playlistmodel.h
    playlist/playlistproxymodel.h
    playlist/playlistview.h
    playlist/playlistitemdelegate.h
    playlist/queueproxymodel.h
    playlist/queueview.h
    playlist/trackmodel.h
    playlist/trackproxymodel.h
    playlist/trackview.h
    playlist/trackheader.h
    playlist/albumitem.h
    playlist/albummodel.h
    playlist/albumproxymodel.h
    playlist/albumitemdelegate.h
    playlist/albumview.h

    utils/querylabel.h
    utils/elidedlabel.h
    utils/animatedcounterlabel.h
    utils/imagebutton.h
    utils/progresstreeview.h
    utils/widgetdragfilter.h
    utils/animatedsplitter.h
    utils/xspfloader.h

    widgets/newplaylistwidget.h
    widgets/welcomewidget.h
    widgets/infowidgets/sourceinfowidget.h
)

set( libUI ${libUI}
    widgets/newplaylistwidget.ui
    widgets/welcomewidget.ui
    widgets/infowidgets/sourceinfowidget.ui
)

include_directories( . ${CMAKE_CURRENT_BINARY_DIR} .. 
    ${QT_INCLUDE_DIR}
    ../../libportfwd/include
    ../../include
    ../network
    ../../qxt/qxtweb-standalone/qxtweb
    ../../rtaudio
    ../../alsa-playback
)


IF( WIN32 )
SET( libSources ${libSources} audio/rtaudiooutput.cpp )
SET( libHeaders ${libHeaders} audio/rtaudiooutput.h )

SET( OS_SPECIFIC_LINK_LIBRARIES
    ${OS_SPECIFIC_LINK_LIBRARIES}
    "iphlpapi.a"
    "ws2_32.dll"
    "dnsapi.dll"
    "dsound.dll" 
    "winmm.dll"
    "mad"
    "vorbisfile"
    "ogg"
    "FLAC++"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../rtaudio/librtaudio.dll"
)
ENDIF( WIN32 )

IF( APPLE )
SET( libSources ${libSources} audio/rtaudiooutput.cpp )
SET( libHeaders ${libHeaders} audio/rtaudiooutput.h )

SET( OS_SPECIFIC_LINK_LIBRARIES
    ${OS_SPECIFIC_LINK_LIBRARIES}
    ${COREAUDIO_LIBRARY}
    ${COREFOUNDATION_LIBRARY}
    rtaudio
)

ENDIF( APPLE )

IF( UNIX AND NOT APPLE )
SET( OS_SPECIFIC_LINK_LIBRARIES 
     ${OS_SPECIFIC_LINK_LIBRARIES}
     alsaplayback
)
ENDIF( UNIX AND NOT APPLE )


qt4_wrap_ui( libUI_H ${libUI} )
qt4_wrap_cpp( libMoc ${libHeaders} )

SET( libSources ${libSources} ${libUI_H} )

add_library( tomahawklib SHARED ${libSources} ${libMoc} )

target_link_libraries( tomahawklib
    ${QT_LIBRARIES}
    ${OS_SPECIFIC_LINK_LIBRARIES}
    portfwd
    qjson
)

install( TARGETS tomahawklib DESTINATION lib )
