#ifndef LOGGER_H
#define LOGGER_H

#include <QDebug>

#include "dllmacro.h"

#define tLog( PARAM ) qDebug( PARAM )

// Disable qDebug in release builds.
#ifdef QT_NO_DEBUG
#define qDebug QT_NO_QDEBUG_MACRO
#endif

namespace Logger
{
    DLLEXPORT void TomahawkLogHandler( QtMsgType type, const char *msg );
    DLLEXPORT void setupLogfile();
}

#endif