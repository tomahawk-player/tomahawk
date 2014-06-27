TEMPLATE = app
TARGET = tst_keybuilder

CONFIG += testcase
QT += testlib network

LIBS    += -Wl,-rpath,../../../src/certificate -L../../../src/certificate -lcertificate
INCLUDEPATH += ../../../src/certificate

SOURCES += tst_keybuilder.cpp

