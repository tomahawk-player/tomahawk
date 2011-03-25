TEMPLATE = lib
TARGET = lastfm_fingerprint
LIBS += -L$$DESTDIR -llastfm
QT = core xml network sql
include( _files.qmake )
DEFINES += LASTFM_FINGERPRINT_LIB

INSTALLS = target
target.path = /lib

mac:CONFIG( app_bundle ) {
    LIBS += libfftw3f.a libsamplerate.a -L/opt/local/include
    INCLUDEPATH += /opt/local/include:/opt/qt/qt-current/lib/QtSql.framework/Include/
}else{
    INCLUDEPATH += /opt/qt/qt-current/lib/QtSql.framework/Include/
    CONFIG += link_pkgconfig
    PKGCONFIG += samplerate
    win32 {
        CONFIG += link_pkgconfig
        DEFINES += __NO_THREAD_CHECK
        QMAKE_LFLAGS_DEBUG += /NODEFAULTLIB:msvcrt.lib /NODEFAULTLIB:libcmt.lib
	}
    PKGCONFIG += fftw3f

}
