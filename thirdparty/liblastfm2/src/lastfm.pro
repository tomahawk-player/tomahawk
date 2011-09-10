TEMPLATE = lib
QT = core network xml

INSTALLS = target
target.path = /lib

win32{
    DEFINES += LASTFM_LIB _ATL_DLL 
    LIBS += winhttp.lib wbemuuid.lib # ws configuration
}
mac{
    LIBS += -framework SystemConfiguration # ws configuration
    #TODO we should only use these with the carbon version of Qt!
    LIBS += -framework Carbon -framework CoreFoundation # various
}

linux*{
    QT += dbus
}

SOURCES += \
	ws/ws.cpp \
	ws/NetworkConnectionMonitor.cpp \
	ws/NetworkAccessManager.cpp \
	ws/InternetConnectionMonitor.cpp \
	types/Xspf.cpp \
	types/User.cpp \
	types/Track.cpp \
	types/Tasteometer.cpp \
	types/Tag.cpp \
	types/Playlist.cpp \
	types/Mbid.cpp \
	types/FingerprintId.cpp \
	types/Artist.cpp \
	types/Album.cpp \
	scrobble/ScrobbleCache.cpp \
	scrobble/Audioscrobbler.cpp \
	radio/RadioTuner.cpp \
	radio/RadioStation.cpp \
	core/XmlQuery.cpp \
	core/UrlBuilder.cpp \
	core/misc.cpp
	
HEADERS += \
	ws/ws.h \
	ws/NetworkConnectionMonitor.h \
	ws/NetworkAccessManager.h \ 
	ws/InternetConnectionMonitor.h \
	types/Xspf.h \
	types/User.h \
	types/Track.h \
	types/Tasteometer.h \
	types/Tag.h \
	types/Playlist.h \
	types/Mbid.h \
	types/FingerprintId.h \
	types/Artist.h \
	types/Album.h \
	types/AbstractType.h \
	scrobble/ScrobblePoint.h \
	scrobble/ScrobbleCache.h \
	scrobble/Audioscrobbler.h \
	radio/RadioTuner.h \
	radio/RadioStation.h \
	global.h \
	core/XmlQuery.h \
	core/UrlBuilder.h \
	core/misc.h
	
win32:SOURCES += ws/win/WNetworkConnectionMonitor_win.cpp \
	ws/win/WmiSink.cpp \
	ws/win/Pac.cpp \
	ws/win/NdisEvents.cpp
	
win32:HEADERS += ws/win/WNetworkConnectionMonitor.h \
	ws/win/WmiSink.h \
	ws/win/Pac.h \
	ws/win/NdisEvents.h \
	ws/win/IeSettings.h \
	ws/win/ComSetup.h

mac:SOURCES += 	ws/mac/MNetworkConnectionMonitor_mac.cpp

mac:HEADERS += ws/mac/ProxyDict.h \
               ws/mac/MNetworkConnectionMonitor.h

!win32:VERSION = 0.4.0
