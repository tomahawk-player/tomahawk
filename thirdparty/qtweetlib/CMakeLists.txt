PROJECT(QTweetLib)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6 FATAL_ERROR)
SET(CMAKE_VERBOSE_MAKEFILE ON)

SET( QT_USE_QTNETWORK TRUE )
INCLUDE( ${QT_USE_FILE} )


add_definitions( ${QT_DEFINITIONS} )
add_definitions( -DQT_SHARED )

if(WIN32)
    set(PLATFORM_SPECIFIC_LIBS "ws2_32.dll" "advapi32.dll" )
endif(WIN32)

set(TOMAHAWK_QTWEETLIB_SOURCES
    src/oauth.cpp
    src/oauthtwitter.cpp
    src/qtweetfriendstimeline.cpp
    src/qtweethometimeline.cpp
    src/qtweetmentions.cpp
    src/qtweetnetbase.cpp
    src/qtweetretweetbyme.cpp
    src/qtweetretweetsofme.cpp
    src/qtweetretweettome.cpp
    src/qtweetstatus.cpp
    src/qtweetstatusshow.cpp
    src/qtweetstatusupdate.cpp
    src/qtweetuser.cpp
    src/qtweetusertimeline.cpp
    src/qtweetstatusdestroy.cpp
    src/qtweetstatusretweet.cpp
    src/qtweetstatusretweets.cpp
    src/qtweetusershow.cpp
    src/qtweetuserlookup.cpp
    src/qtweetdirectmessages.cpp
    src/qtweetuserstream.cpp
    src/qtweetdmstatus.cpp
    src/qtweetusersearch.cpp
    src/qtweetuserstatusesfriends.cpp
    src/qtweetuserstatusesfollowers.cpp
    src/qtweetlist.cpp
    src/qtweetlistcreate.cpp
    src/qtweetlistupdate.cpp
    src/qtweetlistgetlists.cpp
    src/qtweetlistshowlist.cpp
    src/qtweetlistdeletelist.cpp
    src/qtweetliststatuses.cpp
    src/qtweetlistmemberships.cpp
    src/qtweetlistsubscriptions.cpp
    src/qtweetlistgetmembers.cpp
    src/qtweetlistaddmember.cpp
    src/qtweetlistdeletemember.cpp
    src/qtweetlistsubscribers.cpp
    src/qtweetlistsubscribe.cpp
    src/qtweetlistunsubscribe.cpp
    src/qtweetdirectmessagessent.cpp
    src/qtweetdirectmessagenew.cpp
    src/qtweetdirectmessagedestroy.cpp
    src/qtweetfriendshipcreate.cpp
    src/qtweetfriendshipdestroy.cpp
    src/qtweetfriendsid.cpp
    src/qtweetfollowersid.cpp
    src/qtweetaccountverifycredentials.cpp
    src/qtweetaccountratelimitstatus.cpp
    src/qtweetfavorites.cpp
    src/qtweetfavoritescreate.cpp
    src/qtweetfavoritesdestroy.cpp
    src/qtweetsearch.cpp
    src/qtweetsearchresult.cpp
    src/qtweetsearchpageresults.cpp
    src/qtweetplace.cpp
    src/qtweetgeoreversegeocode.cpp
    src/qtweetgeosearch.cpp
    src/qtweetgeosimilarplaces.cpp
    src/qtweetgeoplaceid.cpp
    src/qtweetgeoplacecreate.cpp
    src/qtweetgeocoord.cpp
    src/qtweetgeoboundingbox.cpp
    src/qtweetconvert.cpp
    src/qtweetentityurl.cpp
    src/qtweetentityhashtag.cpp
    src/qtweetentityusermentions.cpp
)

set(TOMAHAWK_QTWEETLIB_HEADERS
    src/oauth.h
    src/oauthtwitter.h
    src/qtweetfriendstimeline.h
    src/qtweetlib_global.h
    src/qtweethometimeline.h
    src/qtweetmentions.h
    src/qtweetnetbase.h
    src/qtweetretweetbyme.h
    src/qtweetretweetsofme.h
    src/qtweetretweettome.h
    src/qtweetstatus.h
    src/qtweetstatusshow.h
    src/qtweetstatusupdate.h
    src/qtweetuser.h
    src/qtweetusertimeline.h
    src/qtweetstatusdestroy.h
    src/qtweetstatusretweet.h
    src/qtweetstatusretweets.h
    src/qtweetusershow.h
    src/qtweetuserlookup.h
    src/qtweetdirectmessages.h
    src/qtweetuserstream.h
    src/qtweetdmstatus.h
    src/qtweetusersearch.h
    src/qtweetuserstatusesfriends.h
    src/qtweetuserstatusesfollowers.h
    src/qtweetlist.h
    src/qtweetlistcreate.h
    src/qtweetlistupdate.h
    src/qtweetlistgetlists.h
    src/qtweetlistshowlist.h
    src/qtweetlistdeletelist.h
    src/qtweetliststatuses.h
    src/qtweetlistmemberships.h
    src/qtweetlistsubscriptions.h
    src/qtweetlistgetmembers.h
    src/qtweetlistaddmember.h
    src/qtweetlistdeletemember.h
    src/qtweetlistsubscribers.h
    src/qtweetlistsubscribe.h
    src/qtweetlistunsubscribe.h
    src/qtweetdirectmessagessent.h
    src/qtweetdirectmessagenew.h
    src/qtweetdirectmessagedestroy.h
    src/qtweetfriendshipcreate.h
    src/qtweetfriendshipdestroy.h
    src/qtweetfriendsid.h
    src/qtweetfollowersid.h
    src/qtweetaccountverifycredentials.h
    src/qtweetaccountratelimitstatus.h
    src/qtweetfavorites.h
    src/qtweetfavoritescreate.h
    src/qtweetfavoritesdestroy.h
    src/qtweetsearch.h
    src/qtweetsearchresult.h
    src/qtweetsearchpageresults.h
    src/qtweetplace.h
    src/qtweetgeoreversegeocode.h
    src/qtweetgeosearch.h
    src/qtweetgeosimilarplaces.h
    src/qtweetgeoplaceid.h
    src/qtweetgeoplacecreate.h
    src/qtweetgeocoord.h
    src/qtweetgeoboundingbox.h
    src/qtweetconvert.h
    src/qtweetentityurl.h
    src/qtweetentityhashtag.h
    src/qtweetentityusermentions.h
)

include_directories(
    .
    ${QT_INCLUDE_DIR}
    ${QT_INCLUDES}
    src
)

qt4_wrap_cpp( TOMAHAWK_QTWEETLIB_MOC ${TOMAHAWK_QTWEETLIB_HEADERS} )

ADD_LIBRARY(tomahawk_qtweetlib SHARED ${TOMAHAWK_QTWEETLIB_SOURCES} ${TOMAHAWK_QTWEETLIB_MOC})

target_link_libraries(tomahawk_qtweetlib
    ${QT_LIBRARIES}
    qjson
)

SET_TARGET_PROPERTIES( tomahawk_qtweetlib PROPERTIES DEFINE_SYMBOL  MAKE_QTWEETLIB_LIB )

IF( EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/WARNING-twitter-api-keys )
  INCLUDE( ${CMAKE_CURRENT_SOURCE_DIR}/WARNING-twitter-api-keys )
ENDIF()

INSTALL(TARGETS tomahawk_qtweetlib DESTINATION lib)


