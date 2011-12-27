FILE( GLOB _icons "${CMAKE_SOURCE_DIR}/data/icons/tomahawk-icon-*.png" )
FOREACH( _file ${_icons} )
 STRING( REPLACE "${CMAKE_SOURCE_DIR}/data/icons/tomahawk-icon-" "" _res ${_file} )
 STRING( REPLACE ".png" "" _res ${_res} )
 INSTALL( FILES ${_file} RENAME tomahawk.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/${_res}/apps )
ENDFOREACH( _file )

INSTALL( FILES ${CMAKE_SOURCE_DIR}/data/icons/tomahawk-icon.svg RENAME tomahawk.svg  DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps )

INSTALL( FILES ${CMAKE_SOURCE_DIR}/admin/unix/tomahawk.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications )
