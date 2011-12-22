
IF( LIBJREEN_FOUND )
    add_subdirectory( xmpp )
ENDIF()

IF( QTWEETLIB_FOUND AND BUILD_GUI )
    ADD_SUBDIRECTORY( twitter )
ENDIF()

ADD_SUBDIRECTORY( zeroconf )