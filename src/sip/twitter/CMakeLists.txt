project( tomahawk )

include( ${QT_USE_FILE} )
add_definitions( ${QT_DEFINITIONS} )
add_definitions( -DQT_PLUGIN )
add_definitions( -DQT_SHARED )
add_definitions( -DSIPDLLEXPORT_PRO )

set( twitterSources
    twitter.cpp
    twitterconfigwidget.cpp
    tomahawkoauthtwitter.cpp
)

set( twitterHeaders
    twitter.h
    twitterconfigwidget.h
    tomahawkoauthtwitter.h
)

set( twitterUI
    twitterconfigwidget.ui
)

include_directories( . ${CMAKE_CURRENT_BINARY_DIR} ..
   ${QT_INCLUDE_DIR}
   ${QTWEETLIB_INCLUDE_DIR}
)

qt4_add_resources( RC_SRCS "resources.qrc" )
qt4_wrap_cpp( twitterMoc ${twitterHeaders} )
qt4_wrap_ui( twitterUI_H ${twitterUI} )
add_library( tomahawk_siptwitter SHARED ${twitterUI_H} ${twitterSources} ${twitterMoc} ${RC_SRCS} )

IF( WIN32 )
SET( OS_SPECIFIC_LINK_LIBRARIES
        ${OS_SPECIFIC_LINK_LIBRARIES}
        "winmm.dll"
        "iphlpapi.a"
)
ENDIF( WIN32 )

target_link_libraries( tomahawk_siptwitter
    ${TOMAHAWK_LIBRARIES}
    ${QTWEETLIB_LIBRARIES}
    ${QT_LIBRARIES}
    ${OS_SPECIFIC_LINK_LIBRARIES}
)

IF( APPLE )
#  SET( CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS} "-undefined dynamic_lookup" )
ENDIF( APPLE )

install( TARGETS tomahawk_siptwitter DESTINATION lib${LIB_SUFFIX} )
