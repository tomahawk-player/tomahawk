set( tomahawk_db_list_artists_src
    listartists.cpp
)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable( tomahawk_db_list_artists_bin WIN32 MACOSX_BUNDLE
    ${tomahawk_db_list_artists_src} )
set_target_properties( tomahawk_db_list_artists_bin
    PROPERTIES
        AUTOMOC TRUE
        RUNTIME_OUTPUT_NAME tomahawk-db-list-artists
)
target_link_libraries( tomahawk_db_list_artists_bin
    ${TOMAHAWK_LIBRARIES}
)

qt5_use_modules(tomahawk_db_list_artists_bin Core)
install( TARGETS tomahawk_db_list_artists_bin BUNDLE DESTINATION . RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} )

set( tomahawk_db_fuzzysearch_src
    fuzzysearch.cpp
)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable( tomahawk_db_fuzzysearch_bin WIN32 MACOSX_BUNDLE
    ${tomahawk_db_fuzzysearch_src} )
set_target_properties( tomahawk_db_fuzzysearch_bin
    PROPERTIES
        AUTOMOC TRUE
        RUNTIME_OUTPUT_NAME tomahawk-db-fuzzysearch
)
target_link_libraries( tomahawk_db_fuzzysearch_bin
    ${TOMAHAWK_LIBRARIES}
)

qt5_use_modules(tomahawk_db_list_artists_bin Core)
install( TARGETS tomahawk_db_list_artists_bin BUNDLE DESTINATION . RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} )

