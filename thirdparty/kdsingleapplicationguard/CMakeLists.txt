cmake_minimum_required(VERSION 2.6)

cmake_policy(SET CMP0020 OLD)

project(kdsingleapplicationguard)

list(APPEND kdsingleapplicationguard_INCLUDE_DIRECTORIES
    ${QT_INCLUDES}
    ${QT_QTCORE_INCLUDE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

list(APPEND kdsingleapplicationguard_SOURCES
    kdsingleapplicationguard.cpp
    kdsharedmemorylocker.cpp
    kdtoolsglobal.cpp
    kdlockedsharedmemorypointer.cpp
)

include_directories(${kdsingleapplicationguard_INCLUDE_DIRECTORIES})
add_library(kdsingleapplicationguard STATIC ${kdsingleapplicationguard_SOURCES})
set_target_properties(
    kdsingleapplicationguard
        PROPERTIES
            AUTOMOC TRUE
)
target_link_libraries(kdsingleapplicationguard Qt5::Core)
