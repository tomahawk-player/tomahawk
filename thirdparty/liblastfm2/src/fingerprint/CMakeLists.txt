cmake_minimum_required(VERSION 2.6)

include_directories(${LIBFFTW3_INCLUDE_DIRS})
include_directories(${LIBSAMPLERATE_INCLUDE_DIRS})
include_directories(${QT_INCLUDES})
include_directories(${CMAKE_CURRENT_BINARY_DIR}/..)

link_directories(${LIBFFTW3_LIBRARY_DIRS})
link_directories(${LIBSAMPLERATE_LIBRARY_DIRS})

set(SOURCES
  Collection.cpp
  Fingerprint.cpp
  Sha256.cpp
  fplib/Filter.cpp
  fplib/FingerprintExtractor.cpp
  fplib/OptFFT.cpp
)

add_library(tomahawk_lastfm2_fingerprint SHARED
  ${SOURCES}
)

target_link_libraries(tomahawk_lastfm2_fingerprint
  ${QT_LIBRARIES}
  ${QT_QTSQL_LIBRARY}
  ${LIBFFTW3_LIBRARY}
  ${LIBSAMPLERATE_LIBRARY}
  tomahawk_lastfm2
)

set_target_properties(tomahawk_lastfm2_fingerprint PROPERTIES COMPILE_FLAGS "-DLASTFM_FINGERPRINT_OHAI_QMAKE"  ) 
