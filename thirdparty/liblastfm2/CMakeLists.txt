cmake_minimum_required(VERSION 2.6)

set( CMAKE_MODULE_PATH
  ${CMAKE_MODULE_PATH}
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules
  )

if(${CMAKE_BUILD_TYPE} MATCHES "Release")
  add_definitions(-DNDEBUG)
endif(${CMAKE_BUILD_TYPE} MATCHES "Release")

# Set up definitions and paths
add_definitions(${QT_DEFINITIONS})
include(${QT_USE_FILE})

# Main Library
add_subdirectory(src)

# Optionally build the fingerprint library
option(BUILD_FINGERPRINT "Build the lastfm-fingerprint library" ON)
find_package(LibSamplerate)
find_package(LibFFTW3)

if (BUILD_FINGERPRINT AND LIBFFTW3_FOUND AND LIBSAMPLERATE_FOUND)
  add_subdirectory(src/fingerprint)
endif (BUILD_FINGERPRINT AND LIBFFTW3_FOUND AND LIBSAMPLERATE_FOUND)

