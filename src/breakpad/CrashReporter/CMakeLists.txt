PROJECT( CrashReporter )
cmake_policy(SET CMP0017 NEW)

FIND_PACKAGE( Qt4 REQUIRED )
SET( QT_USE_QTNETWORK TRUE )


SET( crashreporter_SOURCES main.cpp CrashReporter.cpp )
SET( crashreporter_UI CrashReporter.ui )
SET( crashreporter_RC ../../../resources.qrc )

QT4_WRAP_UI( crashreporter_UI_HEADERS ${crashreporter_UI} )
QT4_ADD_RESOURCES( crashreporter_RC_RCC ${crashreporter_RC} )

INCLUDE( ${QT_USE_FILE} )
INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_BINARY_DIR} ../../libtomahawk )
ADD_DEFINITIONS( ${QT_DEFINITIONS} )

# translations
include( ${CMAKE_SOURCE_DIR}/lang/translations.cmake )
add_tomahawk_translations( ${TOMAHAWK_TRANSLATION_LANGUAGES} )

ADD_EXECUTABLE( tomahawk_crash_reporter WIN32 ${crashreporter_SOURCES} ${crashreporter_HEADERS_MOC} ${crashreporter_UI_HEADERS} ${crashreporter_RC_RCC} ${trans_outfile} )
TARGET_LINK_LIBRARIES( tomahawk_crash_reporter ${QT_LIBRARIES} tomahawklib )
set_target_properties( tomahawk_crash_reporter PROPERTIES AUTOMOC ON)
install(TARGETS tomahawk_crash_reporter RUNTIME DESTINATION ${CMAKE_INSTALL_LIBEXECDIR})
