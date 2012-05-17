project( tomahawk )

include( ${QT_USE_FILE} )
add_definitions( ${QT_DEFINITIONS} )
add_definitions( -DQT_PLUGIN )
add_definitions( -DQT_SHARED )
add_definitions( -DINFOPLUGINDLLEXPORT_PRO )

set( hypemInfoPluginSources
    HypemPlugin.cpp
)

set( hypemInfoPluginHeaders
    HypemPlugin.h
)

include_directories(
   ${QT_INCLUDE_DIR}
)

qt4_wrap_cpp( hypemInfoPluginMoc ${hypemInfoPluginHeaders} )
add_library( tomahawk_infoplugin_hypem SHARED ${hypemInfoPluginSources} ${hypemInfoPluginMoc} ${RC_SRCS} )

IF( WIN32 )
SET( OS_SPECIFIC_LINK_LIBRARIES
        ${OS_SPECIFIC_LINK_LIBRARIES}
        "winmm.dll"
        "iphlpapi.a"
)
ENDIF( WIN32 )

target_link_libraries( tomahawk_infoplugin_hypem
    ${TOMAHAWK_LIBRARIES}
    ${QT_LIBRARIES}
    ${OS_SPECIFIC_LINK_LIBRARIES}
)

IF( APPLE )
#  SET( CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS} "-undefined dynamic_lookup" )
ENDIF( APPLE )

install( TARGETS tomahawk_infoplugin_hypem DESTINATION ${CMAKE_INSTALL_LIBDIR} )

