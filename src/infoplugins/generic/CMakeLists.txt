include_directories(
    ${ECHONEST_INCLUDE_DIR}
    ${Boost_INCLUDE_DIR}
)
if(WIN32 OR APPLE)
if(BUILD_GUI AND LIBSNORE_FOUND)
    SET(snore_srcs
        snorenotify/SnoreNotifyPlugin.cpp
    )
    SET(SNORE_LINK_LIBRARIES ${LINK_LIBRARIES} ${LIBSNORE_LIBRARIES} )

    tomahawk_add_plugin(snorenotify
        TYPE infoplugin EXPORT_MACRO INFOPLUGINDLLEXPORT_PRO
        SOURCES "${snore_srcs}" LINK_LIBRARIES "${SNORE_LINK_LIBRARIES}"
    )
endif(BUILD_GUI AND LIBSNORE_FOUND)
endif(WIN32 OR APPLE)

list(APPEND simple_plugins
    Echonest
    Charts
    NewReleases
    Spotify
    Hypem
    MusixMatch
    MusicBrainz
    Rovi
    Discogs
)

foreach(simple_plugin ${simple_plugins})
    STRING(TOLOWER "${simple_plugin}" dir)
    tomahawk_add_plugin(${dir}
        TYPE infoplugin EXPORT_MACRO INFOPLUGINDLLEXPORT_PRO
        SOURCES "${dir}/${simple_plugin}Plugin.cpp"
    )
endforeach()

