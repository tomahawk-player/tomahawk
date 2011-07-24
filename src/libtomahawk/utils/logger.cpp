#include "logger.h"

#include <iostream>
#include <fstream>

#include <QDir>
#include <QFileInfo>
#include <QMutex>
#include <QTime>
#include <QVariant>

#include "utils/tomahawkutils.h"

#define LOGFILE TomahawkUtils::appLogDir().filePath( "Tomahawk.log" ).toLocal8Bit()
#define LOGFILE_SIZE 1024 * 512

using namespace std;
ofstream logfile;

namespace Logger
{

tLog&
tLog::operator<<( const QVariant& v )
{
    QString const s = v.toString();
    if ( !s.isEmpty() )
        log( s.toAscii().data() );

    return *this;
}


void
tLog::log( const char *msg )
{
    logfile << QTime::currentTime().toString().toAscii().data() << " " << msg << endl;
    logfile.flush();
}


void
TomahawkLogHandler( QtMsgType type, const char *msg )
{
    static QMutex s_mutex;

    QMutexLocker locker( &s_mutex );
    switch( type )
    {
        case QtDebugMsg:
            // Disable debug logging in release builds:
            #ifndef QT_NO_DEBUG
            tLog::log( msg );
            #endif
            break;

        case QtCriticalMsg:
            tLog::log( msg );
            break;

        case QtWarningMsg:
            tLog::log( msg );
            break;

        case QtFatalMsg:
            tLog::log( msg );
            break;
    }

    cout << msg << endl;
    cout.flush();
}


void
setupLogfile()
{
    if ( QFileInfo( LOGFILE ).size() > LOGFILE_SIZE )
    {
        QByteArray lc;
        {
            QFile f( LOGFILE );
            f.open( QIODevice::ReadOnly | QIODevice::Text );
            lc = f.readAll();
            f.close();
        }

        QFile::remove( LOGFILE );

        {
            QFile f( LOGFILE );
            f.open( QIODevice::WriteOnly | QIODevice::Text );
            f.write( lc.right( LOGFILE_SIZE - (LOGFILE_SIZE / 4) ) );
            f.close();
        }
    }

    logfile.open( LOGFILE, ios::app );
    qInstallMsgHandler( TomahawkLogHandler );
}

} // ns
