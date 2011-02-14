QT       += core gui network

TARGET = geosearch
TEMPLATE = app
win32:LIBS += ../../lib/QTweetLib.lib
INCLUDEPATH += ../../src
CONFIG += mobility
MOBILITY += location

SOURCES += \
    main.cpp \
    geosearch.cpp

HEADERS += \
    geosearch.h

FORMS += \
    geosearch.ui
