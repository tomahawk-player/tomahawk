set( tomahawk_test_musicscan_src
    main.cpp
)

add_executable( tomahawk_test_musicscan_bin WIN32 MACOSX_BUNDLE
    ${tomahawk_test_musicscan_src} )
set_target_properties( tomahawk_test_musicscan_bin
    PROPERTIES
        AUTOMOC TRUE
        RUNTIME_OUTPUT_NAME tomahawk-test-musicscan
)
target_link_libraries( tomahawk_test_musicscan_bin
    ${TOMAHAWK_LIBRARIES}
)

qt5_use_modules(tomahawk_test_musicscan_bin Core Gui Network Widgets)
install( TARGETS tomahawk_test_musicscan_bin BUNDLE DESTINATION . RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} )
