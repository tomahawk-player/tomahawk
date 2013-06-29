IF( WITH_UPOWER )
    SET( tomahawkSources ${tomahawkSources} "${CMAKE_CURRENT_SOURCE_DIR}/UPowerHandler.cpp" PARENT_SCOPE )
ENDIF( WITH_UPOWER )
