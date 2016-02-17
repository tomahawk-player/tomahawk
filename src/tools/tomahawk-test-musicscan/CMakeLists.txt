set(TOMAHAWK_TOOL_MUSICSCAN_TARGET ${TOMAHAWK_TARGET_NAME}-musicscan)

set( tomahawk_test_musicscan_src
    main.cpp
)

add_executable( ${TOMAHAWK_TOOL_MUSICSCAN_TARGET} WIN32 MACOSX_BUNDLE
    ${tomahawk_test_musicscan_src} )
set_target_properties( ${TOMAHAWK_TOOL_MUSICSCAN_TARGET}
    PROPERTIES
        AUTOMOC TRUE
)

target_link_libraries( ${TOMAHAWK_TOOL_MUSICSCAN_TARGET}
    ${TOMAHAWK_LIBRARIES}
)

qt5_use_modules(${TOMAHAWK_TOOL_MUSICSCAN_TARGET} Core Gui Network Widgets)
install( TARGETS ${TOMAHAWK_TOOL_MUSICSCAN_TARGET} BUNDLE DESTINATION . RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} )
