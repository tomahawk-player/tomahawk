TEMPLATE = app
TARGET = create_signed_certificate

QT += network

LIBS    += -Wl,-rpath,../../src/certificate -L../../src/certificate -lcertificate
INCLUDEPATH += ../../src/certificate

SOURCES = main.cpp
