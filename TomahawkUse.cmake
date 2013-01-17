#FIXME: this only handles qt4 and duplicates top level cmakelists: how can we reduce code duplication?

find_package(Qt4 COMPONENTS QtNetwork QtCore QtGui QtSql REQUIRED)
include( ${QT_USE_FILE} )

set(NEEDED_QT4_COMPONENTS "QtCore" "QtXml" "QtNetwork")
if(BUILD_GUI)
    list(APPEND NEEDED_QT4_COMPONENTS "QtGui" "QtWebkit" "QtUiTools" "QtSvg")
endif()

find_package(Qt4 4.7.0 COMPONENTS ${NEEDED_QT4_COMPONENTS})
include( ${QT_USE_FILE} )

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


if(NOT TOMAHAWK_CMAKE_DIR)
    set(TOMAHAWK_CMAKE_DIR ${CMAKE_CURRENT_LIST_DIR})
endif()

include( "${TOMAHAWK_CMAKE_DIR}/TomahawkAddPlugin.cmake" )
