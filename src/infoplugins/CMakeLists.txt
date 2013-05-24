include( ${CMAKE_CURRENT_LIST_DIR}/../../TomahawkAddPlugin.cmake )

add_subdirectory( generic )

if(UNIX AND NOT APPLE)
    add_subdirectory( linux )
endif()

if(APPLE)
    add_subdirectory( mac )
endif()
