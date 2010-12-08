project( tomahawk )

include( ${QT_USE_FILE} )
add_definitions( ${QT_DEFINITIONS} )
add_definitions( -DQT_PLUGIN )
add_definitions( -DQT_SHARED )

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
add_library( sip_jabber SHARED ${jabberSources} ${jabberMoc} )

target_link_libraries( sip_jabber
	${QT_LIBRARIES}
        ${GLOOX_LIBS}
)

install( TARGETS sip_jabber DESTINATION . )
