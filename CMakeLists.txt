PROJECT( tomahawk )
CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
SET( CMAKE_MODULE_PATH  "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules" )

IF( ${CMAKE_VERSION} VERSION_GREATER 2.8.3 )
  CMAKE_POLICY(SET CMP0017 NEW)
ENDIF( ${CMAKE_VERSION} VERSION_GREATER 2.8.3 )

###
### Tomahawk application info
###
SET( TOMAHAWK_ORGANIZATION_NAME "Tomahawk" )
SET( TOMAHAWK_ORGANIZATION_DOMAIN "tomahawk-player.org" )
SET( TOMAHAWK_APPLICATION_NAME  "Tomahawk" )
SET( TOMAHAWK_DESCRIPTION_SUMMARY "The social media player" )

SET( TOMAHAWK_VERSION_MAJOR 0 )
SET( TOMAHAWK_VERSION_MINOR 2 )
SET( TOMAHAWK_VERSION_PATCH 99 )

#SET( TOMAHAWK_VERSION_RC 0 )


# build options
option(BUILD_GUI "Build Tomahawk with GUI" ON)
option(BUILD_RELEASE "Generate TOMAHAWK_VERSION without GIT info" OFF)

# generate version string

# base string used in release and unstable builds
SET( TOMAHAWK_VERSION ${TOMAHAWK_VERSION_MAJOR}.${TOMAHAWK_VERSION_MINOR}.${TOMAHAWK_VERSION_PATCH} )
IF( TOMAHAWK_VERSION_RC )
  SET( TOMAHAWK_VERSION ${TOMAHAWK_VERSION}rc${TOMAHAWK_VERSION_RC} )
ENDIF()

# additional info for non-release builds
IF( NOT BUILD_RELEASE )
    INCLUDE( CMakeDateStamp )
    SET( TOMAHAWK_VERSION_DATE "${CMAKE_DATESTAMP_YEAR}${CMAKE_DATESTAMP_MONTH}${CMAKE_DATESTAMP_DAY}" )
    IF( ${TOMAHAWK_VERSION_DATE} GREATER 0)
        SET( TOMAHAWK_VERSION ${TOMAHAWK_VERSION}.${TOMAHAWK_VERSION_DATE} )
    ENDIF()

    INCLUDE( CMakeVersionSource )
    IF( CMAKE_VERSION_SOURCE )
        SET( TOMAHAWK_VERSION ${TOMAHAWK_VERSION}-${CMAKE_VERSION_SOURCE} )
    ENDIF()
ENDIF()



# set paths
SET( THIRDPARTY_DIR  ${CMAKE_SOURCE_DIR}/thirdparty )
SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}" )
SET( CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}" )
SET( CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}" )

# installer creation
INCLUDE( TomahawkCPack.cmake )

#deps
INCLUDE( MacroOptionalFindPackage )
INCLUDE( MacroLogFeature )

# Check if we need qtgui:
SET(NEEDED_QT4_COMPONENTS QtCore QtXml QtNetwork )
IF( NOT BUILD_GUI )
    ADD_DEFINITIONS( -DENABLE_HEADLESS )
    MESSAGE( STATUS "Building Tomahawk ${TOMAHAWK_VERSION} in HEADLESS mode ***" )
ELSE()
    MESSAGE( STATUS "Building Tomahawk ${TOMAHAWK_VERSION} full GUI version ***" )
    LIST(APPEND NEEDED_QT4_COMPONENTS "QtGui" "QtWebkit" )
ENDIF()

IF( BUILD_GUI AND UNIX AND NOT APPLE )
    FIND_PACKAGE( X11 )
ENDIF()

macro_optional_find_package(Qt4 4.7.0 COMPONENTS ${NEEDED_QT4_COMPONENTS} )
macro_log_feature(QT4_FOUND "Qt" "A cross-platform application and UI framework" "http://qt.nokia.com" TRUE "" "If you see this, although libqt4-devel is installed, check whether \n     the qtwebkit-devel package is installed as well")

macro_optional_find_package(Phonon 4.5.0)
macro_log_feature(PHONON_FOUND "Phonon" "The Phonon multimedia library" "http://phonon.kde.org" TRUE "" "")
if(PHONON_FOUND)
    message(STATUS "Phonon found; ensure that phonon-vlc is at least 0.4")
endif()

macro_optional_find_package(LibEchonest 1.1.7)
macro_log_feature(LIBECHONEST_FOUND "Echonest" "Qt library for communicating with The Echo Nest" "http://projects.kde.org/libechonest" TRUE "" "libechonest 1.1.8 is needed for dynamic playlists and the infosystem")

macro_optional_find_package(CLucene 0.9.23)
macro_log_feature(CLucene_FOUND "CLucene" "The open-source, C++ search engine" "http://clucene.sf.net" TRUE "" "CLucene is used for indexing the collection")

macro_optional_find_package(QJSON)
macro_log_feature(QJSON_FOUND "QJson" "Qt library that maps JSON data to QVariant objects" "http://qjson.sf.net" TRUE "" "libqjson is used for encoding communication between Tomahawk instances")

macro_optional_find_package(Taglib 1.6.0)
macro_log_feature(TAGLIB_FOUND "TagLib" "Audio Meta-Data Library" "http://developer.kde.org/~wheeler/taglib.html" TRUE "" "taglib is needed for reading meta data from audio files")
include( CheckTagLibFileName )
check_taglib_filename( COMPLEX_TAGLIB_FILENAME )

macro_optional_find_package(Boost)
macro_log_feature(Boost_FOUND "Boost" "Provides free peer-reviewed portable C++ source libraries" "http://www.boost.org" TRUE "" "") #FIXME: give useful explaination

macro_optional_find_package(QCA2)
macro_log_feature(QCA2_FOUND "QCA2" "Provides encryption and signing functions required for Grooveshark resolver" "http://delta.affinix.com/qca/" TRUE "" "")

# required
#While we distribute our own liblastfm2, don't need to look for it
#macro_optional_find_package(LibLastFm 0.3.3)
#macro_log_feature(LIBLASTFM_FOUND "LastFm" "Qt library for the Last.fm webservices" "https://github.com/mxcl/liblastfm" FALSE "" "liblastfm is needed for scrobbling tracks to Last.fm and fetching cover artwork")
set(LIBLASTFM_FOUND true)


# this installs headers and such and should really be handled in a separate package by packagers
IF( INTERNAL_JREEN )
    ADD_SUBDIRECTORY( ${THIRDPARTY_DIR}/jreen )
    SET( LIBJREEN_INCLUDE_DIR  ${CMAKE_CURRENT_BINARY_DIR}/thirdparty/jreen/include )
    SET( LIBJREEN_LIBRARY jreen )
    SET( LIBJREEN_LIBRARIES ${LIBJREEN_LIBRARY} )
    SET( LIBJREEN_FOUND  true )
    MESSAGE(STATUS "INTERNAL libjreen: ${LIBJREEN_INCLUDE_DIR}, ${LIBJREEN_LIBRARY}")
ELSE( INTERNAL_JREEN )
    macro_optional_find_package(Jreen)
ENDIF( INTERNAL_JREEN )
macro_log_feature(LIBJREEN_FOUND "Jreen" "Qt XMPP Library" "https://github.com/euroelessar/jreen" FALSE "" "Jreen is needed for the Jabber SIP plugin. \n\n     Use -DINTERNAL_JREEN=ON to build the git submodule inside Tomahawk \n     Be aware this installs a full jreen with headers and everything!\n")

# this installs headers and such and should really be handled in a separate package by packagers
IF( INTERNAL_QTWEETLIB )
    ADD_SUBDIRECTORY( ${THIRDPARTY_DIR}/qtweetlib )
    # copy headers to build/QTweetLib so we can use proper includes inside the code
    FILE( COPY ${CMAKE_SOURCE_DIR}/thirdparty/qtweetlib/QTweetLib/src/ DESTINATION ${CMAKE_BINARY_DIR}/QTweetLib )
    SET( QTWEETLIB_INCLUDE_DIR  ${CMAKE_BINARY_DIR} )
    SET( QTWEETLIB_LIBRARY  tomahawk_qtweetlib )
    SET( QTWEETLIB_LIBRARIES ${QTWEETLIB_LIBRARY} )
    SET( QTWEETLIB_FOUND  true )
    MESSAGE(STATUS "INTERNAL libQTweetLib: ${QTWEETLIB_INCLUDE_DIR}, ${QTWEETLIB_LIBRARY}")
ELSE( INTERNAL_QTWEETLIB )
    macro_optional_find_package(QTweetLib)
ENDIF( INTERNAL_QTWEETLIB )
macro_log_feature(QTWEETLIB_FOUND "QTweetLib" "Qt Twitter Library" "https://github.com/minimoog/QTweetLib" FALSE "" "QTweetLib is needed for the Twitter SIP plugin. \n\n     Use -DINTERNAL_QTWEETLIB=ON to build the git submodule inside Tomahawk \n")

### libportfwd
SET( LIBPORTFWD_INCLUDE_DIR ${THIRDPARTY_DIR}/libportfwd/include )
SET( LIBPORTFWD_LIBRARY tomahawk_portfwd )
SET( LIBPORTFWD_LIBRARIES ${LIBPORTFWD_LIBRARY} )
ADD_SUBDIRECTORY( ${THIRDPARTY_DIR}/libportfwd )

# we need pthreads too
macro_optional_find_package(Threads)
macro_log_feature(THREADS_FOUND "Threads" "Threading Library" "" TRUE "" "Platform specific library for threading")

macro_optional_find_package(KDE4)
macro_optional_find_package(KDE4Installed)

# this was used before we had FindKDE4Installed, just leaving it here to keep the flags
# for future kde integration

# macro_optional_find_package(KDE4)
IF(KDE4_FOUND)
    #KDE4 adds and removes some compiler flags that we don't like
    STRING( REPLACE "-std=iso9899:1990" "" CLEAN_C_FLAGS ${CMAKE_C_FLAGS} )
    SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions" )
ELSE()
    SET( CLEAN_C_FLAGS ${CMAKE_C_FLAGS} )
ENDIF()

IF( WIN32 )
  find_library(QTSPARKLE_LIBRARIES qtsparkle)
ENDIF( WIN32 )

#show dep log
macro_display_feature_log()

SET( INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}" )

# make uninstall support
CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
  IMMEDIATE @ONLY)

# KDE4 defines an uninstall target for us automatically
IF( NOT KDE4_FOUND )
    ADD_CUSTOM_TARGET(uninstall "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")
ENDIF()

IF( NOT APPLE )
    # Make linking as strict on linux as it is on osx. Then we don't break linking on mac so often
    SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--no-undefined" )
ENDIF( NOT APPLE )

ADD_SUBDIRECTORY( thirdparty )
ADD_SUBDIRECTORY( src/libtomahawk )
SET( TOMAHAWK_LIBRARIES tomahawklib )
ADD_SUBDIRECTORY( src )
ADD_SUBDIRECTORY( admin )
