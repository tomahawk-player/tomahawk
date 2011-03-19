project( tomahawk )

include( ${QT_USE_FILE} )
add_definitions( ${QT_DEFINITIONS} )
add_definitions( -DQT_PLUGIN )
add_definitions( -DQT_SHARED )
add_definitions( -DSIPDLLEXPORT_PRO )

set( jabberSources
        jabber.cpp
	jabber_p.cpp
)

set( jabberHeaders
	jabber.h
        jabber_p.h
)

include_directories( . ${CMAKE_CURRENT_BINARY_DIR} .. 
   ${QT_INCLUDE_DIR}
)

qt4_wrap_cpp( jabberMoc ${jabberHeaders} )
add_library( tomahawk_sipjabber SHARED ${jabberSources} ${jabberMoc} )

IF( WIN32 )
SET( OS_SPECIFIC_LINK_LIBRARIES
	${OS_SPECIFIC_LINK_LIBRARIES}
	"secur32.dll"
	"crypt32.dll"
        ${CMAKE_BINARY_DIR}/src/libtomahawk/libtomahawklib.dll
)
ENDIF( WIN32 )

target_link_libraries( tomahawk_sipjabber
	${QT_LIBRARIES}
        ${GLOOX_LIBRARIES}
	${OS_SPECIFIC_LINK_LIBRARIES}
	tomahawklib
)

IF( APPLE )
#  SET( CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS} "-undefined dynamic_lookup" )
ENDIF( APPLE )

install( TARGETS tomahawk_sipjabber DESTINATION lib )
