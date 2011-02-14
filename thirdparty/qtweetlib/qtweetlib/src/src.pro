QT       += network
QT       += gui

TEMPLATE = lib
TARGET = QTweetLib
DESTDIR = ../lib
win32:LIBS += ../lib/qjson.lib
unix:LIBS += -L../lib -lqjson
CONFIG += create_prl
INCLUDEPATH += ..

windows: {
	DEFINES += QTWEETLIB_MAKEDLL
}

HEADERS += \
    oauth.h \
    oauthtwitter.h \
    qtweetfriendstimeline.h \
    qtweetlib_global.h \
    qtweethometimeline.h \
    qtweetmentions.h \
    qtweetnetbase.h \
    qtweetretweetbyme.h \
    qtweetretweetsofme.h \
    qtweetretweettome.h \
    qtweetstatus.h \
    qtweetstatusshow.h \
    qtweetstatusupdate.h \
    qtweetuser.h \
    qtweetusertimeline.h \
    qtweetstatusdestroy.h \
    qtweetstatusretweet.h \
    qtweetstatusretweets.h \
    qtweetusershow.h \
    qtweetuserlookup.h \
    qtweetdirectmessages.h \
    qtweetuserstream.h \
    qtweetdmstatus.h \
    qtweetusersearch.h \
    qtweetuserstatusesfriends.h \
    qtweetuserstatusesfollowers.h \
    qtweetlist.h \
    qtweetlistcreate.h \
    qtweetlistupdate.h \
    qtweetlistgetlists.h \
    qtweetlistshowlist.h \
    qtweetlistdeletelist.h \
    qtweetliststatuses.h \
    qtweetlistmemberships.h \
    qtweetlistsubscriptions.h \
    qtweetlistgetmembers.h \
    qtweetlistaddmember.h \
    qtweetlistdeletemember.h \
    qtweetlistsubscribers.h \
    qtweetlistsubscribe.h \
    qtweetlistunsubscribe.h \
    qtweetdirectmessagessent.h \
    qtweetdirectmessagenew.h \
    qtweetdirectmessagedestroy.h \
    qtweetfriendshipcreate.h \
    qtweetfriendshipdestroy.h \
    qtweetfriendsid.h \
    qtweetfollowersid.h \
    qtweetaccountverifycredentials.h \
    qtweetaccountratelimitstatus.h \
    qtweetfavorites.h \
    qtweetfavoritescreate.h \
    qtweetfavoritesdestroy.h \
    qtweetsearch.h \
    qtweetsearchresult.h \
    qtweetsearchpageresults.h \
    qtweetplace.h \
    qtweetgeoreversegeocode.h \
    qtweetgeosearch.h \
    qtweetgeosimilarplaces.h \
    qtweetgeoplaceid.h \
    qtweetgeoplacecreate.h \
    qtweetgeocoord.h \
    qtweetgeoboundingbox.h \
    qtweetconvert.h \
    qtweetentityurl.h \
    qtweetentityhashtag.h \
    qtweetentityusermentions.h

SOURCES += \
    oauth.cpp \
    oauthtwitter.cpp \
    qtweetfriendstimeline.cpp \
    qtweethometimeline.cpp \
    qtweetmentions.cpp \
    qtweetnetbase.cpp \
    qtweetretweetbyme.cpp \
    qtweetretweetsofme.cpp \
    qtweetretweettome.cpp \
    qtweetstatus.cpp \
    qtweetstatusshow.cpp \
    qtweetstatusupdate.cpp \
    qtweetuser.cpp \
    qtweetusertimeline.cpp \
    qtweetstatusdestroy.cpp \
    qtweetstatusretweet.cpp \
    qtweetstatusretweets.cpp \
    qtweetusershow.cpp \
    qtweetuserlookup.cpp \
    qtweetdirectmessages.cpp \
    qtweetuserstream.cpp \
    qtweetdmstatus.cpp \
    qtweetusersearch.cpp \
    qtweetuserstatusesfriends.cpp \
    qtweetuserstatusesfollowers.cpp \
    qtweetlist.cpp \
    qtweetlistcreate.cpp \
    qtweetlistupdate.cpp \
    qtweetlistgetlists.cpp \
    qtweetlistshowlist.cpp \
    qtweetlistdeletelist.cpp \
    qtweetliststatuses.cpp \
    qtweetlistmemberships.cpp \
    qtweetlistsubscriptions.cpp \
    qtweetlistgetmembers.cpp \
    qtweetlistaddmember.cpp \
    qtweetlistdeletemember.cpp \
    qtweetlistsubscribers.cpp \
    qtweetlistsubscribe.cpp \
    qtweetlistunsubscribe.cpp \
    qtweetdirectmessagessent.cpp \
    qtweetdirectmessagenew.cpp \
    qtweetdirectmessagedestroy.cpp \
    qtweetfriendshipcreate.cpp \
    qtweetfriendshipdestroy.cpp \
    qtweetfriendsid.cpp \
    qtweetfollowersid.cpp \
    qtweetaccountverifycredentials.cpp \
    qtweetaccountratelimitstatus.cpp \
    qtweetfavorites.cpp \
    qtweetfavoritescreate.cpp \
    qtweetfavoritesdestroy.cpp \
    qtweetsearch.cpp \
    qtweetsearchresult.cpp \
    qtweetsearchpageresults.cpp \
    qtweetplace.cpp \
    qtweetgeoreversegeocode.cpp \
    qtweetgeosearch.cpp \
    qtweetgeosimilarplaces.cpp \
    qtweetgeoplaceid.cpp \
    qtweetgeoplacecreate.cpp \
    qtweetgeocoord.cpp \
    qtweetgeoboundingbox.cpp \
    qtweetconvert.cpp \
    qtweetentityurl.cpp \
    qtweetentityhashtag.cpp \
    qtweetentityusermentions.cpp
