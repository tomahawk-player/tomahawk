PROJECT( tomahawk )
CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )

###
### Tomahawk application info
###
SET( ORGANIZATION_NAME "Tomahawk" )
SET( ORGANIZATION_DOMAIN "tomahawk-player.org" )
SET( APPLICATION_NAME  "Tomahawk" )
SET( VERSION  "0.0.1" )


# set paths
SET( CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${CMAKE_SOURCE_DIR}/CMakeModules" )
SET( THIRDPARTY_DIR  ${CMAKE_SOURCE_DIR}/thirdparty )

# Check if we need qtgui:
IF( "${gui}" STREQUAL "no" )
    ADD_DEFINITIONS( -DENABLE_HEADLESS )
    MESSAGE( STATUS "Building in HEADLESS mode ***" )
	FIND_PACKAGE( Qt4 4.7.0 COMPONENTS QtCore QtXml QtNetwork REQUIRED )
ELSE()
    MESSAGE( STATUS "Building full GUI version ***" )
	FIND_PACKAGE( Qt4 4.7.0 COMPONENTS QtGui QtCore QtXml QtNetwork REQUIRED )
ENDIF()

#deps
INCLUDE( MacroOptionalFindPackage )
INCLUDE( MacroLogFeature )

# required
#While we distribute our own liblastfm2, don't need to look for it
#macro_optional_find_package(LibLastFm 0.3.3)
#macro_log_feature(LIBLASTFM_FOUND "LastFm" "Qt library for the Last.fm webservices" "https://github.com/mxcl/liblastfm" FALSE "" "liblastfm is needed for scrobbling tracks to Last.fm and fetching cover artwork")
set(LIBLASTFM_FOUND true)

macro_optional_find_package(LibEchonest 1.1.1)
macro_log_feature(LIBECHONEST_FOUND "Echonest" "Qt library for communicating with The Echo Nest" "http://projects.kde.org/libechonest" TRUE "" "libechonest is needed for dynamic playlists and the infosystem")

macro_optional_find_package(CLucene 0.9.23)
macro_log_feature(CLucene_FOUND "CLucene" "The open-source, C++ search engine" "http://clucene.sf.net" TRUE "" "CLucene is used for indexing the collection")

macro_optional_find_package(QJSON)
macro_log_feature(QJSON_FOUND "QJson" "Qt library that maps JSON data to QVariant objects" "http://qjson.sf.net" TRUE "" "libqjson is used for encoding communication between Tomahawk instances")

macro_optional_find_package(Taglib 1.6.0)
macro_log_feature(TAGLIB_FOUND "TagLib" "Audio Meta-Data Library" "http://developer.kde.org/~wheeler/taglib.html" TRUE "" "taglib is needed for reading meta data from audio files")

# we need pthreads too
find_package(Threads)

include( CheckTagLibFileName )
check_taglib_filename( COMPLEX_TAGLIB_FILENAME )

# optional
macro_optional_find_package(Jreen)
IF( ENABLE_JREEN AND NOT LIBJREEN_FOUND )
    ADD_SUBDIRECTORY( thirdparty/jreen )
    SET( LIBJREEN_INCLUDE_DIR  ${CMAKE_CURRENT_BINARY_DIR}/thirdparty/jreen/include )
    IF( UNIX AND NOT APPLE )
        SET( LIBJREEN_LIBRARY ${CMAKE_CURRENT_BINARY_DIR}/thirdparty/jreen/libjreen.so )
    ENDIF( UNIX AND NOT APPLE )
    IF( WIN32 )
	SET( LIBJREEN_LIBRARY ${CMAKE_CURRENT_BINARY_DIR}/thirdparty/jreen/libjreen.dll )
    ENDIF( WIN32 )
    SET( LIBJREEN_FOUND  true )
    MESSAGE(STATUS "Internal libjreen: ${LIBJREEN_INCLUDE_DIR}, ${LIBJREEN_LIBRARY}")
ENDIF( ENABLE_JREEN AND NOT LIBJREEN_FOUND )

IF( WIN32 )
  find_library(QTSPARKLE_LIBRARIES qtsparkle)
ENDIF( WIN32 )

macro_log_feature(JREEN_FOUND "Jreen" "Qt XMPP library" "http://gitorious.org/jreen" FALSE "" "Jreen is needed for the alternative/new Jabber SIP plugin. Built automatically inside Tomahawk, if not installed systemwide and ENABLE_JREEN is true")

macro_optional_find_package(Gloox 1.0)
IF( ENABLE_JREEN )
    set( GLOOX_FOUND  false )
ENDIF( ENABLE_JREEN)
macro_log_feature(GLOOX_FOUND "Gloox" "A portable high-level Jabber/XMPP library for C++" "http://camaya.net/gloox" FALSE "" "Gloox is needed for the Jabber SIP plugin and the XMPP-Bot")
#show dep log
macro_display_feature_log()
MESSAGE("WARNING!")
MESSAGE("Phonon is going to replace our own audio abstraction layer soon so we didn't")
MESSAGE("add checks for libmad, libvorbis and libflac. Make sure they are installed!")
MESSAGE("")
MESSAGE("-----------------------------------------------------------------------------")

SET( INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}" )

# make uninstall support
CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
  IMMEDIATE @ONLY)

ADD_CUSTOM_TARGET(uninstall
  "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")


IF( NOT APPLE )
    # Make linking as strict on linux as it is on osx. Then we don't break linking on mac so often
    SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--no-undefined" )
ENDIF( NOT APPLE )

ADD_SUBDIRECTORY( thirdparty )
ADD_SUBDIRECTORY( src/libtomahawk )
ADD_SUBDIRECTORY( src )
ADD_SUBDIRECTORY( admin )
