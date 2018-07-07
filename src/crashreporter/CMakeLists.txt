PROJECT( CrashReporter )
cmake_policy(SET CMP0017 NEW)

set(TOMAHAWK_CRASH_REPORTER_TARGET ${TOMAHAWK_BASE_TARGET_NAME}_crash_reporter)

setup_qt()

list(APPEND crashreporter_SOURCES main.cpp)
list(APPEND crashreporter_RC resources.qrc)

qt5_wrap_ui( crashreporter_UI_HEADERS ${crashreporter_UI} )
qt5_add_resources( crashreporter_RC_RCC ${crashreporter_RC} )


if(BUILD_RELEASE)
    set(CRASHREPORTER_RELEASE_CHANNEL "release")
else()
    set(CRASHREPORTER_RELEASE_CHANNEL "nightly")
endif()

set(CRASHREPORTER_SUBMIT_URL "https://crash-reports.tomahawk-player.org/submit")
set(CRASHREPORTER_ICON ":/tomahawk-icon.png")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/CrashReporterConfig.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/CrashReporterConfig.h)


include_directories(${CMAKE_CURRENT_BINARY_DIR}
    ../../libtomahawk
    ../../thirdparty/libcrashreporter-qt/src
)

add_executable( ${TOMAHAWK_CRASH_REPORTER_TARGET} WIN32
    ${crashreporter_SOURCES}
    ${crashreporter_HEADERS_MOC}
    ${crashreporter_UI_HEADERS}
    ${crashreporter_RC_RCC}
)


target_link_libraries( ${TOMAHAWK_CRASH_REPORTER_TARGET}
    ${TOMAHAWK_LIBRARIES}
    crashreporter-gui
    ${QT_LIBRARIES}
)

target_link_libraries(${TOMAHAWK_CRASH_REPORTER_TARGET} Qt5::Widgets Qt5::Network)

set_target_properties(${TOMAHAWK_CRASH_REPORTER_TARGET} PROPERTIES AUTOMOC ON)
install(TARGETS ${TOMAHAWK_CRASH_REPORTER_TARGET} RUNTIME DESTINATION ${CMAKE_INSTALL_LIBEXECDIR})
