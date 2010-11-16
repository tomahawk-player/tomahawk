#include "tomahawkutils.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>

#ifdef WIN32
    #include <windows.h>
    #include <shlobj.h>
#endif

#ifdef Q_WS_MAC
    #include <Carbon/Carbon.h>
    #include <sys/sysctl.h>
#endif


namespace TomahawkUtils
{

#ifdef Q_WS_MAC

QString
appSupportFolderPath()
{
    // honestly, it is *always* this --mxcl
    return QDir::home().filePath( "Library/Application Support" );
}

#endif // Q_WS_MAC


QDir
appConfigDir()
{
    QDir ret;

#ifdef Q_WS_MAC
    if( getenv( "HOME" ) )
    {
        return QDir( QString( "%1" ).arg( getenv( "HOME" ) ) );
    }
    else
    {
        qDebug() << "Error, $HOME not set.";
        throw "$HOME not set";
        return QDir( "/tmp" );
    }

#elif defined(Q_WS_WIN)
    throw "TODO";
    return QDir( "c:\\" ); //TODO refer to Qt documentation to get code to do this

#else
    if( getenv( "XDG_CONFIG_HOME" ) )
    {
        ret = QDir( QString( "%1/Tomahawk" ).arg( getenv( "XDG_CONFIG_HOME" ) ) );
    }
    else if( getenv( "HOME" ) )
    {
        ret = QDir( QString( "%1/.config/Tomahawk" ).arg( getenv( "HOME" ) ) );
    }
    else
    {
        qDebug() << "Error, $HOME or $XDG_CONFIG_HOME not set.";
        throw "Error, $HOME or $XDG_CONFIG_HOME not set.";
        ret = QDir( "/tmp" );
    }

    if( !ret.exists() )
    {
        ret.mkpath( ret.canonicalPath() );
    }

    return ret;
#endif
}


QDir
appDataDir()
{
    QString path;

    #ifdef WIN32
        if ( ( QSysInfo::WindowsVersion & QSysInfo::WV_DOS_based ) == 0 )
        {
            // Use this for non-DOS-based Windowses
            char acPath[MAX_PATH];
            HRESULT h = SHGetFolderPathA( NULL, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE,
                                          NULL, 0, acPath );
            if ( h == S_OK )
            {
                path = QString::fromLocal8Bit( acPath );
            }
        }
    #elif defined(Q_WS_MAC)
        path = appSupportFolderPath();
    #elif defined(Q_WS_X11)
        path = QDir::home().filePath( ".local/share" );
    #else
        path = QCoreApplication::applicationDirPath();
    #endif

    path += "/" + QCoreApplication::organizationName();
    QDir d( path );
    d.mkpath( path );

    return d;
}


QString
timeToString( int seconds )
{
    int hrs  = seconds / 60 / 60;
    int mins = seconds / 60 % 60;
    int secs = seconds % 60;

    if ( seconds < 0 )
    {
        hrs = mins = secs = 0;
    }

    return QString( "%1%2:%3" ).arg( hrs > 0 ? hrs  < 10 ? "0" + QString::number( hrs ) + ":" : QString::number( hrs ) + ":" : "" )
                               .arg( mins < 10 ? "0" + QString::number( mins ) : QString::number( mins ) )
                               .arg( secs < 10 ? "0" + QString::number( secs ) : QString::number( secs ) );
}

QString
ageToString( const QDateTime& time )
{
    QDateTime now = QDateTime::currentDateTime();

    int mins = time.secsTo( now ) / 60;
    int hours = mins / 60;
    int days = time.daysTo( now );
    int weeks = days / 7;
    int months = days / 30.42;
    int years = months / 12;

    if ( years )
    {
        if ( years > 1 )
            return QString( "%1 years ago" ).arg( years );
        else
            return QString( "%1 year ago" ).arg( years );
    }

    if ( months )
    {
        if ( months > 1 )
           return QString( "%1 months ago" ).arg( months );
        else
            return QString( "%1 month ago" ).arg( months );
    }

    if ( weeks )
    {
        if ( weeks > 1 )
            return QString( "%1 weeks ago" ).arg( weeks );
        else
            return QString( "%1 week ago" ).arg( weeks );
    }

    if ( days )
    {
        if ( days > 1 )
            return QString( "%1 days ago" ).arg( days );
        else
            return QString( "%1 day ago" ).arg( days );
    }

    if ( hours )
    {
        if ( hours > 1 )
            return QString( "%1 hours ago" ).arg( hours );
        else
            return QString( "%1 hour ago" ).arg( hours );
    }

    if ( mins )
    {
        if ( mins > 1 )
            return QString( "%1 minutes ago" ).arg( mins );
        else
            return QString( "%1 minute ago" ).arg( mins );
    }

    return QString();
}

} // ns
