add_subdirectory( generic )

if(UNIX AND NOT APPLE)
    add_subdirectory( linux )
endif()

if(APPLE)
    add_subdirectory( mac )
endif()
