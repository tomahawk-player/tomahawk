#include <QtPlugin>

#if QT_VERSION >= QT_VERSION_CHECK( 5, 0, 0 )
    #if defined(Q_EXPORT_PLUGIN)
        #undef Q_EXPORT_PLUGIN
        #undef Q_EXPORT_PLUGIN2
    #endif

    #define Q_EXPORT_PLUGIN(a)
    #define Q_EXPORT_PLUGIN2(a, b)
#else
    # define Q_PLUGIN_METADATA(a)
#endif