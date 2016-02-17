cmake_minimum_required(VERSION 2.8)
CMAKE_POLICY(SET CMP0017 NEW)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/Modules")

if(NOT TOMAHAWK_LIBRARIES)
    message(STATUS "BUILDING OUTSIDE TOMAHAWK")
    find_package(Tomahawk REQUIRED)
else()
    message(STATUS "BUILDING INSIDE TOMAHAWK")
    set(TOMAHAWK_USE_FILE "${PROJECT_BINARY_DIR}/TomahawkUse.cmake")
endif()
include( ${TOMAHAWK_USE_FILE} )

find_package(OpenSSL REQUIRED)
find_package(websocketpp 0.2.99 REQUIRED)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${WEBSOCKETPP_INCLUDE_DIR}
    ${TOMAHAWK_INCLUDE_DIRS}
    ${QCA2_INCLUDE_DIR}
)

add_definitions(-D_WEBSOCKETPP_CPP11_STL_)


tomahawk_add_plugin(hatchet
    TYPE account
    EXPORT_MACRO ACCOUNTDLLEXPORT_PRO
    SOURCES
        account/HatchetAccount.cpp
        account/HatchetAccountConfig.cpp
        sip/WebSocket.cpp
        sip/WebSocketThreadController.cpp
        sip/HatchetSip.cpp
    UI
        account/HatchetAccountConfig.ui
    LINK_LIBRARIES
        ${TOMAHAWK_LIBRARIES}
        ${QCA2_LIBRARIES}
        ${PLATFORM_SPECIFIC_LINK_LIBRARIES}
)

