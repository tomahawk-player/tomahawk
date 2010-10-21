PROJECT( tomahawk )
CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )

IF( "${gui}" STREQUAL "no" )
    SET( QT_DONT_USE_QTGUI TRUE )
ENDIF()
SET( QT_USE_QTSQL TRUE )
SET( QT_USE_QTNETWORK TRUE )
SET( QT_USE_QTXML TRUE )
INCLUDE( ${QT_USE_FILE} )

SET( CMAKE_BUILD_TYPE "debugfull" )
SET( CMAKE_VERBOSE_MAKEFILE ON )
SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}" )
SET( CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}" )
SET( CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}" )
SET( TOMAHAWK_INC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../include/" )

# build plugins
# use glob, but hardcoded list for now:
#FILE( GLOB plugindirs "src/plugins/*" )
#FOREACH( moddir ${plugindirs} )
#    MESSAGE( status "Building plugin: ${moddir}" )
#    ADD_SUBDIRECTORY( ${moddir} )
#ENDFOREACH( moddir )

SET( tomahawkSources ${tomahawkSources}
     pipeline.cpp
     playlist.cpp
     pluginapi.cpp
     query.cpp
     result.cpp
     source.cpp
     sourcelist.cpp

     utils/tomahawkutils.cpp
     jabber/jabber_p.cpp

     bufferiodevice.cpp
     connection.cpp
     msgprocessor.cpp
     controlconnection.cpp
     collection.cpp
     filetransferconnection.cpp
     dbsyncconnection.cpp
     musicscanner.cpp
     remotecollection.cpp
     servent.cpp
     scriptresolver.cpp

     database/fuzzyindex.cpp
     database/database.cpp
     database/databaseworker.cpp
     database/databaseimpl.cpp
     database/databaseresolver.cpp
     database/databasecommand.cpp
     database/databasecommandloggable.cpp
     database/databasecommand_resolve.cpp
     database/databasecommand_alltracks.cpp
     database/databasecommand_addfiles.cpp
     database/databasecommand_dirmtimes.cpp
     database/databasecommand_loadfile.cpp
     database/databasecommand_addsource.cpp
     database/databasecommand_sourceoffline.cpp
     database/databasecommand_collectionstats.cpp
     database/databasecommand_loadplaylistentries.cpp
     database/databasecommand_modifyplaylist.cpp
     database/databasecommand_setplaylistrevision.cpp
     database/databasecommand_loadallplaylists.cpp
     database/databasecommand_createplaylist.cpp
     database/databasecommand_deleteplaylist.cpp
     database/databasecommand_loadops.cpp
     database/databasecommand_updatesearchindex.cpp
     database/databasecollection.cpp

     web/api_v1.cpp

     tomahawksettings.cpp
     tomahawkapp.cpp
     main.cpp
)

SET( tomahawkSourcesGui ${tomahawkSourcesGui}
     xspfloader.cpp

     utils/imagebutton.cpp
     utils/progresstreeview.cpp

     audio/madtranscode.cpp
     audio/audioengine.cpp

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
     playlist/trackmodel.cpp
     playlist/trackproxymodel.cpp

     sourcetree/sourcesmodel.cpp
     sourcetree/sourcetreeitem.cpp
     sourcetree/sourcetreeitemwidget.cpp
     sourcetree/sourcetreeview.cpp

     topbar/topbar.cpp
     topbar/clearbutton.cpp  
     topbar/searchlineedit.cpp  
     topbar/lineedit.cpp
     topbar/searchbutton.cpp

     tomahawkwindow.cpp
     audiocontrols.cpp
     settingsdialog.cpp
     proxystyle.cpp

     widgetdragfilter.cpp
)

SET( tomahawkHeaders ${tomahawkHeaders}
     "${TOMAHAWK_INC_DIR}/tomahawk/tomahawkapp.h"

     "${TOMAHAWK_INC_DIR}/tomahawk/collection.h"
     "${TOMAHAWK_INC_DIR}/tomahawk/pipeline.h"
     "${TOMAHAWK_INC_DIR}/tomahawk/pluginapi.h"
     "${TOMAHAWK_INC_DIR}/tomahawk/query.h"
     "${TOMAHAWK_INC_DIR}/tomahawk/resolver.h"
     "${TOMAHAWK_INC_DIR}/tomahawk/result.h"
     "${TOMAHAWK_INC_DIR}/tomahawk/source.h"
     "${TOMAHAWK_INC_DIR}/tomahawk/sourcelist.h"

     "${TOMAHAWK_INC_DIR}/tomahawk/artist.h"
     "${TOMAHAWK_INC_DIR}/tomahawk/album.h"
     "${TOMAHAWK_INC_DIR}/tomahawk/track.h"
     "${TOMAHAWK_INC_DIR}/tomahawk/playlist.h"

     "${TOMAHAWK_INC_DIR}/tomahawk/functimeout.h"
#     "${TOMAHAWK_INC_DIR}/tomahawk/tomahawkplugin.h"

     database/fuzzyindex.h
     database/database.h
     database/databaseworker.h
     database/databaseimpl.h
     database/databaseresolver.h
     database/databasecommand.h
     database/databasecommandloggable.h
     database/databasecommand_resolve.h
     database/databasecommand_alltracks.h
     database/databasecommand_addfiles.h
     database/databasecommand_dirmtimes.h
     database/databasecommand_loadfile.h
     database/databasecommand_addsource.h
     database/databasecommand_sourceoffline.h
     database/databasecommand_collectionstats.h
     database/databasecommand_loadplaylistentries.h
     database/databasecommand_modifyplaylist.h
     database/databasecommand_setplaylistrevision.h
     database/databasecommand_loadallplaylists.h
     database/databasecommand_createplaylist.h
     database/databasecommand_deleteplaylist.h
     database/databasecommand_loadops.h
     database/databasecommand_updatesearchindex.h
     database/databasecollection.h

     jabber/jabber.h
     jabber/jabber_p.h

     bufferiodevice.h
     connection.h
     msgprocessor.h
     controlconnection.h
     filetransferconnection.h
     dbsyncconnection.h
     musicscanner.h
     tomahawkzeroconf.h 
     remotecollection.h
     servent.h
     scriptresolver.h
     tomahawksettings.h

     web/api_v1.h
)

SET( tomahawkHeadersGui ${tomahawkHeadersGui}
     xspfloader.h

     utils/animatedcounterlabel.h
     utils/imagebutton.h
     utils/progresstreeview.h

     audio/transcodeinterface.h
     audio/madtranscode.h
     audio/audioengine.h

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
     playlist/trackmodel.h
     playlist/trackproxymodel.h

     sourcetree/sourcesmodel.h
     sourcetree/sourcetreeitem.h
     sourcetree/sourcetreeitemwidget.h
     sourcetree/sourcetreeview.h

     topbar/topbar.h
     topbar/clearbutton.h 
     topbar/searchlineedit.h 
     topbar/lineedit.h
     topbar/lineedit_p.h
     topbar/searchbutton.h

     tomahawkwindow.h
     audiocontrols.h
     settingsdialog.h

     widgetdragfilter.h
)

SET( tomahawkUI ${tomahawkUI}
     tomahawkwindow.ui 
     settingsdialog.ui 

     audiocontrols.ui 
     sourcetree/sourcetreeitemwidget.ui 
     topbar/topbar.ui 
)

INCLUDE_DIRECTORIES(
    . 
    ${TOMAHAWK_INC_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}

    audio
    database
    playlist
    sourcetree
    topbar
    utils

    ../rtaudio
    ../alsa-playback
    ../libportfwd/include
    ../qxtweb-standalone/qxtweb/

    /usr/include/taglib
    /usr/local/include/taglib
    /usr/local/include
)

SET( OS_SPECIFIC_LINK_LIBRARIES "" )

IF( WIN32 )
  INCLUDE( "CMakeLists.win32.txt" )
ENDIF( WIN32 )
IF( UNIX )
  INCLUDE( "CMakeLists.unix.txt" )
ENDIF( UNIX )
IF( APPLE )
  INCLUDE( "CMakeLists.osx.txt" )
ENDIF( APPLE )
IF( UNIX AND NOT APPLE )
  INCLUDE( "CMakeLists.linux.txt" )
ENDIF( UNIX AND NOT APPLE )

qt4_add_resources( RC_SRCS "../resources.qrc" )
qt4_wrap_cpp( tomahawkMoc ${tomahawkHeaders} )

SET( final_src ${final_src} ${tomahawkMoc} ${tomahawkSources} ${tomahawkHeaders} )

IF( "${gui}" STREQUAL "no" )
ELSE()
	qt4_wrap_ui( tomahawkUI_H ${tomahawkUI} )
	qt4_wrap_cpp( tomahawkMocGui ${tomahawkHeadersGui} )
	SET( final_src ${final_src} ${tomahawkUI_H} ${tomahawkMocGui} ${tomahawkSourcesGui} ${RC_SRCS} )
ENDIF()

IF( UNIX AND NOT APPLE )
    ADD_EXECUTABLE( tomahawk ${final_src} )
ENDIF( UNIX AND NOT APPLE )
IF( APPLE )
    ADD_EXECUTABLE( tomahawk MACOSX_BUNDLE ${final_src} )
ENDIF( APPLE )
IF( WIN32 )
    ADD_EXECUTABLE( tomahawk ${final_src} )
ENDIF( WIN32 )

MESSAGE( STATUS "OS_SPECIFIC_LINK_LIBRARIES: ${OS_SPECIFIC_LINK_LIBRARIES}" )

TARGET_LINK_LIBRARIES( tomahawk
    ${QT_LIBRARIES}
    ${MAC_EXTRA_LIBS}
    ${OS_SPECIFIC_LINK_LIBRARIES}
    portfwd
)

INCLUDE( "CPack.txt" )
