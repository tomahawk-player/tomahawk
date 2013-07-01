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
    set(target "tomahawk_${PLUGIN_TYPE}_${PLUGIN_NAME}")

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

    list(APPEND tomahawk_add_library_args "NO_VERSION")

    tomahawk_add_library(${tomahawk_add_library_args})
endfunction()
