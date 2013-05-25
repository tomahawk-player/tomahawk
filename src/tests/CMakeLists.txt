if( BUILD_TESTS )
setup_qt()

enable_testing()

include_directories(${CMAKE_CURRENT_LIST_DIR}/../src/tomahawk ${CMAKE_CURRENT_LIST_DIR}/../src/libtomahawk)
include(tomahawk_add_test.cmake)

tomahawk_add_test(Result)
tomahawk_add_test(Query)
tomahawk_add_test(Database)

endif()
