

SET( touchmahawkSources ${final_src} )
#LIST( REMOVE_ITEM touchmahawkSources "main.cpp"  )

SET( touchmahawkHeaders
    active/tomahawktouchwindow.h
)

SET( touchmahawkSources
    ${touchmahawkSources}
    active/tomahawktouchwindow.cpp

#   active/main.cpp
)


qt4_wrap_cpp( touchmahawkMoc ${touchmahawkHeaders} )

SET( touchmahawkFinalSources ${touchmahawkMoc} ${touchmahawkSources} )
SET( touchmahawkLinkLibraries
  kdeclarative
  ${tomahawkLinkLibraries}
) #${QT_QTDECLARATIVE_LIBRARY}

ADD_EXECUTABLE( active-tomahawk ${touchmahawkFinalSources} )
TARGET_LINK_LIBRARIES( active-tomahawk ${touchmahawkLinkLibraries} )
SET_TARGET_PROPERTIES( active-tomahawk PROPERTIES COMPILE_FLAGS -DTOUCHMAHAWK )
INSTALL( TARGETS active-tomahawk DESTINATION bin )

