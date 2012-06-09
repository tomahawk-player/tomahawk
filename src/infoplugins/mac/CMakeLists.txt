set(adium_srcs
    adium/AdiumPlugin.cpp
    adium/Adium.mm
)


add_tomahawk_plugin(adium
    TYPE infoplugin EXPORT_MACRO INFOPLUGINDLLEXPORT_PRO
    SOURCES "${adium_srcs}"
)

