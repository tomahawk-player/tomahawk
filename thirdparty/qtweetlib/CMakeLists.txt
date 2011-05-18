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
    QTweetLib/src/oauth.cpp
    QTweetLib/src/oauthtwitter.cpp
    QTweetLib/src/qtweetfriendstimeline.cpp
    QTweetLib/src/qtweethometimeline.cpp
    QTweetLib/src/qtweetmentions.cpp
    QTweetLib/src/qtweetnetbase.cpp
    QTweetLib/src/qtweetretweetbyme.cpp
    QTweetLib/src/qtweetretweetsofme.cpp
    QTweetLib/src/qtweetretweettome.cpp
    QTweetLib/src/qtweetstatus.cpp
    QTweetLib/src/qtweetstatusshow.cpp
    QTweetLib/src/qtweetstatusupdate.cpp
    QTweetLib/src/qtweetuser.cpp
    QTweetLib/src/qtweetusertimeline.cpp
    QTweetLib/src/qtweetstatusdestroy.cpp
    QTweetLib/src/qtweetstatusretweet.cpp
    QTweetLib/src/qtweetstatusretweets.cpp
    QTweetLib/src/qtweetusershow.cpp
    QTweetLib/src/qtweetuserlookup.cpp
    QTweetLib/src/qtweetdirectmessages.cpp
    QTweetLib/src/qtweetuserstream.cpp
    QTweetLib/src/qtweetdmstatus.cpp
    QTweetLib/src/qtweetusersearch.cpp
    QTweetLib/src/qtweetuserstatusesfriends.cpp
    QTweetLib/src/qtweetuserstatusesfollowers.cpp
    QTweetLib/src/qtweetlist.cpp
    QTweetLib/src/qtweetlistcreate.cpp
    QTweetLib/src/qtweetlistupdate.cpp
    QTweetLib/src/qtweetlistgetlists.cpp
    QTweetLib/src/qtweetlistshowlist.cpp
    QTweetLib/src/qtweetlistdeletelist.cpp
    QTweetLib/src/qtweetliststatuses.cpp
    QTweetLib/src/qtweetlistmemberships.cpp
    QTweetLib/src/qtweetlistsubscriptions.cpp
    QTweetLib/src/qtweetlistgetmembers.cpp
    QTweetLib/src/qtweetlistaddmember.cpp
    QTweetLib/src/qtweetlistdeletemember.cpp
    QTweetLib/src/qtweetlistsubscribers.cpp
    QTweetLib/src/qtweetlistsubscribe.cpp
    QTweetLib/src/qtweetlistunsubscribe.cpp
    QTweetLib/src/qtweetdirectmessagessent.cpp
    QTweetLib/src/qtweetdirectmessagenew.cpp
    QTweetLib/src/qtweetdirectmessagedestroy.cpp
    QTweetLib/src/qtweetfriendshipcreate.cpp
    QTweetLib/src/qtweetfriendshipdestroy.cpp
    QTweetLib/src/qtweetfriendsid.cpp
    QTweetLib/src/qtweetfollowersid.cpp
    QTweetLib/src/qtweetaccountverifycredentials.cpp
    QTweetLib/src/qtweetaccountratelimitstatus.cpp
    QTweetLib/src/qtweetfavorites.cpp
    QTweetLib/src/qtweetfavoritescreate.cpp
    QTweetLib/src/qtweetfavoritesdestroy.cpp
    QTweetLib/src/qtweetsearch.cpp
    QTweetLib/src/qtweetsearchresult.cpp
    QTweetLib/src/qtweetsearchpageresults.cpp
    QTweetLib/src/qtweetplace.cpp
    QTweetLib/src/qtweetgeoreversegeocode.cpp
    QTweetLib/src/qtweetgeosearch.cpp
    QTweetLib/src/qtweetgeosimilarplaces.cpp
    QTweetLib/src/qtweetgeoplaceid.cpp
    QTweetLib/src/qtweetgeoplacecreate.cpp
    QTweetLib/src/qtweetgeocoord.cpp
    QTweetLib/src/qtweetgeoboundingbox.cpp
    QTweetLib/src/qtweetconvert.cpp
    QTweetLib/src/qtweetentityurl.cpp
    QTweetLib/src/qtweetentityhashtag.cpp
    QTweetLib/src/qtweetentityusermentions.cpp
    QTweetLib/src/qtweetblocksblocking.cpp
    QTweetLib/src/qtweetblocksblockingids.cpp
    QTweetLib/src/qtweetblockscreate.cpp
    QTweetLib/src/qtweetblocksdestroy.cpp
    QTweetLib/src/qtweetblocksexists.cpp

    tomahawk-custom/tomahawkoauthtwitter.cpp
)

set(TOMAHAWK_QTWEETLIB_HEADERS
    QTweetLib/src/oauth.h
    QTweetLib/src/oauthtwitter.h
    QTweetLib/src/qtweetfriendstimeline.h
    QTweetLib/src/qtweetlib_global.h
    QTweetLib/src/qtweethometimeline.h
    QTweetLib/src/qtweetmentions.h
    QTweetLib/src/qtweetnetbase.h
    QTweetLib/src/qtweetretweetbyme.h
    QTweetLib/src/qtweetretweetsofme.h
    QTweetLib/src/qtweetretweettome.h
    QTweetLib/src/qtweetstatus.h
    QTweetLib/src/qtweetstatusshow.h
    QTweetLib/src/qtweetstatusupdate.h
    QTweetLib/src/qtweetuser.h
    QTweetLib/src/qtweetusertimeline.h
    QTweetLib/src/qtweetstatusdestroy.h
    QTweetLib/src/qtweetstatusretweet.h
    QTweetLib/src/qtweetstatusretweets.h
    QTweetLib/src/qtweetusershow.h
    QTweetLib/src/qtweetuserlookup.h
    QTweetLib/src/qtweetdirectmessages.h
    QTweetLib/src/qtweetuserstream.h
    QTweetLib/src/qtweetdmstatus.h
    QTweetLib/src/qtweetusersearch.h
    QTweetLib/src/qtweetuserstatusesfriends.h
    QTweetLib/src/qtweetuserstatusesfollowers.h
    QTweetLib/src/qtweetlist.h
    QTweetLib/src/qtweetlistcreate.h
    QTweetLib/src/qtweetlistupdate.h
    QTweetLib/src/qtweetlistgetlists.h
    QTweetLib/src/qtweetlistshowlist.h
    QTweetLib/src/qtweetlistdeletelist.h
    QTweetLib/src/qtweetliststatuses.h
    QTweetLib/src/qtweetlistmemberships.h
    QTweetLib/src/qtweetlistsubscriptions.h
    QTweetLib/src/qtweetlistgetmembers.h
    QTweetLib/src/qtweetlistaddmember.h
    QTweetLib/src/qtweetlistdeletemember.h
    QTweetLib/src/qtweetlistsubscribers.h
    QTweetLib/src/qtweetlistsubscribe.h
    QTweetLib/src/qtweetlistunsubscribe.h
    QTweetLib/src/qtweetdirectmessagessent.h
    QTweetLib/src/qtweetdirectmessagenew.h
    QTweetLib/src/qtweetdirectmessagedestroy.h
    QTweetLib/src/qtweetfriendshipcreate.h
    QTweetLib/src/qtweetfriendshipdestroy.h
    QTweetLib/src/qtweetfriendsid.h
    QTweetLib/src/qtweetfollowersid.h
    QTweetLib/src/qtweetaccountverifycredentials.h
    QTweetLib/src/qtweetaccountratelimitstatus.h
    QTweetLib/src/qtweetfavorites.h
    QTweetLib/src/qtweetfavoritescreate.h
    QTweetLib/src/qtweetfavoritesdestroy.h
    QTweetLib/src/qtweetsearch.h
    QTweetLib/src/qtweetsearchresult.h
    QTweetLib/src/qtweetsearchpageresults.h
    QTweetLib/src/qtweetplace.h
    QTweetLib/src/qtweetgeoreversegeocode.h
    QTweetLib/src/qtweetgeosearch.h
    QTweetLib/src/qtweetgeosimilarplaces.h
    QTweetLib/src/qtweetgeoplaceid.h
    QTweetLib/src/qtweetgeoplacecreate.h
    QTweetLib/src/qtweetgeocoord.h
    QTweetLib/src/qtweetgeoboundingbox.h
    QTweetLib/src/qtweetconvert.h
    QTweetLib/src/qtweetentityurl.h
    QTweetLib/src/qtweetentityhashtag.h
    QTweetLib/src/qtweetentityusermentions.h
    QTweetLib/src/qtweetblocksblocking.h
    QTweetLib/src/qtweetblocksblockingids.h
    QTweetLib/src/qtweetblockscreate.h
    QTweetLib/src/qtweetblocksdestroy.h
    QTweetLib/src/qtweetblocksexists.h

    tomahawk-custom/tomahawkoauthtwitter.h
)

include_directories(
    .
    ${QT_INCLUDE_DIR}
    ${QT_INCLUDES}
    ${QJSON_INCLUDE_DIR}
    QTweetLib/src
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
    ${QJSON_LIBRARIES}
)   

INSTALL(TARGETS tomahawk_qtweetlib DESTINATION lib${LIB_SUFFIX})


