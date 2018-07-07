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

#include "Logger.h"

#include <iostream>
#include <fstream>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QMutex>
#include <QTime>
#include <QVariant>

#include "utils/TomahawkUtils.h"

#define LOGFILE_SIZE 1024 * 256

#define RELEASE_LEVEL_THRESHOLD 0
#define DEBUG_LEVEL_THRESHOLD LOGEXTRA
#define LOG_SQL_QUERIES 1

using namespace std;

ofstream logStream;
static int s_threshold = -1;
QMutex s_mutex;
bool shutdownInProgress = false;


namespace Logger
{

static void
log( const char *msg, unsigned int debugLevel, bool toDisk = true )
{
    if ( s_threshold < 0 )
    {
        if ( qApp->arguments().contains( "--verbose" ) )
            s_threshold = LOGTHIRDPARTY;
        else
            #ifdef QT_NO_DEBUG
            s_threshold = RELEASE_LEVEL_THRESHOLD;
            #else
            s_threshold = DEBUG_LEVEL_THRESHOLD;
            #endif
    }

    if ( debugLevel > LOGTHIRDPARTY )
        toDisk = false;

    #ifdef LOG_SQL_QUERIES
    if ( debugLevel == LOGSQL )
        toDisk = true;
    #endif

    if ( toDisk || (int)debugLevel <= s_threshold )
    {
        QMutexLocker lock( &s_mutex );

        #ifdef LOG_SQL_QUERIES
        if ( debugLevel == LOGSQL )
            logStream << "TSQLQUERY: ";
        #endif

        if ( shutdownInProgress )
        {
            // Do not use locales anymore in shutdown
            logStream << QDate::currentDate().day() << "."
                      << QDate::currentDate().month() << "."
                      << QDate::currentDate().year() << " - "
                      << QTime::currentTime().hour() << ":"
                      << QTime::currentTime().minute() << ":"
                      << QTime::currentTime().second()
                      << " [" << QString::number( debugLevel ).toUtf8().data() << "]: "
                      << msg << endl;
        }
        else
        {
            logStream << QDate::currentDate().toString().toUtf8().data()
                      << " - "
                      << QTime::currentTime().toString().toUtf8().data()
                      << " [" << QString::number( debugLevel ).toUtf8().data() << "]: "
                      << msg << endl;
        }

        logStream.flush();
    }

    if ( debugLevel <= LOGEXTRA || (int)debugLevel <= s_threshold )
    {
        QMutexLocker lock( &s_mutex );

        if ( shutdownInProgress )
        {
            wcout << QTime::currentTime().hour() << ":"
                  << QTime::currentTime().minute() << ":"
                  << QTime::currentTime().second()
                  << " [" << QString::number( debugLevel ).toStdWString().c_str() << "]: "
                  << msg << endl;
        }
        else
        {
            wcout << QTime::currentTime().toString().toUtf8().data()
                  << " [" << QString::number( debugLevel ).toStdWString().c_str() << "]: "
                  << msg << endl;
        }

        wcout.flush();
    }
}


void
TomahawkLogHandler( QtMsgType type, const QMessageLogContext& context, const QString& msg )
{
    static QMutex s_mutex;

    QByteArray ba = msg.toUtf8();
    const char* message = ba.constData();

    QMutexLocker locker( &s_mutex );
    switch( type )
    {
        case QtDebugMsg:
            log( message, LOGTHIRDPARTY );
            break;

        case QtCriticalMsg:
            log( message, 0 );
            break;

        case QtWarningMsg:
            log( message, 0 );
            break;

        case QtFatalMsg:
            log( message, 0 );
            break;
    }
}



void
setupLogfile( QFile& f )
{
    if ( QFileInfo( f ).size() > LOGFILE_SIZE )
    {
        QByteArray lc;
        {
            f.open( QIODevice::ReadOnly | QIODevice::Text );
            f.seek( f.size() - ( LOGFILE_SIZE - ( LOGFILE_SIZE / 4 ) ) );
            lc = f.readAll();
            f.close();
        }

        f.remove();

        {
            f.open( QIODevice::WriteOnly | QIODevice::Text );
            f.write( lc );
            f.close();
        }
    }

#ifdef OFSTREAM_CAN_OPEN_WCHAR_FILE_NAMES
    // this is not supported in upstream libstdc++ as shipped with GCC
    // GCC needs the patch from https://gcc.gnu.org/ml/libstdc++/2011-06/msg00066.html applied
    logStream.open( f.fileName().toStdWString().c_str() );
#else
    logStream.open( f.fileName().toStdString().c_str() );
#endif

    qInstallMessageHandler( TomahawkLogHandler );
}

}

using namespace Logger;

TLog::TLog( unsigned int debugLevel )
    : QDebug( &m_msg )
    , m_debugLevel( debugLevel )
{
}


TLog::~TLog()
{
    log( m_msg.toUtf8().data(), m_debugLevel );
}


void
tLogNotifyShutdown()
{
    QMutexLocker locker( &s_mutex );
    shutdownInProgress = true;
}
