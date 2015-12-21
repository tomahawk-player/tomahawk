if(NOT WIN32)
    add_definitions(-fPIC)
endif()

add_definitions( -DPORTFWDDLLEXPORT_STATIC -DSTATICLIB )
if (APPLE)
  add_definitions( -DMACOSX -D_DARWIN_C_SOURCE )
endif ()

add_library(tomahawk_portfwd STATIC
    src/portfwd.cpp
)

target_include_directories(tomahawk_portfwd
    PRIVATE
        ${LIBMINIUPNPC_INCLUDE_DIRS}
)

target_link_libraries(tomahawk_portfwd
    PRIVATE
        ${LIBMINIUPNPC_LIBRARIES}
)

if(WIN32)
    target_link_libraries(tomahawk_portfwd
        PRIVATE
            "ws2_32.dll"
            "iphlpapi.a"
    )
endif()
