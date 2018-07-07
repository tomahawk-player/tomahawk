#include <QtPlugin>

#if defined(Q_EXPORT_PLUGIN)
    #undef Q_EXPORT_PLUGIN
    #undef Q_EXPORT_PLUGIN2
#endif

#define Q_EXPORT_PLUGIN(a)
#define Q_EXPORT_PLUGIN2(a, b)
