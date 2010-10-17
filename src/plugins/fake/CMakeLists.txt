project( tomahawk )
cmake_minimum_required(VERSION 2.6)
find_package( Qt4 REQUIRED )

include( ${QT_USE_FILE} )


SET(TOMAHAWK_INC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../include/")

SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${TOMAHAWK_INC_DIR}/..")

set(cpps
    fakeplugin.cpp
    fakecollection.cpp
)

set(hs
fakeplugin.h
fakecollection.h
)

include_directories(    
    . 
    ..
    ${TOMAHAWK_INC_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${QT_INCLUDE_DIR}
)

qt4_wrap_cpp( mocs ${hs} )

ADD_DEFINITIONS(${QT_DEFINITIONS})
ADD_DEFINITIONS(-DQT_PLUGIN)
#ADD_DEFINITIONS(-DQT_NO_DEBUG)
ADD_DEFINITIONS(-DQT_SHARED)

add_library(fake SHARED
    ${cpps}
    ${mocs}
)

target_link_libraries(fake
	${QT_LIBRARIES}
    ${QT_QTSQL_LIBRARIES}
)
