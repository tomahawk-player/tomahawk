PROJECT(QTweetLib)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6 FATAL_ERROR)
SET(CMAKE_VERBOSE_MAKEFILE ON)

SET( QT_USE_QTNETWORK TRUE )
INCLUDE( ${QT_USE_FILE} )


add_definitions( ${QT_DEFINITIONS} )
add_definitions( -DQT_SHARED )
add_definitions( -DQTWEETLIB_MAKEDLL )

if(WIN32)
    set(PLATFORM_SPECIFIC_LIBS "ws2_32.dll" "advapi32.dll" )
endif(WIN32)

set(TOMAHAWK_QTWEETLIB_SOURCES
    qtweetlib/src/oauth.cpp
    qtweetlib/src/oauthtwitter.cpp
    qtweetlib/src/qtweetfriendstimeline.cpp
    qtweetlib/src/qtweethometimeline.cpp
    qtweetlib/src/qtweetmentions.cpp
    qtweetlib/src/qtweetnetbase.cpp
    qtweetlib/src/qtweetretweetbyme.cpp
    qtweetlib/src/qtweetretweetsofme.cpp
    qtweetlib/src/qtweetretweettome.cpp
    qtweetlib/src/qtweetstatus.cpp
    qtweetlib/src/qtweetstatusshow.cpp
    qtweetlib/src/qtweetstatusupdate.cpp
    qtweetlib/src/qtweetuser.cpp
    qtweetlib/src/qtweetusertimeline.cpp
    qtweetlib/src/qtweetstatusdestroy.cpp
    qtweetlib/src/qtweetstatusretweet.cpp
    qtweetlib/src/qtweetstatusretweets.cpp
    qtweetlib/src/qtweetusershow.cpp
    qtweetlib/src/qtweetuserlookup.cpp
    qtweetlib/src/qtweetdirectmessages.cpp
    qtweetlib/src/qtweetuserstream.cpp
    qtweetlib/src/qtweetdmstatus.cpp
    qtweetlib/src/qtweetusersearch.cpp
    qtweetlib/src/qtweetuserstatusesfriends.cpp
    qtweetlib/src/qtweetuserstatusesfollowers.cpp
    qtweetlib/src/qtweetlist.cpp
    qtweetlib/src/qtweetlistcreate.cpp
    qtweetlib/src/qtweetlistupdate.cpp
    qtweetlib/src/qtweetlistgetlists.cpp
    qtweetlib/src/qtweetlistshowlist.cpp
    qtweetlib/src/qtweetlistdeletelist.cpp
    qtweetlib/src/qtweetliststatuses.cpp
    qtweetlib/src/qtweetlistmemberships.cpp
    qtweetlib/src/qtweetlistsubscriptions.cpp
    qtweetlib/src/qtweetlistgetmembers.cpp
    qtweetlib/src/qtweetlistaddmember.cpp
    qtweetlib/src/qtweetlistdeletemember.cpp
    qtweetlib/src/qtweetlistsubscribers.cpp
    qtweetlib/src/qtweetlistsubscribe.cpp
    qtweetlib/src/qtweetlistunsubscribe.cpp
    qtweetlib/src/qtweetdirectmessagessent.cpp
    qtweetlib/src/qtweetdirectmessagenew.cpp
    qtweetlib/src/qtweetdirectmessagedestroy.cpp
    qtweetlib/src/qtweetfriendshipcreate.cpp
    qtweetlib/src/qtweetfriendshipdestroy.cpp
    qtweetlib/src/qtweetfriendsid.cpp
    qtweetlib/src/qtweetfollowersid.cpp
    qtweetlib/src/qtweetaccountverifycredentials.cpp
    qtweetlib/src/qtweetaccountratelimitstatus.cpp
    qtweetlib/src/qtweetfavorites.cpp
    qtweetlib/src/qtweetfavoritescreate.cpp
    qtweetlib/src/qtweetfavoritesdestroy.cpp
    qtweetlib/src/qtweetsearch.cpp
    qtweetlib/src/qtweetsearchresult.cpp
    qtweetlib/src/qtweetsearchpageresults.cpp
    qtweetlib/src/qtweetplace.cpp
    qtweetlib/src/qtweetgeoreversegeocode.cpp
    qtweetlib/src/qtweetgeosearch.cpp
    qtweetlib/src/qtweetgeosimilarplaces.cpp
    qtweetlib/src/qtweetgeoplaceid.cpp
    qtweetlib/src/qtweetgeoplacecreate.cpp
    qtweetlib/src/qtweetgeocoord.cpp
    qtweetlib/src/qtweetgeoboundingbox.cpp
    qtweetlib/src/qtweetconvert.cpp
    qtweetlib/src/qtweetentityurl.cpp
    qtweetlib/src/qtweetentityhashtag.cpp
    qtweetlib/src/qtweetentityusermentions.cpp

    tomahawk-custom/tomahawkoauthtwitter.cpp
)

set(TOMAHAWK_QTWEETLIB_HEADERS
    qtweetlib/src/oauth.h
    qtweetlib/src/oauthtwitter.h
    qtweetlib/src/qtweetfriendstimeline.h
    qtweetlib/src/qtweetlib_global.h
    qtweetlib/src/qtweethometimeline.h
    qtweetlib/src/qtweetmentions.h
    qtweetlib/src/qtweetnetbase.h
    qtweetlib/src/qtweetretweetbyme.h
    qtweetlib/src/qtweetretweetsofme.h
    qtweetlib/src/qtweetretweettome.h
    qtweetlib/src/qtweetstatus.h
    qtweetlib/src/qtweetstatusshow.h
    qtweetlib/src/qtweetstatusupdate.h
    qtweetlib/src/qtweetuser.h
    qtweetlib/src/qtweetusertimeline.h
    qtweetlib/src/qtweetstatusdestroy.h
    qtweetlib/src/qtweetstatusretweet.h
    qtweetlib/src/qtweetstatusretweets.h
    qtweetlib/src/qtweetusershow.h
    qtweetlib/src/qtweetuserlookup.h
    qtweetlib/src/qtweetdirectmessages.h
    qtweetlib/src/qtweetuserstream.h
    qtweetlib/src/qtweetdmstatus.h
    qtweetlib/src/qtweetusersearch.h
    qtweetlib/src/qtweetuserstatusesfriends.h
    qtweetlib/src/qtweetuserstatusesfollowers.h
    qtweetlib/src/qtweetlist.h
    qtweetlib/src/qtweetlistcreate.h
    qtweetlib/src/qtweetlistupdate.h
    qtweetlib/src/qtweetlistgetlists.h
    qtweetlib/src/qtweetlistshowlist.h
    qtweetlib/src/qtweetlistdeletelist.h
    qtweetlib/src/qtweetliststatuses.h
    qtweetlib/src/qtweetlistmemberships.h
    qtweetlib/src/qtweetlistsubscriptions.h
    qtweetlib/src/qtweetlistgetmembers.h
    qtweetlib/src/qtweetlistaddmember.h
    qtweetlib/src/qtweetlistdeletemember.h
    qtweetlib/src/qtweetlistsubscribers.h
    qtweetlib/src/qtweetlistsubscribe.h
    qtweetlib/src/qtweetlistunsubscribe.h
    qtweetlib/src/qtweetdirectmessagessent.h
    qtweetlib/src/qtweetdirectmessagenew.h
    qtweetlib/src/qtweetdirectmessagedestroy.h
    qtweetlib/src/qtweetfriendshipcreate.h
    qtweetlib/src/qtweetfriendshipdestroy.h
    qtweetlib/src/qtweetfriendsid.h
    qtweetlib/src/qtweetfollowersid.h
    qtweetlib/src/qtweetaccountverifycredentials.h
    qtweetlib/src/qtweetaccountratelimitstatus.h
    qtweetlib/src/qtweetfavorites.h
    qtweetlib/src/qtweetfavoritescreate.h
    qtweetlib/src/qtweetfavoritesdestroy.h
    qtweetlib/src/qtweetsearch.h
    qtweetlib/src/qtweetsearchresult.h
    qtweetlib/src/qtweetsearchpageresults.h
    qtweetlib/src/qtweetplace.h
    qtweetlib/src/qtweetgeoreversegeocode.h
    qtweetlib/src/qtweetgeosearch.h
    qtweetlib/src/qtweetgeosimilarplaces.h
    qtweetlib/src/qtweetgeoplaceid.h
    qtweetlib/src/qtweetgeoplacecreate.h
    qtweetlib/src/qtweetgeocoord.h
    qtweetlib/src/qtweetgeoboundingbox.h
    qtweetlib/src/qtweetconvert.h
    qtweetlib/src/qtweetentityurl.h
    qtweetlib/src/qtweetentityhashtag.h
    qtweetlib/src/qtweetentityusermentions.h

    tomahawk-custom/tomahawkoauthtwitter.h
)

include_directories(
    .
    ${QT_INCLUDE_DIR}
    ${QT_INCLUDES}
    ${QJSON_INCLUDE_DIR}
    qtweetlib/src
)

qt4_wrap_cpp( TOMAHAWK_QTWEETLIB_MOC ${TOMAHAWK_QTWEETLIB_HEADERS} )

ADD_LIBRARY(tomahawk_qtweetlib SHARED ${TOMAHAWK_QTWEETLIB_SOURCES} ${TOMAHAWK_QTWEETLIB_MOC})

if(APPLE)
  SET(QJSON_FLAGS ${QJSON_LIBRARIES})
else(APPLE)
  SET(QJSON_FLAGS ${QJSON_LDFLAGS})
endif(APPLE)
 
target_link_libraries(tomahawk_qtweetlib
    ${QT_LIBRARIES}
    ${QJSON_FLAGS}
)   

INCLUDE( ${CMAKE_CURRENT_SOURCE_DIR}/twitter-api-keys )

INSTALL(TARGETS tomahawk_qtweetlib DESTINATION lib${LIB_SUFFIX})


