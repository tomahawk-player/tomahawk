project( tomahawk )

include( ${QT_USE_FILE} )
add_definitions( ${QT_DEFINITIONS} )
add_definitions( -DQT_PLUGIN )
add_definitions( -DQT_SHARED )
add_definitions( -DSIPDLLEXPORT_PRO )

set( jabberSources
    jabber.cpp
    tomahawksipmessage.cpp
    tomahawksipmessagefactory.cpp
    avatarmanager.cpp
)

set( jabberHeaders
    jabber.h
    tomahawksipmessage.h
    tomahawksipmessagefactory.h
    avatarmanager.h
)

set( jabberUI
     configwidget.ui
)

include_directories( . ${CMAKE_CURRENT_BINARY_DIR} ..
   ${QT_INCLUDE_DIR}
   ${LIBJREEN_INCLUDE_DIR}
)

qt4_add_resources( RC_SRCS "resources.qrc" )
qt4_wrap_ui( jabberUI_H ${jabberUI} )
qt4_wrap_cpp( jabberMoc ${jabberHeaders} )
add_library( tomahawk_sipjabber SHARED ${jabberSources} ${jabberMoc} ${jabberUI_H} ${RC_SRCS} )

IF( WIN32 )
SET( OS_SPECIFIC_LINK_LIBRARIES
    ${OS_SPECIFIC_LINK_LIBRARIES}
    "secur32.dll"
    "crypt32.dll"
    ${TOMAHAWK_LIBRARIES}
)
ENDIF( WIN32 )

target_link_libraries( tomahawk_sipjabber
    ${QT_LIBRARIES}
    ${LIBJREEN_LIBRARY}
    ${OS_SPECIFIC_LINK_LIBRARIES}
    tomahawklib
)

IF( APPLE )
#  SET( CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS} "-undefined dynamic_lookup" )
ENDIF( APPLE )

install( TARGETS tomahawk_sipjabber DESTINATION lib${LIB_SUFFIX} )


add_subdirectory(googlewrapper)
