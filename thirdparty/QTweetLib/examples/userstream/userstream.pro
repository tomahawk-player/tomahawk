QT       += core gui network

TARGET = userstream
TEMPLATE = app
win32:LIBS += ../../lib/QTweetLib.lib
INCLUDEPATH += ../../src


SOURCES += \
    main.cpp \
    userstream.cpp

HEADERS += \
    userstream.h

FORMS += \
    userstream.ui
