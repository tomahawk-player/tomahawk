TEMPLATE = lib
QT = core network xml
include( _files.qmake )

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
