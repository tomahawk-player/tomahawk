project( tomahawklib )

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

     network/bufferiodevice.cpp
     network/msgprocessor.cpp
     network/filetransferconnection.cpp
     network/dbsyncconnection.cpp
     network/remotecollection.cpp

     database/fuzzyindex.cpp
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
     database/databasecommand_addsource.cpp
     database/databasecommand_sourceoffline.cpp
     database/databasecommand_collectionstats.cpp
     database/databasecommand_loadplaylistentries.cpp
     database/databasecommand_modifyplaylist.cpp
     database/databasecommand_setplaylistrevision.cpp
     database/databasecommand_loadallplaylists.cpp
     database/databasecommand_createplaylist.cpp
     database/databasecommand_deleteplaylist.cpp
     database/databasecommand_renameplaylist.cpp
     database/databasecommand_loadops.cpp
     database/databasecommand_updatesearchindex.cpp
     database/databasecollection.cpp

	database/database.cpp

        network/servent.cpp
        network/connection.cpp
        network/controlconnection.cpp
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

	database/database.h
     database/fuzzyindex.h
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
     database/databasecommand_addsource.h
     database/databasecommand_sourceoffline.h
     database/databasecommand_collectionstats.h
     database/databasecommand_loadplaylistentries.h
     database/databasecommand_modifyplaylist.h
     database/databasecommand_setplaylistrevision.h
     database/databasecommand_loadallplaylists.h
     database/databasecommand_createplaylist.h
     database/databasecommand_deleteplaylist.h
     database/databasecommand_renameplaylist.h
     database/databasecommand_loadops.h
     database/databasecommand_updatesearchindex.h
     database/databasecollection.h

     network/bufferiodevice.h
     network/msgprocessor.h
     network/remotecollection.h
     network/filetransferconnection.h
     network/dbsyncconnection.h
        network/servent.h
        network/connection.h
        network/controlconnection.h
)

include_directories( . ${CMAKE_CURRENT_BINARY_DIR} .. 
    ${QT_INCLUDE_DIR}
    ../../libportfwd/include
    ../../include
    ../network
    ../../qxt/qxtweb-standalone/qxtweb
)

qt4_wrap_cpp( libMoc ${libHeaders} )
add_library( tomahawklib SHARED ${libSources} ${libMoc} )

IF( WIN32 )
SET( OS_SPECIFIC_LINK_LIBRARIES
	${OS_SPECIFIC_LINK_LIBRARIES}
        "iphlpapi.a"
	"ws2_32.dll"
	"dnsapi.dll"
)
ENDIF( WIN32 )

target_link_libraries( tomahawklib
	${QT_LIBRARIES}
	${OS_SPECIFIC_LINK_LIBRARIES}
	portfwd
	qjson
)

install( TARGETS tomahawklib DESTINATION . )
