macro(tomahawk_add_test test_class)
    include_directories(${QT_INCLUDES} "${PROJECT_SOURCE_DIR}/src" ${CMAKE_CURRENT_BINARY_DIR})

    set(TOMAHAWK_TEST_CLASS ${test_class})
    set(TOMAHAWK_TEST_TARGET ${TOMAHAWK_TEST_CLASS}Test)
    configure_file(main.cpp.in Test${TOMAHAWK_TEST_CLASS}.cpp)
    configure_file(Test${TOMAHAWK_TEST_CLASS}.h Test${TOMAHAWK_TEST_CLASS}.h)

    add_executable(${TOMAHAWK_TEST_CLASS}Test Test${TOMAHAWK_TEST_CLASS}.cpp)

    set_target_properties(${TOMAHAWK_TEST_TARGET} PROPERTIES AUTOMOC ON)

    target_link_libraries(${TOMAHAWK_TEST_TARGET}
        ${TOMAHAWK_LIBRARIES}
        ${QT_QTTEST_LIBRARY}
        ${QT_QTCORE_LIBRARY}
    )

    add_test(NAME ${TOMAHAWK_TEST_TARGET} COMMAND ${TOMAHAWK_TEST_TARGET})
endmacro()