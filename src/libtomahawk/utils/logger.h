#ifndef LOGGER_H
#define LOGGER_H

#include <QDebug>

#include "dllmacro.h"

namespace Logger
{
    class DLLEXPORT tLog
    {
    public:
        tLog& operator<<( const QVariant& v );
        static void log( const char *msg );
    };

    DLLEXPORT void TomahawkLogHandler( QtMsgType type, const char *msg );
    DLLEXPORT void setupLogfile();
}

#define tLog Logger::tLog

#endif