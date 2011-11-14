project( tomahawk )

include( ${QT_USE_FILE} )
add_definitions( ${QT_DEFINITIONS} )
add_definitions( -DQT_PLUGIN )
add_definitions( -DQT_SHARED )
add_definitions( -DDLLEXPORT_PRO )

set( twitterAccountSources
    twitteraccount.cpp
    twitterconfigwidget.cpp
)

set( twitterAccountHeaders
    twitteraccount.h
    twitterconfigwidget.h
)

set( twitterAccountUI
    twitterconfigwidget.ui
)

set( tomahawkOAuthTwitterSources
    tomahawkoauthtwitter.cpp
)

set( tomahawkOAuthTwitterHeaders
    tomahawkoauthtwitter.h
)

include_directories( . ${CMAKE_CURRENT_BINARY_DIR} ..
   ${QT_INCLUDE_DIR}
   ${QTWEETLIB_INCLUDE_DIR}
)

qt4_wrap_cpp( tomahawkOAuthTwitterMoc ${tomahawkOAuthTwitterHeaders} )
add_library( tomahawk_oauth_twitter SHARED ${tomahawkOAuthTwitterSources} ${tomahawkOAuthTwitterMoc} )
qt4_wrap_cpp( twitterAccountMoc ${twitterAccountHeaders} )
qt4_wrap_ui( twitterAccountUI_H ${twitterAccountUI} )
qt4_add_resources( RC_SRCS "resources.qrc" )
add_library( tomahawk_account_twitter SHARED ${twitterAccountUI_H} ${twitterAccountSources} ${twitterAccountMoc} ${RC_SRCS} )

IF( WIN32 )
SET( OS_SPECIFIC_LINK_LIBRARIES
        ${OS_SPECIFIC_LINK_LIBRARIES}
        "winmm.dll"
        "iphlpapi.a"
)
ENDIF( WIN32 )

target_link_libraries( tomahawk_oauth_twitter
    ${TOMAHAWK_LIBRARIES}
    ${QTWEETLIB_LIBRARIES}
    ${QT_LIBRARIES}
    ${OS_SPECIFIC_LINK_LIBRARIES}
)

target_link_libraries( tomahawk_account_twitter
    tomahawk_oauth_twitter
    tomahawk_sip_twitter
    ${TOMAHAWK_LIBRARIES}
    ${QTWEETLIB_LIBRARIES}
    ${QT_LIBRARIES}
    ${OS_SPECIFIC_LINK_LIBRARIES}
)

IF( APPLE )
#  SET( CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS} "-undefined dynamic_lookup" )
ENDIF( APPLE )

install( TARGETS tomahawk_oauth_twitter DESTINATION lib${LIB_SUFFIX} )
install( TARGETS tomahawk_account_twitter DESTINATION lib${LIB_SUFFIX} )
