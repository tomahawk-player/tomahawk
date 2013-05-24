setup_qt()

include_directories( ${CMAKE_CURRENT_BINARY_DIR}/libtomahawk )
include_directories( ${CMAKE_CURRENT_LIST_DIR}/libtomahawk )

file(GLOB SUBDIRECTORIES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*")

foreach(SUBDIRECTORY ${SUBDIRECTORIES})
    if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${SUBDIRECTORY}" AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${SUBDIRECTORY}/CMakeLists.txt")
        message(STATUS "build: " ${SUBDIRECTORY})
        add_subdirectory( ${SUBDIRECTORY} )
    endif()
endforeach()
