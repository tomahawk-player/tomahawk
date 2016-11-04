include_directories(
    ${Boost_INCLUDE_DIR}
)
if(WIN32 OR APPLE)
if(BUILD_GUI AND LibsnoreQt5_FOUND)
    SET(snore_srcs
        snorenotify/SnoreNotifyPlugin.cpp
    )
    tomahawk_add_plugin(snorenotify
        TYPE infoplugin EXPORT_MACRO INFOPLUGINDLLEXPORT_PRO
        SOURCES "${snore_srcs}" LINK_LIBRARIES Snore::Libsnore
    )
endif()
endif(WIN32 OR APPLE)

list(APPEND simple_plugins
    Charts
    NewReleases
    #Spotify
    Hypem
    MusixMatch
    MusicBrainz
    #Rovi
    Discogs
)

foreach(simple_plugin ${simple_plugins})
    STRING(TOLOWER "${simple_plugin}" dir)
    tomahawk_add_plugin(${dir}
        TYPE infoplugin EXPORT_MACRO INFOPLUGINDLLEXPORT_PRO
        SOURCES "${dir}/${simple_plugin}Plugin.cpp"
    )
endforeach()

