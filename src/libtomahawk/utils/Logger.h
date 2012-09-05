/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Tomahawk is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TOMAHAWK_LOGGER_H
#define TOMAHAWK_LOGGER_H

#include <QDebug>

#include "DllMacro.h"

#define LOGDEBUG 1
#define LOGINFO 2
#define LOGEXTRA 5
#define LOGVERBOSE 8
#define LOGTHIRDPARTY 9
#define LOGSQL 10

namespace Logger
{
    class DLLEXPORT TLog : public QDebug
    {
    public:
        TLog( unsigned int debugLevel = 0 );
        virtual ~TLog();

    private:
        QString m_msg;
        unsigned int m_debugLevel;
    };

    class DLLEXPORT TDebug : public TLog
    {
    public:
        TDebug( unsigned int debugLevel = LOGDEBUG ) : TLog( debugLevel )
        {
        }
    };

    class DLLEXPORT TSqlLog : public TLog
    {
    public:
        TSqlLog() : TLog( LOGSQL )
        {
        }
    };

    DLLEXPORT void TomahawkLogHandler( QtMsgType type, const char* msg );
    DLLEXPORT void setupLogfile();
    DLLEXPORT QString logFile();
}

#define tLog Logger::TLog
#define tDebug Logger::TDebug
#define tSqlLog Logger::TSqlLog

#endif // TOMAHAWK_LOGGER_H
