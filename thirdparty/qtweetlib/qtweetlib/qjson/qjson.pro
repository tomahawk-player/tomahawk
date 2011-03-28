TEMPLATE = lib
QT -= gui
TARGET = qjson
DESTDIR = ../lib
CONFIG += create_prl

windows: {
	DEFINES += QJSON_MAKEDLL
}

HEADERS += \
	stack.hh \
    serializerrunnable.h \
    serializer.h \
    qobjecthelper.h \
    qjson_export.h \
    qjson_debug.h \
    position.hh \
    parserrunnable.h \
    parser_p.h \
    parser.h \
    location.hh \
    json_scanner.h \
    json_parser.hh \
	
SOURCES += \
	serializerrunnable.cpp \
    serializer.cpp \
    qobjecthelper.cpp \
    parserrunnable.cpp \
    parser.cpp \
    json_scanner.cpp \
    json_parser.cc \
	
