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
target_link_libraries(${TOMAHAWK_TOOL_MUSICSCAN_TARGET} Qt5::Core Qt5::Gui Qt5::Network Qt5::Widgets)

install( TARGETS ${TOMAHAWK_TOOL_MUSICSCAN_TARGET} BUNDLE DESTINATION . RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} )
