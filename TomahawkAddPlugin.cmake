include( CMakeParseArguments )


function(tomahawk_add_plugin)
    # parse arguments (name needs to be saved before passing ARGN into the macro)
    set(NAME ${ARGV0})
    set(options NO_INSTALL)
    set(oneValueArgs NAME TYPE EXPORT_MACRO)
    set(multiValueArgs SOURCES UI LINK_LIBRARIES COMPILE_DEFINITIONS)
    cmake_parse_arguments(PLUGIN "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    set(PLUGIN_NAME ${NAME})

#     message("*** Arguments for ${PLUGIN_NAME}")
#     message("Sources: ${PLUGIN_SOURCES}")
#     message("Link libraries: ${PLUGIN_LINK_LIBRARIES}")
#     message("UI: ${PLUGIN_UI}")
#     message("TYPE: ${PLUGIN_TYPE}")
#     message("EXPORT_MACRO: ${PLUGIN_EXPORT_MACRO}")
#     message("NO_INSTALL: ${PLUGIN_NO_INSTALL}")

    # create target name once for convenience
    set(target "tomahawk_${PLUGIN_TYPE}_${PLUGIN_NAME}")

    # qt stuff
    include_directories(${CMAKE_CURRENT_BINARY_DIR})
    if(PLUGIN_UI)
        qt_wrap_ui(PLUGIN_UI_SOURCES ${PLUGIN_UI})
        list(APPEND PLUGIN_SOURCES ${PLUGIN_UI_SOURCES})
    endif()

    if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/resources.qrc")
        qt_add_resources(PLUGIN_RC_SOURCES "resources.qrc")
        list(APPEND PLUGIN_SOURCES ${PLUGIN_RC_SOURCES})
        unset(PLUGIN_RC_SOURCES)
    endif()

    # add target
    if(NOT ${PLUGIN_SHARED_LIB})
        add_library(${target} MODULE ${PLUGIN_SOURCES})
    else()
        add_library(${target} SHARED ${PLUGIN_SOURCES})
    endif()

    # add qt modules
    qt5_use_modules(${target} Core Network Widgets Sql Xml DBus)

    # definitions - can this be moved into set_target_properties below?
    add_definitions(${QT_DEFINITIONS})
    set_target_properties(${target} PROPERTIES AUTOMOC TRUE COMPILE_DEFINITIONS ${PLUGIN_EXPORT_MACRO})
    if(PLUGIN_COMPILE_DEFINITIONS)
        # Dear CMake, i hate you! Sincerely, domme
        # At least in CMake 2.8.8, you CANNOT set more than one COMPILE_DEFINITIONS value
        # only takes the first one if called multiple times or bails out with wrong number of arguments
        # when passing in a list, thus i redefine the export macro here in hope it won't mess up other targets
        add_definitions( "-D${PLUGIN_EXPORT_MACRO}" )

        set_target_properties(${target} PROPERTIES COMPILE_DEFINITIONS ${PLUGIN_COMPILE_DEFINITIONS})
    endif()

    # add link targets
    target_link_libraries(${target} ${TOMAHAWK_LIBRARIES})
    if(PLUGIN_LINK_LIBRARIES)
        target_link_libraries(${target} ${PLUGIN_LINK_LIBRARIES})
    endif()

    # make installation optional, maybe useful for dummy plugins one day
    if(NOT PLUGIN_NO_INSTALL)
        include(GNUInstallDirs)
        install(TARGETS ${target} DESTINATION ${CMAKE_INSTALL_LIBDIR})
    endif()
endfunction()
