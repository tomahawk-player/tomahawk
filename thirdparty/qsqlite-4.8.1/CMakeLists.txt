cmake_minimum_required(VERSION 2.6)

FIND_PACKAGE( Qt4 4.8.1 COMPONENTS QtCore QtSql REQUIRED )
set(QT_USE_QTSQL TRUE)
include( ${QT_USE_FILE} )

# Source files
set(THSQLITE-SOURCES
    qsql_thsqlite.cpp
    smain.cpp
)

# Header files that have Q_OBJECT in
set(THSQLITE-MOC-HEADERS
    qsql_thsqlite.h
)

qt4_wrap_cpp(THSQLITE-SOURCES-MOC ${THSQLITE-MOC-HEADERS})

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${QT_INCLUDES})

add_library(tomahawk_qsqlite SHARED
  ${THSQLITE-SOURCES}
  ${THSQLITE-SOURCES-MOC}
)
target_link_libraries(tomahawk_qsqlite
    sqlite3
    ${QT_LIBRARIES}
)

INSTALL( TARGETS tomahawk_qsqlite
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    BUNDLE DESTINATION library
)

