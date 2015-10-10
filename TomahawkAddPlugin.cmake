include( CMakeParseArguments )
include( ${TOMAHAWK_CMAKE_DIR}/TomahawkAddLibrary.cmake )

function(tomahawk_add_plugin)
    # parse arguments (name needs to be saved before passing ARGN into the macro)
    set(NAME ${ARGV0})
    set(options NO_INSTALL SHARED_LIB)
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
    set(target "${TOMAHAWK_BASE_TARGET_NAME}_${PLUGIN_TYPE}_${PLUGIN_NAME}")

    # create option to disable plugins
    string(TOUPPER "${PLUGIN_TYPE}" PLUGIN_TYPE_UPPER)
    string(TOUPPER "${PLUGIN_NAME}" PLUGIN_NAME_UPPER)
    set(PLUGIN_OPTION "BUILD_${PLUGIN_TYPE_UPPER}_${PLUGIN_NAME_UPPER}")

    if(NOT DEFINED ${PLUGIN_OPTION})
        set(${PLUGIN_OPTION} ON)
    endif()

    option(${PLUGIN_OPTION} "Build Tomahawk with the ${PLUGIN_NAME} ${PLUGIN_TYPE}" ${${PLUGIN_OPTION}})

    if(${PLUGIN_OPTION})
        # determine target type
        if(NOT ${PLUGIN_SHARED_LIB})
            set(target_type "MODULE")
        else()
            set(target_type "SHARED")
        endif()

        list(APPEND tomahawk_add_library_args
        "${target}"
        "EXPORT_MACRO" "${PLUGIN_EXPORT_MACRO}"
        "TARGET_TYPE" "${target_type}"
        "SOURCES" "${PLUGIN_SOURCES}"
        )

        if(PLUGIN_UI)
            list(APPEND tomahawk_add_library_args "UI" "${PLUGIN_UI}")
        endif()

        if(PLUGIN_LINK_LIBRARIES)
            list(APPEND tomahawk_add_library_args "LINK_LIBRARIES" "${PLUGIN_LINK_LIBRARIES}")
        endif()

        if(PLUGIN_COMPILE_DEFINITIONS)
            list(APPEND tomahawk_add_library_args "COMPILE_DEFINITIONS" ${PLUGIN_COMPILE_DEFINITIONS})
        endif()

        list(APPEND tomahawk_add_library_args "NO_VERSION")

        list(APPEND tomahawk_add_library_args "INSTALL_BINDIR" "${CMAKE_INSTALL_LIBDIR}")

        tomahawk_add_library(${tomahawk_add_library_args})
    endif()
endfunction()
