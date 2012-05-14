
list(APPEND simple_plugins
    Charts
    NewReleases
    spotify
    hypem
    MusixMatch
    musicbrainz
    Rovi
)

foreach(simple_plugin ${simple_plugins})
    add_tomahawk_plugin(${simple_plugin}
        TYPE infoplugin EXPORT_MACRO INFOPLUGINDLLEXPORT_PRO
        SOURCES "${simple_plugin}Plugin.cpp"
    )
endforeach()


include_directories(${LIBECHONEST_INCLUDE_DIR})
add_tomahawk_plugin(echonest
    TYPE infoplugin EXPORT_MACRO INFOPLUGINDLLEXPORT_PRO
    SOURCES EchonestPlugin.cpp
)

