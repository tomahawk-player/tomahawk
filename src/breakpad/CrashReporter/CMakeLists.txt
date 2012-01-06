PROJECT( CrashReporter )


FIND_PACKAGE( Qt4 REQUIRED )
SET( QT_USE_QTNETWORK TRUE )


SET( crashreporter_SOURCES main.cpp CrashReporter.cpp )
SET( crashreporter_HEADERS CrashReporter.h )
SET( crashreporter_UI CrashReporter.ui )
SET( crashreporter_RC ../../../resources.qrc )

QT4_WRAP_CPP( crashreporter_HEADERS_MOC ${crashreporter_HEADERS} )
QT4_WRAP_UI( crashreporter_UI_HEADERS ${crashreporter_UI} )
QT4_ADD_RESOURCES( crashreporter_RC_RCC ${crashreporter_RC} )

INCLUDE( ${QT_USE_FILE} )
INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR}/src ../../libtomahawk )
ADD_DEFINITIONS( ${QT_DEFINITIONS} )

ADD_EXECUTABLE( tomahawk_crash_reporter WIN32 ${crashreporter_SOURCES} ${crashreporter_HEADERS_MOC} ${crashreporter_UI_HEADERS} ${crashreporter_RC_RCC} )
TARGET_LINK_LIBRARIES( tomahawk_crash_reporter ${QT_LIBRARIES} tomahawklib )

INCLUDE(GNUInstallDirs)
install(TARGETS tomahawk_crash_reporter RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_LIBEXECDIR})
