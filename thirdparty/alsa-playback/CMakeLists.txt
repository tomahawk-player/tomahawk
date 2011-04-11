PROJECT(alsaplayback)

find_package( Qt4 REQUIRED )

include( ${QT_USE_FILE} )

CMAKE_MINIMUM_REQUIRED(VERSION 2.6 FATAL_ERROR)
SET(CMAKE_VERBOSE_MAKEFILE ON)
#SET(CMAKE_INSTALL_PREFIX ".")

SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

SET( CMAKE_C_FLAGS ${CLEAN_C_FLAGS} )

#ADD_DEFINITIONS(-Wall -O2 -DNDEBUG)
ADD_DEFINITIONS(-fPIC)

SET(AUDIO_LIBS "")

if(UNIX AND NOT APPLE)
	SET(AUDIO_LIBS "asound")
endif(UNIX AND NOT APPLE)

set( alsaplaybackSources
	alsaplayback.cpp
	alsaaudio.cpp
	xconvert.c
)

set( alsaplaybackHeaders
	alsaplayback.h
)

qt4_wrap_cpp( alsaplaybackMoc ${alsaplaybackHeaders} )
SET(final_src ${alsaplaybackMoc} ${alsaplaybackSources} ${alsaplaybackHeaders})

ADD_LIBRARY(alsaplayback STATIC ${final_src})

target_link_libraries( alsaplayback
	${QT_LIBRARIES}
	${AUDIO_LIBS}
)

#INSTALL(TARGETS alsaplayback ARCHIVE DESTINATION lib${LIB_SUFFIX})
