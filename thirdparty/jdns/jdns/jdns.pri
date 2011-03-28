# qmake project include file

QT *= network

windows:{
	LIBS += -lWs2_32 -lAdvapi32
}
unix:{
	#QMAKE_CFLAGS += -pedantic
}

HEADERS += \
	$$PWD/jdns_packet.h \
	$$PWD/jdns_mdnsd.h \
	$$PWD/jdns_p.h \
	$$PWD/jdns.h \
	$$PWD/qjdns_sock.h \
	$$PWD/qjdns.h

SOURCES += \
	$$PWD/jdns_util.c \
	$$PWD/jdns_packet.c \
	$$PWD/jdns_mdnsd.c \
	$$PWD/jdns_sys.c \
	$$PWD/jdns.c \
	$$PWD/qjdns_sock.cpp \
	$$PWD/qjdns.cpp
