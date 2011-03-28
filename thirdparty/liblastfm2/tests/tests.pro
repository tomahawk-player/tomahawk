QT = core testlib network xml
LIBS += -llastfm -L$$DESTDIR
SOURCES = main.cpp 
HEADERS = TestTrack.h TestUrlBuilder.h