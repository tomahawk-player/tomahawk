project (QuaZip)
cmake_minimum_required(VERSION 2.6)

set(qt_min_version "4.5.0")
find_package(Qt4 REQUIRED)
set(QT_USE_QTGUI false)
include(${QT_USE_FILE})

ADD_DEFINITIONS(-fPIC)

set(LIB_SUFFIX "" CACHE STRING "Define suffix of directory name (32/64)")
set(LIB_DESTINATION "${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}" CACHE STRING "Library directory name" FORCE)

include_directories(${CMAKE_SOURCE_DIR})
add_subdirectory(quazip)
