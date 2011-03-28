# only build one of them, if ENABLE_JREEN is true, GLOOX_FOUND is automatically set to "false"
IF( GLOOX_FOUND )
    ADD_SUBDIRECTORY( jabber )
ENDIF( GLOOX_FOUND )
IF( ENABLE_JREEN )
    ADD_SUBDIRECTORY( jreen )
ENDIF( ENABLE_JREEN)

ADD_SUBDIRECTORY( twitter )
ADD_SUBDIRECTORY( zeroconf )
