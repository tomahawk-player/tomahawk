QJSON_BASE = ..
QJSON_SRCBASE = .

TEMPLATE = lib
QT      -= gui
TARGET   = qjson
DESTDIR  = $$QJSON_BASE/lib
CONFIG += create_prl

VERSION = 0.7.1

windows: {
  DEFINES += QJSON_MAKEDLL
}

QJSON_CPP = $$QJSON_SRCBASE
INCLUDEPATH += $$QJSON_CPP

PRIVATE_HEADERS += \
  json_parser.hh \
  json_scanner.h \
  location.hh \
  parser_p.h  \
  position.hh \
  qjson_debug.h  \
  stack.hh

PUBLIC_HEADERS += \
  parser.h \
  parserrunnable.h \
  qobjecthelper.h \
  serializer.h \
  serializerrunnable.h \
  qjson_export.h

HEADERS += $$PRIVATE_HEADERS $$PUBLIC_HEADERS

SOURCES += \
  json_parser.cc \
  json_scanner.cpp \
  parser.cpp \
  parserrunnable.cpp \
  qobjecthelper.cpp \
  serializer.cpp \
  serializerrunnable.cpp

symbian: {
  DEFINES += QJSON_MAKEDLL
  #export public header to \epocroot\epoc32\include to be able to use them
  headers.files = $$PUBLIC_HEADERS
  headers.path = $$PWD
  for(header, headers.files) {
    {BLD_INF_RULES.prj_exports += "$$header"}
  }

  TARGET.EPOCALLOWDLLDATA = 1
  # uid for the dll
  #TARGET.UID3=
  TARGET.CAPABILITY = ReadDeviceData WriteDeviceData
  
  # do not freeze api-> no libs produced. Comment when freezing!
  # run "abld freeze winscw" to create def files
  symbian:MMP_RULES += "EXPORTUNFROZEN"

  # add dll to the sis
  QjsonDeployment.sources = $${TARGET}.dll
  QjsonDeployment.path = /sys/bin

  DEPLOYMENT += QjsonDeployment
}
