QT       += core gui network

TARGET = georeverse
TEMPLATE = app
win32:LIBS += ../../lib/QTweetLib.lib
INCLUDEPATH += ../../src
CONFIG += mobility
MOBILITY += location

SOURCES += \
    main.cpp \
    georeverse.cpp

HEADERS += \
    georeverse.h

FORMS += \
    georeverse.ui
