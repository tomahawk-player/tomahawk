include_directories(${CMAKE_CURRENT_LIST_DIR}/../tomahawk ${CMAKE_CURRENT_LIST_DIR}/../libtomahawk)
include(tomahawk_add_test.cmake)

tomahawk_add_test(Result)
tomahawk_add_test(Query)
tomahawk_add_test(Database)
tomahawk_add_test(Servent)
