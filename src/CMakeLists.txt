PROJECT( tomahawk )
CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )

IF( "${gui}" STREQUAL "no" )
    SET( QT_DONT_USE_QTGUI TRUE )
ENDIF()
SET( QT_USE_QTSQL TRUE )
SET( QT_USE_QTNETWORK TRUE )
SET( QT_USE_QTXML TRUE )

INCLUDE( ${QT_USE_FILE} )
INCLUDE( ${CMAKE_MODULE_PATH}/AddAppIconMacro.cmake )

#SET( CMAKE_BUILD_TYPE "Release" )
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
     audio/madtranscode.cpp
     audio/vorbistranscode.cpp
     audio/flactranscode.cpp
     audio/audioengine.cpp

     utils/tomahawkutils.cpp

     sip/SipHandler.cpp

     infosystem/infosystem.cpp
     infosystem/infoplugins/echonestplugin.cpp
     infosystem/infoplugins/musixmatchplugin.cpp

     xmppbot/xmppbot.cpp
     web/api_v1.cpp

     musicscanner.cpp
     scriptresolver.cpp
     scrobbler.cpp

     tomahawkapp.cpp
     main.cpp
)

SET( tomahawkSourcesGui ${tomahawkSourcesGui}
     utils/querylabel.cpp
     utils/elidedlabel.cpp
     utils/imagebutton.cpp
     utils/progresstreeview.cpp
     utils/proxystyle.cpp
     utils/widgetdragfilter.cpp
     utils/animatedsplitter.cpp

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

     sourcetree/sourcesmodel.cpp
     sourcetree/sourcetreeitem.cpp
     sourcetree/sourcetreeitemwidget.cpp
     sourcetree/sourcetreeview.cpp

     topbar/topbar.cpp
     topbar/clearbutton.cpp  
     topbar/searchlineedit.cpp  
     topbar/lineedit.cpp
     topbar/searchbutton.cpp

     infowidgets/sourceinfowidget.cpp

     dynamic/widgets/DynamicWidget.cpp
     dynamic/widgets/DynamicControlWidget.cpp
     dynamic/widgets/DynamicControlList.cpp
     dynamic/widgets/ReadOrWriteWidget.cpp

     widgets/newplaylistwidget.cpp
     widgets/welcomewidget.cpp

     xspfloader.cpp
     transferview.cpp
     tomahawktrayicon.cpp
     audiocontrols.cpp
     settingsdialog.cpp
     tomahawkwindow.cpp
)

SET( tomahawkHeaders ${tomahawkHeaders}
     "${TOMAHAWK_INC_DIR}/tomahawk/tomahawkapp.h"
     "${TOMAHAWK_INC_DIR}/tomahawk/infosystem.h"

     audio/transcodeinterface.h
     audio/madtranscode.h
     audio/vorbistranscode.h
     audio/flactranscode.h
     audio/audioengine.h

     sip/SipHandler.h

     infosystem/infoplugins/echonestplugin.h
     infosystem/infoplugins/musixmatchplugin.h

     xmppbot/xmppbot.h
     web/api_v1.h

     musicscanner.h
     scriptresolver.h
     scrobbler.h
)

SET( tomahawkHeadersGui ${tomahawkHeadersGui}
     utils/querylabel.h
     utils/elidedlabel.h
     utils/animatedcounterlabel.h
     utils/imagebutton.h
     utils/progresstreeview.h
     utils/widgetdragfilter.h
     utils/animatedsplitter.h

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

     infowidgets/sourceinfowidget.h

     dynamic/widgets/DynamicWidget.h
     dynamic/widgets/DynamicControlWidget.h
     dynamic/widgets/DynamicControlList.h
     dynamic/widgets/ReadOrWriteWidget.h

     widgets/newplaylistwidget.h
     widgets/welcomewidget.h

     xspfloader.h
     transferview.h
     tomahawktrayicon.h
     audiocontrols.h
     settingsdialog.h
     tomahawkwindow.h
)

SET( tomahawkUI ${tomahawkUI}
     tomahawkwindow.ui
     settingsdialog.ui
     proxydialog.ui

     audiocontrols.ui
     sourcetree/sourcetreeitemwidget.ui
     topbar/topbar.ui

     infowidgets/sourceinfowidget.ui
     widgets/newplaylistwidget.ui
     widgets/welcomewidget.ui
)

INCLUDE_DIRECTORIES(
    . 
    ${TOMAHAWK_INC_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}

    audio
    database
    playlist
    network
    sourcetree
    topbar
    utils
    dynamic
    libtomahawk

    ../rtaudio
    ../alsa-playback
    ../qxt/qxtweb-standalone/qxtweb

    /usr/include/taglib
    /usr/local/include/taglib
    /usr/local/include

    ${LIBECHONEST_INCLUDE_DIR}
    ${LIBECHONEST_INCLUDE_DIR}/..
)

SET( OS_SPECIFIC_LINK_LIBRARIES "" )

IF( WIN32 )
  INCLUDE( "CMakeLists.win32.txt" )
ENDIF( WIN32 )
IF( UNIX )
  INCLUDE( "CMakeLists.unix.txt" )
ENDIF( UNIX )

ADD_SUBDIRECTORY( sip/jabber )
ADD_SUBDIRECTORY( sip/zeroconf )

kde4_add_app_icon( tomahawkSources "${CMAKE_CURRENT_SOURCE_DIR}/../data/icons/tomahawk-icon-*.png" )
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
    ${LIBECHONEST_LIBRARY}

    qjson
    tag
)

INCLUDE( "CPack.txt" )
