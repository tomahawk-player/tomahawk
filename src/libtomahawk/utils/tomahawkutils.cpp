#include "tomahawkutils.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QPainter>
#include <QPixmap>
#include <QNetworkAccessManager>

#ifdef WIN32
    #include <windows.h>
    #include <shlobj.h>
#endif

#ifdef Q_WS_MAC
    #include <Carbon/Carbon.h>
    #include <sys/sysctl.h>
#endif

#include <qjdns.h>
#include <jdnsshared.h>

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


QDir
appLogDir()
{
#ifndef Q_WS_MAC
    return appDataDir();
#else
    return QDir( QDir::homePath() + "/Library/Logs" );
#endif
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
            return QString( "%1 years" ).arg( years );
        else
            return QString( "%1 year" ).arg( years );
    }

    if ( months )
    {
        if ( months > 1 )
           return QString( "%1 months" ).arg( months );
        else
            return QString( "%1 month" ).arg( months );
    }

    if ( weeks )
    {
        if ( weeks > 1 )
            return QString( "%1 weeks" ).arg( weeks );
        else
            return QString( "%1 week" ).arg( weeks );
    }

    if ( days )
    {
        if ( days > 1 )
            return QString( "%1 days" ).arg( days );
        else
            return QString( "%1 day" ).arg( days );
    }

    if ( hours )
    {
        if ( hours > 1 )
            return QString( "%1 hours" ).arg( hours );
        else
            return QString( "%1 hour" ).arg( hours );
    }

    if ( mins )
    {
        if ( mins > 1 )
            return QString( "%1 minutes" ).arg( mins );
        else
            return QString( "%1 minute" ).arg( mins );
    }

    return QString();
}


QString
filesizeToString( unsigned int size )
{
    int kb = size / 1024;
    int mb = kb / 1024;

    if ( mb )
    {
        return QString( "%1.%2 Mb" ).arg( mb ).arg( int( ( kb % 1024 ) / 102.4 ) );
    }
    else if ( kb )
    {
        return QString( "%1 Kb" ).arg( kb );
    }
    else
        return QString::number( size );
}


QPixmap
createDragPixmap( int itemCount )
{
    // If more than one item is dragged, align the items inside a
    // rectangular grid. The maximum grid size is limited to 5 x 5 items.
    int xCount = 3;
    int size = 32;

    if ( itemCount > 16 )
    {
        xCount = 5;
        size = 16;
    } else if( itemCount > 9 )
    {
        xCount = 4;
        size = 22;
    }

    if( itemCount < xCount )
    {
        xCount = itemCount;
    }

    int yCount = itemCount / xCount;
    if( itemCount % xCount != 0 )
    {
        ++yCount;
    }
    if( yCount > xCount )
    {
        yCount = xCount;
    }
    // Draw the selected items into the grid cells
    QPixmap dragPixmap( xCount * size + xCount - 1, yCount * size + yCount - 1 );
    dragPixmap.fill( Qt::transparent );

    QPainter painter( &dragPixmap );
    painter.setRenderHint( QPainter::Antialiasing );
    int x = 0;
    int y = 0;
    for( int i = 0; i < itemCount; ++i )
    {
        const QPixmap pixmap = QPixmap( QString( ":/data/icons/audio-x-generic-%2x%2.png" ).arg( size ) );
        painter.drawPixmap( x, y, pixmap );

        x += size + 1;
        if ( x >= dragPixmap.width() )
        {
            x = 0;
            y += size + 1;
        }
        if ( y >= dragPixmap.height() )
        {
            break;
        }
    }

    return dragPixmap;
}


QNetworkAccessManager* s_nam = 0;
QNetworkProxy* s_proxy = 0;

QNetworkAccessManager*
nam()
{
    return s_nam;
}


QNetworkProxy*
proxy()
{
    return s_proxy;
}


void
setNam( QNetworkAccessManager* nam )
{
    s_nam = nam;
}


void
setProxy( QNetworkProxy* proxy )
{
    s_proxy = proxy;
}


///////////////// DNSResolver /////////////////

static DNSResolver* s_dnsResolver = 0;

DNSResolver*
dnsResolver()
{
    if( !s_dnsResolver )
        s_dnsResolver = new DNSResolver();
    
    return s_dnsResolver;
}

DNSResolver::DNSResolver()
{
    m_dnsShared = new JDnsShared(JDnsShared::UnicastInternet);
    m_dnsShared->addInterface(QHostAddress::Any);
    m_dnsShared->addInterface(QHostAddress::AnyIPv6);
    m_dnsSharedRequest = new JDnsSharedRequest(m_dnsShared);

    connect(m_dnsSharedRequest, SIGNAL(resultsReady()), SLOT(resultsReady()));
}

void
DNSResolver::resolve( QString &host, QString type )
{
    if( type == "SRV" )
    {
        // For the moment, assume we are looking for XMPP...
        QString fullHost( "_xmpp-client._tcp." + host );
        
        qDebug() << "Looking up SRV record for " << fullHost.toUtf8();

        m_dnsSharedRequest->query( fullHost.toUtf8(), QJDns::Srv );
    }
    else
    {
        QString badResult( "NONE" );
        emit result( badResult );
    }
}

void
DNSResolver::resultsReady()
{
    if( m_dnsSharedRequest->success() )
    {
        QList<QJDns::Record> results = m_dnsSharedRequest->results();
        foreach( QJDns::Record r, results )
        {
	    qDebug() << "Found result (of some type): " << QString( r.name );
            if( r.type == QJDns::Srv )
            {
                QString foundResult( r.name );
                emit result( foundResult );
                return;
            }
        }
    }
    qDebug() << "DNS resolve request was NOT successful! Error: " << (int)(m_dnsSharedRequest->error());
    QString badResult( "NONE" );
    emit result( badResult );
}

} // ns
