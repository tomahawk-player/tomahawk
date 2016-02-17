set(TOMAHAWK_TOOL_DB_LIST_ARTISTS_TARGET ${TOMAHAWK_TARGET_NAME}-db-list-artists)

set( tomahawk_db_list_artists_src
    listartists.cpp
)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable( ${TOMAHAWK_TOOL_DB_LIST_ARTISTS_TARGET} WIN32 MACOSX_BUNDLE
    ${tomahawk_db_list_artists_src} )
set_target_properties( ${TOMAHAWK_TOOL_DB_LIST_ARTISTS_TARGET}
    PROPERTIES
        AUTOMOC TRUE
)
target_link_libraries( ${TOMAHAWK_TOOL_DB_LIST_ARTISTS_TARGET}
    ${TOMAHAWK_LIBRARIES}
)

qt5_use_modules(${TOMAHAWK_TOOL_DB_LIST_ARTISTS_TARGET} Core)
install( TARGETS ${TOMAHAWK_TOOL_DB_LIST_ARTISTS_TARGET} BUNDLE DESTINATION . RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} )

