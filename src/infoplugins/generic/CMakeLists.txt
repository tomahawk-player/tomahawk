include_directories(${ECHONEST_INCLUDE_DIR})

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

