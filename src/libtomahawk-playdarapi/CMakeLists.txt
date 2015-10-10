
set(TOMAHAWK_PLAYDARAPI_LIBRARY_TARGET ${TOMAHAWK_BASE_TARGET_NAME}-playdarapi)

list(APPEND ${TOMAHAWK_PLAYDARAPI_LIBRARY_TARGET}_SOURCES
    Api_v1.cpp
    Api_v1_5.cpp
    PlaydarApi.cpp
    StatResponseHandler.cpp
    )

list(APPEND ${TOMAHAWK_PLAYDARAPI_LIBRARY_TARGET}_UI
    )

include_directories(
    ${QXTWEB_INCLUDE_DIRS}
    ${THIRDPARTY_DIR}/qt-certificate-addon/src/
    )

tomahawk_add_library(${TOMAHAWK_PLAYDARAPI_LIBRARY_TARGET}
    SOURCES ${${TOMAHAWK_PLAYDARAPI_LIBRARY_TARGET}_SOURCES}
    UI ${${TOMAHAWK_PLAYDARAPI_LIBRARY_TARGET}_UI}
    LINK_PRIVATE_LIBRARIES
        ${QXTWEB_LIBRARIES}
        qtcertificateaddon
        ${GNUTLS_LIBRARIES}
    EXPORT TomahawkLibraryDepends
    VERSION ${TOMAHAWK_VERSION_SHORT}
    )

