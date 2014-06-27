TEMPLATE = app
TARGET = tst_certificaterequest

CONFIG += testcase
QT += testlib network

LIBS    += -Wl,-rpath,../../../src/certificate -L../../../src/certificate -lcertificate
INCLUDEPATH += ../../../src/certificate

SOURCES += tst_certificaterequest.cpp

