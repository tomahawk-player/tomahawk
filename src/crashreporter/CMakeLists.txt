PROJECT( CrashReporter )
cmake_policy(SET CMP0017 NEW)

setup_qt()

list(APPEND crashreporter_SOURCES main.cpp)
list(APPEND crashreporter_RC resources.qrc)

qt_wrap_ui( crashreporter_UI_HEADERS ${crashreporter_UI} )
qt_add_resources( crashreporter_RC_RCC ${crashreporter_RC} )


if(BUILD_RELEASE)
    set(CRASHREPORTER_RELEASE_CHANNEL "release")
else()
    set(CRASHREPORTER_RELEASE_CHANNEL "nightly")
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/CrashReporterConfig.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/CrashReporterConfig.h)


include_directories( ${CMAKE_CURRENT_BINARY_DIR}
    ../../libtomahawk
    ../../thirdparty/libcrashreporter-qt/src
)

add_executable( tomahawk_crash_reporter WIN32
    ${crashreporter_SOURCES}
    ${crashreporter_HEADERS_MOC}
    ${crashreporter_UI_HEADERS}
    ${crashreporter_RC_RCC}
)


target_link_libraries(tomahawk_crash_reporter
    tomahawklib
    crashreporter-gui
    ${QT_LIBRARIES}
)

set_target_properties(tomahawk_crash_reporter PROPERTIES AUTOMOC ON)
install(TARGETS tomahawk_crash_reporter RUNTIME DESTINATION ${CMAKE_INSTALL_LIBEXECDIR})

qt5_use_modules(tomahawk_crash_reporter Widgets Network)
