QT       += core gui network declarative

TARGET = followers
TEMPLATE = app
win32:LIBS += ../../lib/QTweetLib.lib
INCLUDEPATH += ../../src

SOURCES += \
    main.cpp \
    followers.cpp \
    followerslistmodel.cpp

HEADERS += \
    followers.h \
    followerslistmodel.h

FORMS += \
    followers.ui

OTHER_FILES += \
    FollowerDelegate.qml \
    FollowersList.qml

RESOURCES += \
    followers.qrc
