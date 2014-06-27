
QT += network
TEMPLATE = lib
TARGET = certificate

LIBS += -lgnutls
DEFINES += QT_CERTIFICATE_LIB
CONFIG += debug

# Input
SOURCES += certificatebuilder.cpp \
           certificaterequestbuilder.cpp \
           certificaterequest.cpp \
           keybuilder.cpp \
           utils.cpp \
           randomgenerator.cpp




