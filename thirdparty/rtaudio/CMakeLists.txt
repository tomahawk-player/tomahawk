PROJECT(rtaudio)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6 FATAL_ERROR)
SET(CMAKE_VERBOSE_MAKEFILE ON)
SET(CMAKE_INSTALL_PREFIX ".")

#ADD_DEFINITIONS(-Wall -O2 -DNDEBUG)
#ADD_DEFINITIONS(-fPIC)

SET(AUDIO_LIBS "")

if(APPLE)
	ADD_DEFINITIONS(-DHAVE_GETTIMEOFDAY -D__MACOSX_CORE__)
endif(APPLE)

if(WIN32)
	ADD_DEFINITIONS(-D__WINDOWS_DS__)
	INCLUDE_DIRECTORIES( 
	../../dx2010/include 
	# Just copy sal.h from VS\include to directx/include instead of this:
	# "c:\\Program Files\\Microsoft Visual Studio 10.0\\VC\\include"
	)
	SET(AUDIO_LIBS "dsound.dll" "winmm.dll" )
endif(WIN32)

if(UNIX AND NOT APPLE)
	ADD_DEFINITIONS(-DHAVE_GETTIMEOFDAY -D__LINUX_ALSA__)
	SET(AUDIO_LIBS "asound")
endif(UNIX AND NOT APPLE)

if(WIN32)
	ADD_LIBRARY(rtaudio SHARED RtAudio.cpp)
else()
	ADD_LIBRARY(rtaudio STATIC RtAudio.cpp)
endif()

target_link_libraries( rtaudio
	${AUDIO_LIBS}
)

INSTALL(TARGETS rtaudio ARCHIVE DESTINATION lib)
