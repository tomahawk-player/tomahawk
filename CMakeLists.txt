PROJECT( tomahawk )
CMAKE_MINIMUM_REQUIRED( VERSION 2.8.6 )
SET( CMAKE_MODULE_PATH  "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules" )
CMAKE_POLICY(SET CMP0017 NEW)

###
### Tomahawk application info
###
SET( TOMAHAWK_ORGANIZATION_NAME "Tomahawk" )
SET( TOMAHAWK_ORGANIZATION_DOMAIN "tomahawk-player.org" )
SET( TOMAHAWK_APPLICATION_NAME  "Tomahawk" )
SET( TOMAHAWK_DESCRIPTION_SUMMARY "The social media player" )

SET( TOMAHAWK_VERSION_MAJOR 0 )
SET( TOMAHAWK_VERSION_MINOR 6 )
SET( TOMAHAWK_VERSION_PATCH 0 )
#SET( TOMAHAWK_VERSION_RC 0 )

SET( TOMAHAWK_TRANSLATION_LANGUAGES ar bg bn_IN ca cs de en el es fi fr hi_IN hu gl it ja lt pl pt_BR ru sv tr zh_CN zh_TW )

# enforce proper symbol exporting on all platforms
add_definitions( "-fvisibility=hidden" )
# enforce using constBegin, constEnd for const-iterators
add_definitions( "-DQT_STRICT_ITERATORS" )

# build options
option(BUILD_GUI "Build Tomahawk with GUI" ON)
option(BUILD_RELEASE "Generate TOMAHAWK_VERSION without GIT info" OFF)
option(WITH_BREAKPAD "Build with breakpad integration" ON)
option(WITH_CRASHREPORTER "Build with CrashReporter" ON)
option(WITH_BINARY_ATTICA "Enable support for downloading binary resolvers automatically" ON)
option(LEGACY_KDE_INTEGRATION "Install tomahawk.protocol file, deprecated since 4.6.0" OFF)

IF( CMAKE_SYSTEM_PROCESSOR MATCHES "arm" )
    message(STATUS "Build of breakpad library disabled on this platform.")
    SET(WITH_BREAKPAD OFF)
    SET(WITH_CRASHREPORTER OFF)
ENDIF()

# add definitions based on build options
IF( WITH_BREAKPAD )
    message(STATUS "Build with support for breakpad.")
    IF( CMAKE_COMPILER_IS_GNUCXX )
        ADD_DEFINITIONS( -DSTDC_HEADERS -std=gnu++98 )
#        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSTDC_HEADERS -std=gnu++98" )
    ENDIF()
ENDIF()

# generate version string

# base string used in release and unstable builds
SET( TOMAHAWK_VERSION ${TOMAHAWK_VERSION_MAJOR}.${TOMAHAWK_VERSION_MINOR}.${TOMAHAWK_VERSION_PATCH} )
IF( TOMAHAWK_VERSION_RC )
  SET( TOMAHAWK_VERSION ${TOMAHAWK_VERSION}rc${TOMAHAWK_VERSION_RC} )
ENDIF()

# additional info for non-release builds
IF( NOT BUILD_RELEASE AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git/" )
    INCLUDE( CMakeDateStamp )
    SET( TOMAHAWK_VERSION_DATE "${CMAKE_DATESTAMP_YEAR}${CMAKE_DATESTAMP_MONTH}${CMAKE_DATESTAMP_DAY}" )
    IF( TOMAHAWK_VERSION_DATE GREATER 0)
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

# make predefined install dirs available everywhere
INCLUDE( GNUInstallDirs )

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
    LIST(APPEND NEEDED_QT4_COMPONENTS "QtGui" "QtWebkit" "QtUiTools" )
ENDIF()

IF( BUILD_GUI AND UNIX AND NOT APPLE )
    FIND_PACKAGE( X11 )
ENDIF()

macro_optional_find_package(Qt4 4.7.0 COMPONENTS ${NEEDED_QT4_COMPONENTS} )
macro_log_feature(QT4_FOUND "Qt" "A cross-platform application and UI framework" "http://qt.nokia.com" TRUE "" "If you see this, although libqt4-devel is installed, check whether the \n     qtwebkit-devel package and whatever contains QtUiTools is installed too")

macro_optional_find_package(Phonon 4.5.0)
macro_log_feature(PHONON_FOUND "Phonon" "The Phonon multimedia library" "http://phonon.kde.org" TRUE "" "")
if(PHONON_FOUND)
    message(STATUS "Phonon found; ensure that phonon-vlc is at least 0.4")
endif()

macro_optional_find_package(Echonest 2.0.0)
macro_log_feature(ECHONEST_FOUND "Echonest" "Qt library for communicating with The Echo Nest" "http://projects.kde.org/libechonest" TRUE "" "libechonest 2.0.0 is needed for dynamic playlists and the infosystem")

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

macro_optional_find_package(LibAttica 0.4.0)
macro_log_feature(LIBATTICA_FOUND "libattica" "Provides support for installation of resolvers from the Tomahawk website" "https://projects.kde.org/projects/kdesupport/attica" TRUE "" "")

macro_optional_find_package(QuaZip)
macro_log_feature(QuaZip_FOUND "QuaZip" "Provides support for extracting downloaded resolvers automatically." "http://quazip.sourceforge.net/" TRUE "" "")

macro_optional_find_package(Jreen 1.0.5)
macro_log_feature(JREEN_FOUND "Jreen" "Qt XMPP Library" "http://qutim.org/jreen / https://github.com/euroelessar/jreen" FALSE "" "Jreen is needed for the Jabber SIP plugin.\n")
if(PC_JREEN_VERSION STREQUAL "1.1.0")
    message(FATAL_ERROR "Jreen 1.1.0 has a very annoying bug that breaks accepting auth requests in Tomahawk. Please upgrade to 1.1.1 or downgrade to 1.0.5.")
endif()

macro_optional_find_package(QTweetLib)
macro_log_feature(QTWEETLIB_FOUND "QTweetLib" "Qt Twitter Library" "https://github.com/minimoog/QTweetLib" FALSE "" "QTweetLib is needed for the Twitter SIP plugin.\n")

macro_optional_find_package(LibLastFm 1.0.0)
macro_log_feature(LIBLASTFM_FOUND "liblastfm" "Qt library for the Last.fm webservices" "https://github.com/eartle/liblastfm" TRUE "" "liblastfm is needed for scrobbling tracks to Last.fm and fetching cover artwork")

IF( WIN32 )
    macro_optional_find_package(QtSparkle)
    macro_log_feature(QTSPARKLE_FOUND "qtsparkle" "Library for creating auto updaters written in Qt" "https://github.com/davidsansome/qtsparkle" FALSE "" "")
ENDIF( WIN32 )

#### submodules start
# automatically init submodules here, don't delete this code we may add submodules again

# this installs headers and such and should really be handled in a separate package by packagers

# IF( FALSE )
#     IF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/.gitmodules)
#         EXECUTE_PROCESS(COMMAND git submodule init WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} )
#         EXECUTE_PROCESS(COMMAND git submodule update WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} )
#     ENDIF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/.gitmodules)
# ENDIF()

#### submodules end

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
    IF( CMAKE_C_FLAGS )
        # KDE4 adds and removes some compiler flags that we don't like
        # (only for gcc not for clang e.g.)
        STRING( REPLACE "-std=iso9899:1990" "" CLEAN_C_FLAGS ${CMAKE_C_FLAGS} )
        SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions" )
    ENDIF()
ELSE()
    SET( CLEAN_C_FLAGS ${CMAKE_C_FLAGS} )
ENDIF()

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

SET( TOMAHAWK_LIBRARIES tomahawklib )

ADD_SUBDIRECTORY( thirdparty )
ADD_SUBDIRECTORY( src )
ADD_SUBDIRECTORY( src/libtomahawk )
ADD_SUBDIRECTORY( admin )
