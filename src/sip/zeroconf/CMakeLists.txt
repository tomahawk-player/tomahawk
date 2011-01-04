project( tomahawk )

include( ${QT_USE_FILE} )
add_definitions( ${QT_DEFINITIONS} )
add_definitions( -DQT_PLUGIN )
add_definitions( -DQT_SHARED )
add_definitions( -DSIPDLLEXPORT_PRO )

set( zeroconfSources
        zeroconf.cpp
)

set( zeroconfHeaders
	zeroconf.h
	tomahawkzeroconf.h
)

include_directories( . ${CMAKE_CURRENT_BINARY_DIR} .. 
   ${QT_INCLUDE_DIR}
)

qt4_wrap_cpp( zeroconfMoc ${zeroconfHeaders} )
add_library( sip_zeroconf SHARED ${zeroconfSources} ${zeroconfMoc} )

IF( WIN32 )
SET( OS_SPECIFIC_LINK_LIBRARIES
	${OS_SPECIFIC_LINK_LIBRARIES}
	"winmm.dll"
        "iphlpapi.a"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../../build/src/libtomahawk/libtomahawklib.dll"
)
ENDIF( WIN32 )

target_link_libraries( sip_zeroconf
	${QT_LIBRARIES}
	${OS_SPECIFIC_LINK_LIBRARIES}
)

IF( APPLE )
  SET( CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS} "-undefined dynamic_lookup" )
ENDIF( APPLE )

install( TARGETS sip_zeroconf DESTINATION lib )
