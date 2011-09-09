file(GLOB SRCS "*.c" "*.cpp")
file(GLOB PUBLIC_HEADERS "*.h")
qt4_wrap_cpp(MOC_SRCS ${PUBLIC_HEADERS})
set(SRCS ${SRCS} ${MOC_SRCS})

add_library(quazip STATIC ${SRCS})
set_target_properties(quazip PROPERTIES VERSION 1 SOVERSION 1.0.0)
target_link_libraries(quazip ${QT_QTMAIN_LIBRARY} ${QT_QTCORE_LIBRARY} -lz)
