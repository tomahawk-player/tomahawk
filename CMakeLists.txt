PROJECT( tomahawk )
CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )

SET( CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${CMAKE_SOURCE_DIR}/CMakeModules" )
SET( THIRDPARTY_DIR  ${CMAKE_SOURCE_DIR}/thirdparty )

# Check if we need qtgui:
IF( "${gui}" STREQUAL "no" )
    ADD_DEFINITIONS( -DENABLE_HEADLESS )
    MESSAGE( STATUS "Building in HEADLESS mode ***" )
	FIND_PACKAGE( Qt4 4.6.0 COMPONENTS QtCore QtXml QtNetwork REQUIRED )
ELSE()
    MESSAGE( STATUS "Building full GUI version ***" )
	FIND_PACKAGE( Qt4 4.6.0 COMPONENTS QtGui QtCore QtXml QtNetwork REQUIRED )
ENDIF()

FIND_PACKAGE( Taglib 1.6.0 REQUIRED )
include( CheckTagLibFileName )
check_taglib_filename( COMPLEX_TAGLIB_FILENAME )

# required deps
FIND_PACKAGE( LibLastFm 0.3.3 REQUIRED )
FIND_PACKAGE( LibEchonest 1.1.1 REQUIRED )
FIND_PACKAGE( CLucene 0.9.23 REQUIRED )

FIND_PACKAGE( QJSON REQUIRED )

# optional deps
INCLUDE( MacroOptionalFindPackage )
INCLUDE( MacroLogFeature )
macro_optional_find_package(Gloox 1.0)
macro_log_feature(GLOOX_FOUND "Gloox" "A portable high-level Jabber/XMPP library for C++" "http://camaya.net/gloox" FALSE "" "Gloox is needed for the Jabber SIP plugin and the XMPP-Bot")

macro_display_feature_log()


ADD_SUBDIRECTORY( thirdparty )
ADD_SUBDIRECTORY( src/libtomahawk )
ADD_SUBDIRECTORY( src )
