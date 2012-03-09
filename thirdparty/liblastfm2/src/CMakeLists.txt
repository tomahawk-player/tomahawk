cmake_minimum_required(VERSION 2.6)

# Macro to copy and rename headers
macro(copy_header from to)
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/${from}
    ${CMAKE_CURRENT_BINARY_DIR}/lastfm/${to}
    COPY_ONLY
  )
endmacro(copy_header)

# Copy headers
copy_header(core/misc.h misc.h)
copy_header(core/XmlQuery.h XmlQuery)
copy_header(core/UrlBuilder.h UrlBuilder)
copy_header(global.h global.h)
copy_header(radio/RadioTuner.h RadioTuner)
copy_header(radio/RadioStation.h RadioStation)
copy_header(scrobble/Audioscrobbler.h Audioscrobbler)
copy_header(scrobble/ScrobbleCache.h ScrobbleCache)
copy_header(scrobble/ScrobblePoint.h ScrobblePoint)
copy_header(types/AbstractType.h AbstractType)
copy_header(types/Album.h Album)
copy_header(types/Artist.h Artist)
copy_header(types/FingerprintId.h FingerprintId)
copy_header(types/Mbid.h Mbid)
copy_header(types/Playlist.h Playlist)
copy_header(types/Tag.h Tag)
copy_header(types/Track.h Track)
copy_header(types/User.h User)
copy_header(types/User.h UserList)
copy_header(types/Xspf.h Xspf)
copy_header(ws/ws.h ws.h)
copy_header(ws/InternetConnectionMonitor.h InternetConnectionMonitor)
copy_header(ws/NetworkAccessManager.h NetworkAccessManager)

include_directories(${QT_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR})

set(SOURCES
    scrobble/ScrobbleCache.cpp
    scrobble/Audioscrobbler.cpp
    types/FingerprintId.cpp
    types/Artist.cpp
    types/Tag.cpp
    types/Track.cpp
    types/User.cpp
    types/Xspf.cpp
    types/Album.cpp
    types/Playlist.cpp
    types/Mbid.cpp
    radio/RadioTuner.cpp
    radio/RadioStation.cpp
    core/UrlBuilder.cpp
    core/misc.cpp
    core/XmlQuery.cpp
    ws/NetworkAccessManager.cpp
    ws/ws.cpp
    ws/InternetConnectionMonitor.cpp
    ws/NetworkConnectionMonitor.cpp
)

set(MOC_HEADERS
  scrobble/Audioscrobbler.h
  types/Track.h
  types/Xspf.h
  radio/RadioTuner.h
  ws/NetworkConnectionMonitor.h
  ws/InternetConnectionMonitor.h
  ws/NetworkAccessManager.h
)

if(UNIX)
  if(APPLE)
    set(SOURCES ${SOURCES} ws/mac/MNetworkConnectionMonitor_mac.cpp)
    set(MOC_HEADERS ${MOC_HEADERS} ws/mac/MNetworkConnectionMonitor.h)
  else(APPLE)
    set(SOURCES ${SOURCES} ws/linux/LNetworkConnectionMonitor_linux.cpp)
    set(MOC_HEADERS ${MOC_HEADERS} ws/linux/LNetworkConnectionMonitor.h)
  endif(APPLE)
endif(UNIX)
if(WIN32)
  set(SOURCES ${SOURCES} ws/win/WNetworkConnectionMonitor_win.cpp ws/win/Pac.cpp ws/win/NdisEvents.cpp )
  set(MOC_HEADERS ${MOC_HEADERS} ws/win/WNetworkConnectionMonitor.h)
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../winhttp)
endif(WIN32)

qt4_wrap_cpp(MOC_SOURCES ${MOC_HEADERS})

IF( WIN32 )
    add_definitions( -DLASTFM_LIB )
    add_library(tomahawk_lastfm2 SHARED
    ${SOURCES}
    ${MOC_SOURCES}
    )
ELSE()
    add_definitions(-fPIC)
    add_library(tomahawk_lastfm2 STATIC
    ${SOURCES}
    ${MOC_SOURCES}
    )
ENDIF()

target_link_libraries(tomahawk_lastfm2
  ${QT_LIBRARIES}
  ${QT_QTDBUS_LIBRARY}

)

set_target_properties(tomahawk_lastfm2 PROPERTIES COMPILE_FLAGS "-DLASTFM_OHAI_QMAKE"  )

if(APPLE)
  target_link_libraries(tomahawk_lastfm2
    /System/Library/Frameworks/CoreFoundation.framework
    /System/Library/Frameworks/SystemConfiguration.framework
  )
endif(APPLE)


IF( WIN32 )
    install(TARGETS tomahawk_lastfm2
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib${LIB_SUFFIX}
    ARCHIVE DESTINATION lib${LIB_SUFFIX}
    )
ENDIF()
