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

#deps
INCLUDE( MacroOptionalFindPackage )
INCLUDE( MacroLogFeature )

# required
macro_optional_find_package(LibLastFm 0.3.3)
macro_log_feature(LIBLASTFM_FOUND "LastFm" "Qt library for the Last.fm webservices" "https://github.com/mxcl/liblastfm" TRUE "" "liblastfm is needed for scrobbling tracks to Last.fm and fetching cover artwork")

macro_optional_find_package(LibEchonest 1.1.1)
macro_log_feature(LIBECHONEST_FOUND "Echonest" "Qt library for communicating with The Echo Nest" "http://projects.kde.org/libechonest" TRUE "" "libechonest is needed for dynamic playlists and the infosystem")

macro_optional_find_package(CLucene 0.9.23)
macro_log_feature(CLucene_FOUND "CLucene" "The open-source, C++ search engine" "http://clucene.sf.net" TRUE "" "CLucene is used for indexing the collection")

macro_optional_find_package(QJSON)
macro_log_feature(QJSON_FOUND "QJson" "Qt library that maps JSON data to QVariant objects" "http://qjson.sf.net" TRUE "" "libqjson is used for encoding communication between Tomahawk instances")

macro_optional_find_package(Taglib 1.6.0)
macro_log_feature(TAGLIB_FOUND "TagLib" "Audio Meta-Data Library" "http://developer.kde.org/~wheeler/taglib.html" TRUE "" "taglib is needed for reading meta data from audio files")


FIND_PACKAGE( Taglib 1.6.0 REQUIRED )
include( CheckTagLibFileName )
check_taglib_filename( COMPLEX_TAGLIB_FILENAME )

# optional
macro_optional_find_package(Gloox 1.0)
macro_log_feature(GLOOX_FOUND "Gloox" "A portable high-level Jabber/XMPP library for C++" "http://camaya.net/gloox" FALSE "" "Gloox is needed for the Jabber SIP plugin and the XMPP-Bot")

#show dep log
macro_display_feature_log()
MESSAGE("WARNING!")
MESSAGE("Phonon is going to replace our own audio abstraction layer soon so we didn't")
MESSAGE("add checks for libmad, libvorbis and libflac. Make sure they are installed!")
MESSAGE("")
MESSAGE("-----------------------------------------------------------------------------")

ADD_SUBDIRECTORY( thirdparty )
ADD_SUBDIRECTORY( src/libtomahawk )
ADD_SUBDIRECTORY( src )
