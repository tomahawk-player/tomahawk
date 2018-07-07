PROJECT( tomahawk )
CMAKE_MINIMUM_REQUIRED( VERSION 2.8.12 )
CMAKE_POLICY(SET CMP0017 NEW)
CMAKE_POLICY(SET CMP0022 NEW)

# TODO:
# Update to NEW and fix things up
CMAKE_POLICY(SET CMP0023 OLD)

# TODO:
# Disable automatic qtmain linking
CMAKE_POLICY(SET CMP0020 OLD)

find_package(ECM 1.7.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR} "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules" )



INCLUDE(CMakeDependentOption)

###
### Tomahawk application info
###
SET( TOMAHAWK_ORGANIZATION_NAME "Tomahawk" )
SET( TOMAHAWK_ORGANIZATION_DOMAIN "tomahawk-player.org" )
SET( TOMAHAWK_APPLICATION_NAME  "Tomahawk" )
SET( TOMAHAWK_DESCRIPTION_SUMMARY "The social media player" )
SET( TOMAHAWK_APPLICATION_PACKAGE_NAME "org.tomahawk-player.Tomahawk")
IF(APPLE)
    SET( TOMAHAWK_TARGET_NAME "Tomahawk" )
ELSE()
    SET( TOMAHAWK_TARGET_NAME "tomahawk" )
ENDIF()

STRING(TOLOWER "${TOMAHAWK_TARGET_NAME}" TOMAHAWK_BASE_TARGET_NAME)

IF( WIN32 )
    SET( TOMAHAWK_SYSTEM "Windows" )
ELSEIF( APPLE )
    SET( TOMAHAWK_SYSTEM "OS X" )
ELSEIF( UNIX )
    SET( TOMAHAWK_SYSTEM "Linux")
ELSE()
    SET( TOMAHAWK_SYSTEM "Uknown Platform, please let the Tomahawk team know, this actually happened" )
ENDIF()

SET( TOMAHAWK_VERSION_MAJOR 0 )
SET( TOMAHAWK_VERSION_MINOR 8 )
SET( TOMAHAWK_VERSION_PATCH 99 )
SET( TOMAHAWK_VERSION_RC 0 )

SET( TOMAHAWK_TRANSLATION_LANGUAGES ar bg bn_IN ca cs da de en el es es_419 fi fr hi_IN hu gl id it ja lt nl pl pt_BR pt_PT ro ru sq sv th tr uk vi zh_CN zh_TW )

# add_definitions is only in the scope of this directory and all directories
# below it. Tomahawk Libraries and plugins that are built with Tomahawk are
# not included in out-of-source build so we need to re-add_definitions there.
macro(tomahawk_add_definitions DEFINITION)
    add_definitions( ${DEFINITION} )
    set( TOMAHAWK_DEFINITIONS "${TOMAHAWK_DEFINITIONS} ${DEFINITION}")
endmacro()

macro(tomahawk_add_cxx_flags FLAGS)
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FLAGS}")
    set( TOMAHAWK_CXX_FLAGS "${TOMAHAWK_CXX_FLAGS} ${FLAGS}")
endmacro()

macro(tomahawk_add_c_flags FLAGS)
    set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FLAGS}")
    set( TOMAHAWK_C_FLAGS "${TOMAHAWK_C_FLAGS} ${FLAGS}")
endmacro()

if(NOT WIN32)
    # enforce proper symbol exporting on all platforms but Windows
    tomahawk_add_cxx_flags( "-fvisibility=hidden" )
    tomahawk_add_c_flags( "-fvisibility=hidden" )
endif()

# enforce using constBegin, constEnd for const-iterators
tomahawk_add_definitions( "-DQT_STRICT_ITERATORS" )

# build options
option(BUILD_RELEASE "Generate TOMAHAWK_VERSION without GIT info" OFF)
if(BUILD_RELEASE)
    set(BUILD_NO_RELEASE OFF)
else()
    set(BUILD_NO_RELEASE ON)
endif()

option(BUILD_GUI "Build Tomahawk with GUI" ON)
option(BUILD_TESTS "Build Tomahawk with unit tests" ${BUILD_NO_RELEASE})
option(BUILD_TOOLS "Build Tomahawk helper tools" ${BUILD_NO_RELEASE})
option(BUILD_HATCHET "Build the Hatchet plugin" ON)

if(UNIX AND NOT APPLE)
       set(CRASHREPORTER_ENABLED_BY_DEFAULT OFF)
else()
       set(CRASHREPORTER_ENABLED_BY_DEFAULT ON)
endif()
option(WITH_CRASHREPORTER "Build with CrashReporter" ${CRASHREPORTER_ENABLED_BY_DEFAULT})
option(WITH_BINARY_ATTICA "Enable support for downloading binary resolvers automatically" ON)

# build options for development purposes
option(SANITIZE_ADDRESS "Enable Address Sanitizer for memory error detection" OFF)
option(TOMAHAWK_FINEGRAINED_MESSAGES "Enable even more verbose logging (will hurt performance significantly" OFF)

CMAKE_DEPENDENT_OPTION(WITH_UPOWER "Build with support for UPower events" ON
       "UNIX;NOT APPLE" OFF)
CMAKE_DEPENDENT_OPTION(WITH_GNOMESHORTCUTHANDLER "Build with shortcut handler for GNOME" ON
       "UNIX;NOT APPLE" OFF)

IF( CMAKE_SYSTEM_PROCESSOR MATCHES "arm" OR NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/libcrashreporter-qt/CMakeLists.txt")
    message(STATUS "Build of crashreporter disabled.")
    SET(WITH_CRASHREPORTER OFF)
ENDIF()

# generate version string

# base string used in release and unstable builds
SET( TOMAHAWK_VERSION_TMP "${TOMAHAWK_VERSION_MAJOR}.${TOMAHAWK_VERSION_MINOR}.${TOMAHAWK_VERSION_PATCH}")
SET( TOMAHAWK_VERSION_SHORT "${TOMAHAWK_VERSION_TMP}" )
IF( TOMAHAWK_VERSION_RC )
  SET( TOMAHAWK_VERSION_TMP "${TOMAHAWK_VERSION_TMP}rc${TOMAHAWK_VERSION_RC}")
ENDIF()

# additional info for non-release builds
IF( NOT BUILD_RELEASE AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git/" )
    INCLUDE( CMakeDateStamp )
    SET( TOMAHAWK_VERSION_DATE "${CMAKE_DATESTAMP_YEAR}${CMAKE_DATESTAMP_MONTH}${CMAKE_DATESTAMP_DAY}" )
    IF( TOMAHAWK_VERSION_DATE GREATER 0 )
        SET( TOMAHAWK_VERSION_TMP ${TOMAHAWK_VERSION_TMP}.${TOMAHAWK_VERSION_DATE} )
    ENDIF()

    INCLUDE( CMakeVersionSource )
    IF( CMAKE_VERSION_SOURCE )
        SET( TOMAHAWK_VERSION_TMP ${TOMAHAWK_VERSION_TMP}-${CMAKE_VERSION_SOURCE} )
    ENDIF()
ENDIF()

# write Tomahawk version to cache
SET(TOMAHAWK_VERSION "${TOMAHAWK_VERSION_TMP}" CACHE STRING "Tomahawk Version")


# set paths
SET( THIRDPARTY_DIR "${CMAKE_SOURCE_DIR}/thirdparty" )
SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}" )
SET( CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}" )
SET( CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}" )

# make predefined install dirs available everywhere
INCLUDE( GNUInstallDirs )

# installer creation
INCLUDE( TomahawkCPack.cmake )

# deps
INCLUDE( MacroOptionalFindPackage )
INCLUDE( MacroLogFeature )

message( STATUS "Building Tomahawk ${TOMAHAWK_VERSION} ***" )

find_package(Qt5Core REQUIRED)
find_package(Qt5Concurrent REQUIRED)
find_package(Qt5Svg REQUIRED)
find_package(Qt5UiTools REQUIRED)
find_package(Qt5WebKitWidgets REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Xml REQUIRED)
find_package(Qt5X11Extras NO_MODULE QUIET)

if(Qt5X11Extras_FOUND)
    set(HAVE_X11 TRUE)
else()
    set(HAVE_X11 FALSE)
endif()

if( UNIX AND NOT APPLE )
    # We need this to find the paths to qdbusxml2cpp and co
    find_package(Qt5DBus REQUIRED)
endif()

if(APPLE)
    find_package(Qt5MacExtras REQUIRED)
endif()

if(WIN32)
    find_package(Qt5WinExtras REQUIRED)
endif()

find_package(Qt5LinguistTools REQUIRED)

set(QT_RCC_EXECUTABLE "${Qt5Core_RCC_EXECUTABLE}")
# FIXME: CrashReporter depends on deprecated QHttp

include(CheckCXXCompilerFlag)
check_cxx_compiler_flag( "-std=c++11" CXX11_FOUND )
check_cxx_compiler_flag( "-std=c++0x" CXX0X_FOUND )
check_cxx_compiler_flag( "-stdlib=libc++" LIBCPP_FOUND )
if(CXX11_FOUND)
    tomahawk_add_cxx_flags( "-std=c++11" )
elseif(CXX0X_FOUND)
    tomahawk_add_cxx_flags( "-std=c++0x" )
else()
    message(STATUS "${CMAKE_CXX_COMPILER} does not support C++11, please use a
    different compiler")
endif()
if(("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR APPLE) AND LIBCPP_FOUND)
    tomahawk_add_cxx_flags( "-stdlib=libc++" )
endif()

find_package(Boost REQUIRED COMPONENTS filesystem system)
macro_log_feature(Boost_FOUND "Boost" "Provides free peer-reviewed portable C++ source libraries" "http://www.boost.org" TRUE "" "") #FIXME: give useful explanation

macro_optional_find_package(Lucene++ 3.0.0)
macro_log_feature(LUCENEPP_FOUND "Lucene++" "The open-source, C++ search engine" "https://github.com/luceneplusplus/LucenePlusPlus/" TRUE "" "Lucene++ is used for indexing the collection")

macro_optional_find_package(Taglib 1.8.0)
macro_log_feature(TAGLIB_FOUND "TagLib" "Audio Meta-Data Library" "http://developer.kde.org/~wheeler/taglib.html" TRUE "" "taglib is needed for reading meta data from audio files")
include( CheckTagLibFileName )
check_taglib_filename( COMPLEX_TAGLIB_FILENAME )

macro_optional_find_package(Sparsehash)
macro_log_feature(SPARSEHASH_FOUND "Sparsehash"
    "An extremely memory-efficient hash_map implementation."
    "https://code.google.com/p/sparsehash/" TRUE ""
    "Sparsehash is needed for reading metadata of mediastreams and fast
    forward/backward seeking in HTTP streams")

macro_optional_find_package(GnuTLS)
macro_log_feature(GNUTLS_FOUND "GnuTLS"
    "GnuTLS is a secure communications library implementing the SSL, TLS and DTLS protocols and technologies around them."
    "http://gnutls.org/" TRUE ""
    "GnuTLS is needed for serving the Playdar/HTTP API via TLS")

macro_optional_find_package(Qca-qt5)
if(Qca-qt5_DIR)
    set(QCA2_FOUND ON CACHE BOOL "QCA2 was found")
    set(QCA2_LIBRARIES "qca-qt5" CACHE STRING "QCA2 Qt5 target")
endif()
macro_log_feature(QCA2_FOUND "QCA2" "Provides encryption and signing functions necessary for some resolvers and accounts" "http://delta.affinix.com/qca/" TRUE "" "")

macro_optional_find_package(KF5Attica 1.0.0)
set(LIBATTICA_FOUND ${KF5Attica_FOUND})
macro_log_feature(LIBATTICA_FOUND "libattica" "Provides support for installation of resolvers from the Tomahawk website" "http://download.kde.org/stable/attica/" TRUE "" "")

macro_optional_find_package(QuaZip)
macro_log_feature(QUAZIP_FOUND "QuaZip" "Provides support for extracting downloaded resolvers automatically." "http://quazip.sourceforge.net/" TRUE "" "")

macro_optional_find_package(Jreen 1.0.5)
macro_log_feature(JREEN_FOUND "Jreen" "Qt XMPP Library" "http://qutim.org/jreen / https://github.com/euroelessar/jreen" FALSE "" "Jreen is needed for the Jabber SIP plugin.\n")
if( PC_JREEN_VERSION STREQUAL "1.1.0" )
    message(FATAL_ERROR "Jreen 1.1.0 has a very annoying bug that breaks accepting auth requests in Tomahawk. Please upgrade to 1.1.1 or downgrade to 1.0.5.")
endif()

macro_optional_find_package(LibLastFm 1.0.0)
macro_log_feature(LIBLASTFM_FOUND "liblastfm" "Qt library for the Last.fm webservices" "https://github.com/lastfm/liblastfm" TRUE "" "liblastfm is needed for scrobbling tracks to Last.fm and fetching cover artwork")

if( NOT APPLE )
macro_optional_find_package(Qt5Keychain 0.1.0)
macro_log_feature(Qt5Keychain_FOUND "QtKeychain" "Provides support for secure credentials storage" "https://github.com/frankosterfeld/qtkeychain" TRUE "" "")
endif()

if( UNIX AND NOT APPLE )
    macro_optional_find_package(TelepathyQt 0.9.3)
    macro_log_feature(TelepathyQt5_FOUND "Telepathy-Qt" "Telepathy-Qt is a Qt high-level binding for Telepathy, a D-Bus framework for unifying real time communication." FALSE "" "Telepathy-Qt is needed for sharing Jabber/GTalk accounts with Telepathy.\n")
endif()

# we need pthreads too
macro_optional_find_package(Threads)
macro_log_feature(THREADS_FOUND "Threads" "Threading Library" "" TRUE "" "Platform specific library for threading")

### QtSparkle
if( WIN32 )
    macro_optional_find_package(QtSparkle)
    macro_log_feature(QTSPARKLE_FOUND "qtsparkle" "Library for creating auto updaters written in Qt" "https://github.com/davidsansome/qtsparkle" FALSE "" "")
endif( WIN32 )

if( WIN32 OR APPLE )
    macro_optional_find_package(LibsnoreQt5 0.6.0 QUIET)
    macro_log_feature(LibsnoreQt5_FOUND "Libsnore" "Library for notifications" "https://projects.kde.org/projects/playground/libs/snorenotify" FALSE "" "")
endif()

find_package(LIBVLC REQUIRED 2.1.0)
macro_log_feature(LIBVLC_FOUND "LibVLC" "Provides audio output" "https://www.videolan.org/vlc/"  TRUE "" "")

set(QXTWEB_FOUND TRUE)
set(QXTWEB_LIBRARIES qxtweb-standalone)
set(QXTWEB_INCLUDE_DIRS ${THIRDPARTY_DIR}/qxt/qxtweb-standalone/web ${THIRDPARTY_DIR}/qxt/qxtweb-standalone/network ${THIRDPARTY_DIR}/qxt/qxtweb-standalone/core ${CMAKE_CURRENT_BINARY_DIR})

### libportfwd
set(LIBPORTFWD_INCLUDE_DIR ${THIRDPARTY_DIR}/libportfwd/include)
set(LIBPORTFWD_LIBRARY tomahawk_portfwd)
set(LIBPORTFWD_LIBRARIES ${LIBPORTFWD_LIBRARY})
add_subdirectory(${THIRDPARTY_DIR}/libportfwd)

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

SET( CLEAN_C_FLAGS ${CMAKE_C_FLAGS} )

#show dep log
macro_display_feature_log()

SET( INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}" )

# make uninstall support
CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
  IMMEDIATE @ONLY)

IF( ( NOT APPLE ) AND ( NOT SANITIZE_ADDRESS ))
    # Make linking as strict on linux as it is on osx. Then we don't break linking on mac so often
    #
    # On using Address Sanitizer, we cannot link to the ASAN lib, so
    # --no-undefined would break the build.
    SET( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined" )
    SET( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--no-undefined" )
ENDIF()

IF ( SANITIZE_ADDRESS )
    SET( CMAKE_C_FLAGS "${CMAKE_C_FLAGS}   -fsanitize=address -fno-omit-frame-pointer")
    SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}   -fsanitize=address -fno-omit-frame-pointer")
ENDIF()

# Early configure these files as we need them later on
configure_file(TomahawkUse.cmake.in "${PROJECT_BINARY_DIR}/TomahawkUse.cmake" @ONLY)
file(COPY TomahawkAddPlugin.cmake DESTINATION "${PROJECT_BINARY_DIR}")
file(COPY TomahawkAddLibrary.cmake DESTINATION "${PROJECT_BINARY_DIR}")

SET( TOMAHAWK_LIBRARY ${TOMAHAWK_BASE_TARGET_NAME}lib )
SET( TOMAHAWK_LIBRARIES ${TOMAHAWK_LIBRARY} )
SET( TOMAHAWK_WIDGETS_LIBRARIES ${TOMAHAWK_BASE_TARGET_NAME}-widgets )
SET( TOMAHAWK_PLAYDARAPI_LIBRARIES ${TOMAHAWK_BASE_TARGET_NAME}-playdarapi )

ADD_SUBDIRECTORY( thirdparty )
ADD_SUBDIRECTORY( src )
ADD_SUBDIRECTORY( admin )

IF(BUILD_TESTS)
  enable_testing()
  ADD_SUBDIRECTORY( src/tests )
ENDIF()

# Add all targets to the build-tree export set
set(CMAKE_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/Tomahawk" CACHE PATH  "Installation directory for CMake files")
set(CMAKE_INSTALL_FULL_CMAKEDIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_CMAKEDIR}")
export(TARGETS ${TOMAHAWK_LIBRARY}
  FILE "${PROJECT_BINARY_DIR}/TomahawkLibraryDepends.cmake")

# Export the package for use from the build-tree
# (this registers the build-tree with a global CMake-registry)
export(PACKAGE Tomahawk)

# Create a TomahawkBuildTreeSettings.cmake file for the use from the build tree
configure_file(TomahawkBuildTreeSettings.cmake.in "${PROJECT_BINARY_DIR}/TomahawkBuildTreeSettings.cmake" @ONLY)

# Create the TomahawkConfig.cmake and TomahawkConfigVersion files
file(RELATIVE_PATH CONF_REL_INCLUDE_DIR "${CMAKE_INSTALL_FULL_CMAKEDIR}" "${CMAKE_INSTALL_FULL_INCLUDEDIR}")

configure_file(TomahawkConfig.cmake.in "${PROJECT_BINARY_DIR}/TomahawkConfig.cmake" @ONLY)
configure_file(TomahawkConfigVersion.cmake.in "${PROJECT_BINARY_DIR}/TomahawkConfigVersion.cmake" @ONLY)

# Install the cmake files
install(
    FILES
        "${PROJECT_BINARY_DIR}/TomahawkConfig.cmake"
        "${PROJECT_BINARY_DIR}/TomahawkConfigVersion.cmake"
        "${PROJECT_BINARY_DIR}/TomahawkUse.cmake"
        "${PROJECT_BINARY_DIR}/TomahawkAddPlugin.cmake"
        "${PROJECT_BINARY_DIR}/TomahawkAddLibrary.cmake"
    DESTINATION
        "${CMAKE_INSTALL_CMAKEDIR}"
)

# Install the export set for use with the install-tree
install(
    EXPORT
        TomahawkLibraryDepends
    DESTINATION
        "${CMAKE_INSTALL_CMAKEDIR}"
)
