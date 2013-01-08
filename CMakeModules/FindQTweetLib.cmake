# - Try to find QTweetLib
#
#  QTWEETLIB_FOUND - system has QTweetLib
#  QTWEETLIB_INCLUDE_DIRS - the QTweetLib include directories
#  QTWEETLIB_LIBRARIES - link these to use QTweetLib
#
# (c) Dominik Schmidt <dev@dominik-schmidt.de>
#

# Include dir
find_path(QTWEETLIB_INCLUDE_DIR
  NAMES QTweetLib/qtweetlib_global.h
  PATHS ${KDE4_INCLUDE_DIR}
)

# Finally the library itself
find_library(QTWEETLIB_LIBRARY
  NAMES QTweetLib
  PATHS ${KDE4_LIB_DIR}
)

SET( QTWEETLIB_LIBRARIES  ${QTWEETLIB_LIBRARY} ${QJSON_LIBRARIES} )

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(QTweetLib DEFAULT_MSG QTWEETLIB_LIBRARY QTWEETLIB_INCLUDE_DIR)

MARK_AS_ADVANCED(QTWEETLIB_LIBRARIES QTWEETLIB_INCLUDE_DIR)

