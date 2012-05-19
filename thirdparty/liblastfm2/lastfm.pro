TEMPLATE = lib
CONFIG += dll
QT = core network xml

ROOT_DIR = .

BUILD_DIR = _build
DESTDIR = $$ROOT_DIR/_bin

OBJECTS_DIR = $$BUILD_DIR
MOC_DIR = $$BUILD_DIR
UI_DIR = $$BUILD_DIR
RCC_DIR = $$BUILD_DIR
INCLUDEPATH += $$ROOT_DIR/_include

win32:DEFINES += _CRT_SECURE_NO_WARNINGS WIN32_LEAN_AND_MEAN

mac {
    QMAKE_PKGINFO_TYPEINFO = last
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.4
}

*g++* {
    # allow use of 'and', 'or', etc. as symbols
    QMAKE_CXXFLAGS += -fno-operator-names
    QMAKE_CXXFLAGS_RELEASE += -fvisibility-inlines-hidden -fvisibility=hidden
}

# used to determine if we should statically link the fingerprint library
# used by lastfm-desktop and other projects
CONFIG -= app_bundle

win32{
INSTALLS = headers
headers.path = _include/lastfm
headers.files = src/*.h
}
else{
    isEmpty( PREFIX ) {
        PREFIX=/usr/local
    }
INSTALLS = target headers
target.path = $${PREFIX}/lib
headers.path = $${PREFIX}/include/lastfm
headers.files = src/*.h
}

INCLUDEPATH += src

win32{
    DEFINES += LASTFM_LIB _ATL_DLL 
    LIBS += winhttp.lib wbemuuid.lib # ws configuration
}
mac{
    LIBS += -framework SystemConfiguration # ws configuration
    #TODO we should only use these with the carbon version of Qt!
    LIBS += -framework CoreFoundation # various
}

unix:!mac{
    QT += dbus
}

SOURCES += \
        src/ws.cpp \
        src/NetworkConnectionMonitor.cpp \
        src/NetworkAccessManager.cpp \
        src/InternetConnectionMonitor.cpp \
        src/Xspf.cpp \
        src/User.cpp \
        src/Track.cpp \
        src/Tasteometer.cpp \
        src/Tag.cpp \
        src/Playlist.cpp \
        src/Mbid.cpp \
        src/FingerprintId.cpp \
        src/Artist.cpp \
        src/Album.cpp \
        src/ScrobbleCache.cpp \
        src/ScrobblePoint.cpp \
        src/Audioscrobbler.cpp \
        src/RadioTuner.cpp \
        src/RadioStation.cpp \
        src/XmlQuery.cpp \
        src/UrlBuilder.cpp \
        src/misc.cpp \
        src/Chart.cpp \
        src/Auth.cpp \
        src/Library.cpp
	
HEADERS += \
        src/ws.h \
        src/NetworkConnectionMonitor.h \
        src/NetworkAccessManager.h \
        src/InternetConnectionMonitor.h \
        src/Xspf.h \
        src/User.h \
        src/Track.h \
        src/Tasteometer.h \
        src/Tag.h \
        src/Playlist.h \
        src/Mbid.h \
        src/FingerprintId.h \
        src/Artist.h \
        src/Album.h \
        src/AbstractType.h \
        src/ScrobblePoint.h \
        src/ScrobbleCache.h \
        src/Audioscrobbler.h \
        src/RadioTuner.h \
        src/RadioStation.h \
	src/global.h \
        src/XmlQuery.h \
        src/UrlBuilder.h \
        src/misc.h \
        src/Chart.h \
        src/Auth.h \
        src/Library.h
	
win32:SOURCES += src/win/WNetworkConnectionMonitor_win.cpp \
        src/win/WmiSink.cpp \
        src/win/Pac.cpp \
        src/win/NdisEvents.cpp
	
win32:HEADERS += src/win/WNetworkConnectionMonitor.h \
        src/win/WmiSink.h \
        src/win/Pac.h \
        src/win/NdisEvents.h \
        src/win/IeSettings.h \
        src/win/ComSetup.h

mac:SOURCES += 	src/mac/MNetworkConnectionMonitor_mac.cpp

mac:HEADERS += src/mac/ProxyDict.h \
               src/mac/MNetworkConnectionMonitor.h

unix:!mac:SOURCES += src/linux/LNetworkConnectionMonitor_linux.cpp
unix:!mac:HEADERS += src/linux/LNetworkConnectionMonitor.h

!win32:VERSION = 1.0.0


