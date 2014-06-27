TEMPLATE = app
TARGET = tst_certificaterequestbuilder

CONFIG += testcase
QT += testlib network

LIBS    += -Wl,-rpath,../../../src/certificate -L../../../src/certificate -lcertificate
INCLUDEPATH += ../../../src/certificate

SOURCES += tst_certificaterequestbuilder.cpp

