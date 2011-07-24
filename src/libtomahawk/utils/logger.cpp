#include "logger.h"

#include <iostream>
#include <fstream>

#include <QDir>
#include <QFileInfo>
#include <QMutex>
#include <QTime>

#include "utils/tomahawkutils.h"

#define LOGFILE TomahawkUtils::appLogDir().filePath( "Tomahawk.log" ).toLocal8Bit()
#define LOGFILE_SIZE 1024 * 512

using namespace std;
ofstream logfile;

namespace Logger
{

void
TomahawkLogHandler( QtMsgType type, const char *msg )
{
    static QMutex s_mutex;

    QMutexLocker locker( &s_mutex );
    switch( type )
    {
        case QtDebugMsg:
//            logfile << QTime::currentTime().toString().toAscii().data() << " Debug: " << msg << "\n";
            break;

        case QtCriticalMsg:
            logfile << QTime::currentTime().toString().toAscii().data() << " Critical: " << msg << endl;
            break;

        case QtWarningMsg:
            logfile << QTime::currentTime().toString().toAscii().data() << " Warning: " << msg << endl;
            break;

        case QtFatalMsg:
            logfile << QTime::currentTime().toString().toAscii().data() << " Fatal: " << msg << endl;
            logfile.flush();

            cout << msg << endl;
            cout.flush();
            abort();
            break;
    }

    cout << msg << "\n";
    cout.flush();
    logfile.flush();
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
