include_directories(${LIBATTICA_INCLUDE_DIR})

set(adium_srcs
    adium/AdiumPlugin.cpp
    adium/Adium.mm
)


tomahawk_add_plugin(adium
    TYPE infoplugin EXPORT_MACRO INFOPLUGINDLLEXPORT_PRO
    SOURCES "${adium_srcs}"
)

