QT = core xml network
LIBS += -L$$DESTDIR -llastfm -llastfm_fingerprint
LIBS += -lvorbisfile -lFLAC -lfaad -lmp4ff -lmad
SOURCES = AacSource.cpp FlacSource.cpp MadSource.cpp VorbisSource.cpp main.cpp

mac {
    INCLUDEPATH += /opt/local/include
    LIBS += -L/opt/local/lib

    DEFINES += MACPORTS_SUCKS
    SOURCES -= AacSource.cpp
    LIBS -= -lmp4ff
}
