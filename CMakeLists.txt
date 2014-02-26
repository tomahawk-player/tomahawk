PROJECT( tomahawk )
CMAKE_MINIMUM_REQUIRED( VERSION 2.8.6 )
SET( CMAKE_MODULE_PATH  "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules" )
CMAKE_POLICY(SET CMP0017 NEW)

IF ( CMAKE_VERSION VERSION_EQUAL 2.8.12 OR CMAKE_VERSION VERSION_GREATER 2.8.12 )
CMAKE_POLICY(SET CMP0022 NEW)

# TODO:
# Update to NEW and fix things up when we can depend on 2.8.12
CMAKE_POLICY(SET CMP0023 OLD)
ENDIF()

INCLUDE(CMakeDependentOption)

###
### Tomahawk application info
###
SET( TOMAHAWK_ORGANIZATION_NAME "Tomahawk" )
SET( TOMAHAWK_ORGANIZATION_DOMAIN "tomahawk-player.org" )
SET( TOMAHAWK_APPLICATION_NAME  "Tomahawk" )
SET( TOMAHAWK_DESCRIPTION_SUMMARY "The social media player" )

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
SET( TOMAHAWK_VERSION_MINOR 7 )
SET( TOMAHAWK_VERSION_PATCH 100 )
#SET( TOMAHAWK_VERSION_RC 0 )

SET( TOMAHAWK_TRANSLATION_LANGUAGES ar bg bn_IN ca cs de en el es fi fr hi_IN hu gl it ja lt pl pt_BR ro ru sv tr zh_CN zh_TW )

# enforce proper symbol exporting on all platforms
add_definitions( "-fvisibility=hidden" )
# enforce using constBegin, constEnd for const-iterators
add_definitions( "-DQT_STRICT_ITERATORS" )

# build options
option(BUILD_GUI "Build Tomahawk with GUI" ON)
option(BUILD_RELEASE "Generate TOMAHAWK_VERSION without GIT info" OFF)
option(BUILD_TESTS "Build Tomahawk with unit tests" ON)
option(BUILD_HATCHET "Build the Hatchet plugin" OFF)
option(BUILD_WITH_QT4 "Build Tomahawk with Qt4 no matter if Qt5 was found" ON)

option(WITH_BREAKPAD "Build with breakpad integration" ON)
option(WITH_CRASHREPORTER "Build with CrashReporter" ON)
option(WITH_BINARY_ATTICA "Enable support for downloading binary resolvers automatically" ON)
option(LEGACY_KDE_INTEGRATION "Install tomahawk.protocol file, deprecated since 4.6.0" OFF)
option(WITH_KDE4 "Build with support for KDE specific stuff" ON)

CMAKE_DEPENDENT_OPTION(WITH_UPOWER "Build with support for UPower events" ON
       "UNIX;NOT APPLE" OFF)
CMAKE_DEPENDENT_OPTION(WITH_GNOMESHORTCUTHANDLER "Build with shortcut handler for GNOME" ON
       "UNIX;NOT APPLE" OFF)

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
SET( TOMAHAWK_VERSION_SHORT "${TOMAHAWK_VERSION}" )
IF( TOMAHAWK_VERSION_RC )
  SET( TOMAHAWK_VERSION ${TOMAHAWK_VERSION}rc${TOMAHAWK_VERSION_RC} )
ENDIF()


# additional info for non-release builds
IF( NOT BUILD_RELEASE AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git/" )
    INCLUDE( CMakeDateStamp )
    SET( TOMAHAWK_VERSION_DATE "${CMAKE_DATESTAMP_YEAR}${CMAKE_DATESTAMP_MONTH}${CMAKE_DATESTAMP_DAY}" )
    IF( TOMAHAWK_VERSION_DATE GREATER 0 )
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

# deps
INCLUDE( MacroOptionalFindPackage )
INCLUDE( MacroLogFeature )

# headless mode
if( NOT BUILD_GUI )
    add_definitions( -DENABLE_HEADLESS )
    message( STATUS "Building Tomahawk ${TOMAHAWK_VERSION} in HEADLESS mode ***" )
else()
    message( STATUS "Building Tomahawk ${TOMAHAWK_VERSION} full GUI version ***" )
endif()

if( NOT BUILD_WITH_QT4 )
    find_package(Qt5Core QUIET)
    if( Qt5Core_DIR )
        find_package(Qt5Widgets QUIET)
        message(STATUS "Found Qt5! Be aware that Qt5-support is still experimental and not officially supported!")

        if( UNIX AND NOT APPLE )
            # We need this to find the paths to qdbusxml2cpp and co
            find_package(Qt5DBus REQUIRED)
        endif()

        macro(qt_wrap_ui)
            qt5_wrap_ui(${ARGN})
        endmacro()

        macro(qt_add_resources)
            qt5_add_resources(${ARGN})
        endmacro()

        find_package(Qt5LinguistTools REQUIRED)
        macro(qt_add_translation)
            qt5_add_translation(${ARGN})
        endmacro()

        macro(qt_add_dbus_interface)
            qt5_add_dbus_interface(${ARGN})
        endmacro()

        macro(qt_add_dbus_adaptor)
            qt5_add_dbus_adaptor(${ARGN})
        endmacro()

        macro(setup_qt)
        endmacro()

        set(QT_RCC_EXECUTABLE "${Qt5Core_RCC_EXECUTABLE}")
        #FIXME: CrashReporter depends on deprecated QHttp
        set(WITH_KDE4 OFF)
    endif()
endif()
if( NOT Qt5Core_DIR )
    message(STATUS "Could not find Qt5, searching for Qt4 instead...")

    set(NEEDED_QT4_COMPONENTS "QtCore" "QtXml" "QtNetwork")
    if( BUILD_GUI )
        list(APPEND NEEDED_QT4_COMPONENTS "QtGui" "QtWebkit" "QtUiTools" "QtSvg")
    endif()
    if( BUILD_TESTS )
        list(APPEND NEEDED_QT4_COMPONENTS "QtTest")
    endif()

    macro_optional_find_package(Qt4 4.7.0 COMPONENTS ${NEEDED_QT4_COMPONENTS} )
    macro_log_feature(QT4_FOUND "Qt" "A cross-platform application and UI framework" "http://qt.nokia.com" TRUE "" "If you see this, although libqt4-devel is installed, check whether the \n     qtwebkit-devel package and whatever contains QtUiTools is installed too")

    macro(qt5_use_modules)
    endmacro()

    macro(qt_wrap_ui)
        qt4_wrap_ui(${ARGN})
    endmacro()

    macro(qt_add_resources)
        qt4_add_resources(${ARGN})
    endmacro()

    macro(qt_add_translation)
        qt4_add_translation(${ARGN})
    endmacro()

    macro(qt_add_dbus_interface)
      qt4_add_dbus_interface(${ARGN})
    endmacro()

    macro(qt_add_dbus_adaptor)
        qt4_add_dbus_adaptor(${ARGN})
    endmacro()

    macro(setup_qt)
        if( NOT BUILD_GUI )
            set(QT_DONT_USE_QTGUI TRUE)
        endif()
        if( UNIX AND NOT APPLE )
            set(QT_USE_QTDBUS TRUE)
        endif()
        set(QT_USE_QTSQL TRUE)
        set(QT_USE_QTNETWORK TRUE)
        set(QT_USE_QTXML TRUE)
        set(QT_USE_QTWEBKIT TRUE)

        include( ${QT_USE_FILE} )
    endmacro()
endif()

if( Qt5Core_DIR )
    set( TOMAHAWK_QT5 TRUE )
else( Qt5Core_DIR )
    set( TOMAHAWK_QT5 FALSE )
endif( Qt5Core_DIR )

if( BUILD_GUI AND UNIX AND NOT APPLE )
    find_package( X11 )
endif()

macro_optional_find_package(Echonest 2.2.0)
macro_log_feature(ECHONEST_FOUND "Echonest" "Qt library for communicating with The Echo Nest" "http://projects.kde.org/libechonest" TRUE "" "libechonest 2.2.0 is needed for dynamic playlists and the infosystem")

macro_optional_find_package(CLucene 0.9.23)
macro_log_feature(CLucene_FOUND "CLucene" "The open-source, C++ search engine" "http://clucene.sf.net" TRUE "" "CLucene is used for indexing the collection")

macro_optional_find_package(QJSON 0.8.1)
macro_log_feature(QJSON_FOUND "QJson" "Qt library that maps JSON data to QVariant objects" "http://qjson.sf.net" TRUE "" "libqjson is used for encoding communication between Tomahawk instances")

macro_optional_find_package(Taglib 1.6.0)
macro_log_feature(TAGLIB_FOUND "TagLib" "Audio Meta-Data Library" "http://developer.kde.org/~wheeler/taglib.html" TRUE "" "taglib is needed for reading meta data from audio files")
include( CheckTagLibFileName )
check_taglib_filename( COMPLEX_TAGLIB_FILENAME )

find_package(Boost REQUIRED)
macro_log_feature(Boost_FOUND "Boost" "Provides free peer-reviewed portable C++ source libraries" "http://www.boost.org" TRUE "" "") #FIXME: give useful explanation

macro_optional_find_package(QCA2)
macro_log_feature(QCA2_FOUND "QCA2" "Provides encryption and signing functions necessary for some resolvers and accounts" "http://delta.affinix.com/qca/" TRUE "" "")

if( TOMAHAWK_QT5 )
    macro_optional_find_package(KF5Attica 1.0.0)
    set(LIBATTICA_FOUND ${KF5Attica_FOUND})
else()
    macro_optional_find_package(LibAttica 0.4.0)
endif()
macro_log_feature(LIBATTICA_FOUND "libattica" "Provides support for installation of resolvers from the Tomahawk website" "https://projects.kde.org/projects/kdesupport/attica" TRUE "" "")

macro_optional_find_package(QuaZip)
macro_log_feature(QuaZip_FOUND "QuaZip" "Provides support for extracting downloaded resolvers automatically." "http://quazip.sourceforge.net/" TRUE "" "")

macro_optional_find_package(Jreen 1.0.5)
macro_log_feature(JREEN_FOUND "Jreen" "Qt XMPP Library" "http://qutim.org/jreen / https://github.com/euroelessar/jreen" FALSE "" "Jreen is needed for the Jabber SIP plugin.\n")
if( PC_JREEN_VERSION STREQUAL "1.1.0" )
    message(FATAL_ERROR "Jreen 1.1.0 has a very annoying bug that breaks accepting auth requests in Tomahawk. Please upgrade to 1.1.1 or downgrade to 1.0.5.")
endif()

macro_optional_find_package(QTweetLib)
macro_log_feature(QTWEETLIB_FOUND "QTweetLib" "Qt Twitter Library" "https://github.com/minimoog/QTweetLib" FALSE "" "QTweetLib is needed for the Twitter SIP plugin.\n")

macro_optional_find_package(LibLastFm 1.0.0)
macro_log_feature(LIBLASTFM_FOUND "liblastfm" "Qt library for the Last.fm webservices" "https://github.com/lastfm/liblastfm" TRUE "" "liblastfm is needed for scrobbling tracks to Last.fm and fetching cover artwork")

if( NOT APPLE )
if( TOMAHAWK_QT5 )
    macro_optional_find_package(Qt5Keychain 0.1.0)
else()
    macro_optional_find_package(QtKeychain 0.1.0)
endif()
macro_log_feature(QTKEYCHAIN_FOUND "QtKeychain" "Provides support for secure credentials storage" "https://github.com/frankosterfeld/qtkeychain" TRUE "" "")
endif()

if( UNIX AND NOT APPLE )
    macro_optional_find_package(TelepathyQt 0.9.3)
    if ( TOMAHAWK_QT5 )
        macro_log_feature(TelepathyQt5_FOUND "Telepathy-Qt" "Telepathy-Qt is a Qt high-level binding for Telepathy, a D-Bus framework for unifying real time communication." FALSE "" "Telepathy-Qt is needed for sharing Jabber/GTalk accounts with Telepathy.\n")
    else ( TOMAHAWK_QT5)
        macro_log_feature(TelepathyQt4_FOUND "Telepathy-Qt" "Telepathy-Qt is a Qt high-level binding for Telepathy, a D-Bus framework for unifying real time communication." FALSE "" "Telepathy-Qt is needed for sharing Jabber/GTalk accounts with Telepathy.\n")
    endif()
endif()

# we need pthreads too
macro_optional_find_package(Threads)
macro_log_feature(THREADS_FOUND "Threads" "Threading Library" "" TRUE "" "Platform specific library for threading")

### QtSparkle
if( WIN32 )
    macro_optional_find_package(QtSparkle)
    macro_log_feature(QTSPARKLE_FOUND "qtsparkle" "Library for creating auto updaters written in Qt" "https://github.com/davidsansome/qtsparkle" FALSE "" "")
endif( WIN32 )

#TODO: support external qxt
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


if (WITH_KDE4)
    macro_optional_find_package(KDE4)
    macro_optional_find_package(KDE4Installed)
endif(WITH_KDE4)
macro_log_feature(KDE4_FOUND "KDE4" "Provides support for configuring Telepathy Accounts from inside Tomahawk" "https://www.kde.org" FALSE "" "")

if(NOT Phonon_FOUND)
  macro_optional_find_package(Phonon 4.5.0)
endif()
macro_log_feature(Phonon_FOUND "Phonon" "The Phonon multimedia library" "http://phonon.kde.org" TRUE "" "")

if(Phonon_FOUND)
    message(STATUS "Phonon found; ensure that phonon-vlc is at least 0.4")
endif()

IF( KDE4_FOUND OR Phonon_FOUND )
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

# KDE4 defines an uninstall target for us automatically (and at least with Qt4 Phonon does as well no matter if kdelibs was found)
# IF( NOT KDE4_FOUND )
#     ADD_CUSTOM_TARGET(uninstall "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")
# ENDIF()

IF( NOT APPLE )
    # Make linking as strict on linux as it is on osx. Then we don't break linking on mac so often
    SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--no-undefined" )
ENDIF( NOT APPLE )

# Early configure these files as we need them later on
configure_file(TomahawkUse.cmake.in "${PROJECT_BINARY_DIR}/TomahawkUse.cmake" @ONLY)
file(COPY TomahawkAddPlugin.cmake DESTINATION "${PROJECT_BINARY_DIR}")
file(COPY TomahawkAddLibrary.cmake DESTINATION "${PROJECT_BINARY_DIR}")

SET( TOMAHAWK_LIBRARIES tomahawklib )
SET( TOMAHAWK_WIDGETS_LIBRARIES tomahawk-widgets )
SET( TOMAHAWK_PLAYDARAPI_LIBRARIES tomahawk-playdarapi )

ADD_SUBDIRECTORY( thirdparty )
ADD_SUBDIRECTORY( src )
ADD_SUBDIRECTORY( admin )

# Add all targets to the build-tree export set
set(CMAKE_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/Tomahawk" CACHE PATH  "Installation directory for CMake files")
set(CMAKE_INSTALL_FULL_CMAKEDIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_CMAKEDIR}")
export(TARGETS tomahawklib
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
