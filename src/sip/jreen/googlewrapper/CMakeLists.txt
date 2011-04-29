
# fake google plugin

set( googleHeaders
     ../jabber.h
     ../tomahawksipmessage.h
     ../tomahawksipmessagefactory.h
     ../avatarmanager.h
     googlewrapper.h )

set( googleSources
     ../jabber.cpp
     ../jabber_p.cpp
     ../tomahawksipmessage.cpp
     ../tomahawksipmessagefactory.cpp
     ../avatarmanager.cpp
     googlewrapper.cpp )

add_definitions(-DGOOGLE_WRAPPER)

qt4_add_resources( RCX_SRCS "resources.qrc" )

qt4_wrap_cpp( googleMoc googlewrapper.h )
add_library( tomahawk_sipgoogle SHARED ${googleSources} ${googleMoc} ${jabberMoc} ${RCX_SRCS} )

target_link_libraries( tomahawk_sipgoogle
    ${QT_LIBRARIES}
    ${LIBJREEN_LIBRARY}
    ${OS_SPECIFIC_LINK_LIBRARIES}
    tomahawklib
)

install( TARGETS tomahawk_sipgoogle DESTINATION lib${LIB_SUFFIX} )
