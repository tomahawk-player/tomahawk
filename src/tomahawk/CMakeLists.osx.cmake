SET( OS_SPECIFIC_LINK_LIBRARIES
    ${OS_SPECIFIC_LINK_LIBRARIES}
    ${COREAUDIO_LIBRARY}
    ${COREFOUNDATION_LIBRARY}

    crypto
    SPMediaKeyTap

    /System/Library/Frameworks/AppKit.framework
    /System/Library/Frameworks/Carbon.framework
    /System/Library/Frameworks/DiskArbitration.framework
    /System/Library/Frameworks/Foundation.framework
    /System/Library/Frameworks/IOKit.framework
)


if( APPLE )
#  find_library(GROWL Growl)
  option(ENABLE_SPARKLE "Sparkle updating" ON)
  find_library(SPARKLE Sparkle)
  if( ENABLE_SPARKLE AND SPARKLE )
    set(HAVE_SPARKLE ON)
    set( OS_SPECIFIC_LINK_LIBRARIES ${OS_SPECIFIC_LINK_LIBRARIES} ${SPARKLE} )
  endif( ENABLE_SPARKLE AND SPARKLE )
  # Uses Darwin kernel version.
  # 9.8.0  -> 10.5/Leopard
  # 10.4.0 -> 10.6/Snow Leopard
  # 11.x.x -> Lion
  # 12.x.x -> Mountain Lion
  string(REGEX MATCH "[0-9]+" DARWIN_VERSION ${CMAKE_HOST_SYSTEM_VERSION})
  if (DARWIN_VERSION GREATER 11)
    SET(MOUNTAIN_LION 1)
  elseif (DARWIN_VERSION GREATER 10)
    SET(LION 1)
  elseif (DARWIN_VERSION GREATER 9)
    SET(SNOW_LEOPARD 1)
  elseif (DARWIN_VERSION GREATER 8)
    SET(LEOPARD 1)
  endif (DARWIN_VERSION GREATER 11)

# Use two different sparkle update tracks for debug and release
# We have to change the URL in the Info.plist file :-/
  set(TOMAHAWK_SPARKLE_UPDATE_URL "http://download.tomahawk-player.org/sparkle/update.php")

# Disable non-release sparkle for now. We haven't used it yet.
#  IF( NOT CMAKE_BUILD_TYPE STREQUAL "Release" )
#      set(TOMAHAWK_SPARKLE_UPDATE_URL "http://download.tomahawk-player.org/sparkle-debug")
#  ENDIF()

  configure_file(${CMAKE_SOURCE_DIR}/admin/mac/Info.plist ${CMAKE_BINARY_DIR}/Info.plist)


  FILE(COPY ${CMAKE_SOURCE_DIR}/admin/mac/sparkle_pub.pem
    DESTINATION "${CMAKE_BINARY_DIR}/${TOMAHAWK_APPLICATION_NAME}.app/Contents/Resources")

  FILE(COPY /usr/bin/SetFile DESTINATION "${CMAKE_BINARY_DIR}/${TOMAHAWK_APPLICATION_NAME}.app/Contents/MacOS")
  FILE(COPY /usr/bin/GetFileInfo DESTINATION "${CMAKE_BINARY_DIR}/${TOMAHAWK_APPLICATION_NAME}.app/Contents/MacOS")


endif (APPLE)
