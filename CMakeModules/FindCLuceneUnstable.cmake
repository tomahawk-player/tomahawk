# - Try to find clucene-unstable
#  This is a workaround for distros, that want to ship a recent enough clucene but don't want to replace the old version
#
#  CLUCENEUNSTABLE_FOUND - system has clucene-unstable
#  CLUCENE_UNSTABLE_INCLUDE_DIR - the clucene-unstable include directories
#  CLUCENE_UNSTABLE_LIBS - link these to use clucene-unstable
#
# (c) Dominik Schmidt <dev@dominik-schmidt.de>
#

# Include dir
find_path(CLUCENE_UNSTABLE_INCLUDE_DIR
  NAMES CLucene.h
  PATH_SUFFIXES clucene-unstable
  PATHS ${KDE4_INCLUDE_DIR}
)

# Finally the library itself
find_library(CLUCENE_UNSTABLE_SHARED_LIB
  NAMES clucene-unstable-shared
  PATHS ${KDE4_LIB_DIR}
)

find_library(CLUCENE_UNSTABLE_CORE_LIB
  NAMES clucene-unstable-core
  PATHS ${KDE4_LIB_DIR}
)


SET( CLUCENE_UNSTABLE_LIBS  ${CLUCENE_UNSTABLE_SHARED_LIB} ${CLUCENE_UNSTABLE_CORE_LIB} )
SET( CLUCENE_UNSTABLE_INCLUDE_DIRS ${CLUCENE_UNSTABLE_INCLUDE_DIR})
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(CLuceneUnstable DEFAULT_MSG CLUCENE_UNSTABLE_LIBS CLUCENE_UNSTABLE_INCLUDE_DIRS)


MARK_AS_ADVANCED(CLUCENE_UNSTABLE_LIBS CLUCENE_UNSTABLE_INCLUDE_DIRS)

