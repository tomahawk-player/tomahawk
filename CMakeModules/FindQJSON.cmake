# Find QJSON - JSON handling library for Qt
#
# This module defines
#  QJSON_FOUND - whether the qsjon library was found
#  QJSON_LIBRARIES - the qjson library
#  QJSON_INCLUDE_DIR - the include path of the qjson library
#

find_library (QJSON_LIBRARIES
    NAMES
    qjson
    PATHS
    ${QJSON_LIBRARY_DIRS}
    ${LIB_INSTALL_DIR}
    ${KDE4_LIB_DIR}
)

find_path (QJSON_INCLUDE_DIR
    NAMES
    qjson/parser.h
    PATHS
    ${QJSON_INCLUDE_DIRS}
    ${INCLUDE_INSTALL_DIR}
    ${KDE4_INCLUDE_DIR}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(QJSON DEFAULT_MSG QJSON_LIBRARIES QJSON_INCLUDE_DIR)

