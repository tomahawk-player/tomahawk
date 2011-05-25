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
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */

#include "tomahawkutils.h"
#include <tomahawksettings.h>

#include <QCoreApplication>
#include <QColor>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QPainter>
#include <QPixmap>
#include <QNetworkAccessManager>
#include <QNetworkProxy>

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


QString
extensionToMimetype( const QString& extension )
{
    static QMap<QString, QString> s_ext2mime;
    if ( s_ext2mime.isEmpty() )
    {
        s_ext2mime.insert( "mp3",  "audio/mpeg" );
        s_ext2mime.insert( "ogg",  "application/ogg" );
        s_ext2mime.insert( "flac", "audio/flac" );
        s_ext2mime.insert( "mpc",  "audio/x-musepack" );
        s_ext2mime.insert( "wma",  "audio/x-ms-wma" );
        s_ext2mime.insert( "aac",  "audio/mp4" );
        s_ext2mime.insert( "m4a",  "audio/mp4" );
        s_ext2mime.insert( "mp4",  "audio/mp4" );
    }

    return s_ext2mime.value( extension, "unknown" );
}


QColor
alphaBlend( const QColor& colorFrom, const QColor& colorTo, float opacity )
{
    opacity = qMax( (float)0.3, opacity );
    int r = colorFrom.red(), g = colorFrom.green(), b = colorFrom.blue();
    r = opacity * r + ( 1 - opacity ) * colorTo.red();
    g = opacity * g + ( 1 - opacity ) * colorTo.green();
    b = opacity * b + ( 1 - opacity ) * colorTo.blue();

    return QColor( r, g, b );
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


QWeakPointer< QNetworkAccessManager > s_nam;
NetworkProxyFactory* s_proxyFactory = 0;


QList< QNetworkProxy >
NetworkProxyFactory::proxyForQuery( const QNetworkProxyQuery& query )
{
    Q_UNUSED( query );
    QList< QNetworkProxy > proxies;
    proxies << QNetworkProxy( QNetworkProxy::NoProxy );
    return proxies;
}


QList< QNetworkProxy >
NetworkProxyFactory::queryProxy( const QNetworkProxyQuery& query )
{
    QList< QNetworkProxy > proxies;
    QString hostname = query.peerHostName();
    if ( hostname.isEmpty() || m_noProxyHosts.contains( hostname ) )
        proxies << QNetworkProxy( QNetworkProxy::NoProxy );
    else
        proxies << m_proxy << QNetworkProxy( QNetworkProxy::NoProxy ) << QNetworkProxy( QNetworkProxy::DefaultProxy );

    return proxies;
}


void
NetworkProxyFactory::setNoProxyHosts( const QStringList& hosts )
{
    QStringList newList;
    foreach( QString host, hosts )
    {
        QString munge = host.simplified();
        newList << munge;
    }
    qDebug() << Q_FUNC_INFO << " No-proxy hosts: " << newList;
    m_noProxyHosts = newList;
}


void
NetworkProxyFactory::setProxy( const QNetworkProxy& proxy )
{
    m_proxy = proxy;
    if ( !TomahawkSettings::instance()->proxyDns() )
        m_proxy.setCapabilities( QNetworkProxy::TunnelingCapability | QNetworkProxy::ListeningCapability | QNetworkProxy::UdpTunnelingCapability );
    qDebug() << Q_FUNC_INFO << " Proxy using host " << proxy.hostName() << " and port " << proxy.port();
    qDebug() << Q_FUNC_INFO << " setting proxy to use proxy DNS?" << (TomahawkSettings::instance()->proxyDns() ? "true" : "false");
}


NetworkProxyFactory*
proxyFactory()
{
    // Don't use this anywhere! It's provided here for access reasons, but QNAM deletes this at will!
    
    if ( !s_proxyFactory )
        s_proxyFactory = new NetworkProxyFactory();
    
    return s_proxyFactory;
}


void
setProxyFactory( NetworkProxyFactory* factory )
{
    Q_ASSERT( factory );
    s_proxyFactory = factory;
    NetworkProxyFactory::setApplicationProxyFactory( s_proxyFactory );
}


QNetworkAccessManager*
nam()
{
    if ( s_nam.isNull() )
        return 0;

    return s_nam.data();
}


void
setNam( QNetworkAccessManager* nam )
{
    Q_ASSERT( nam );
    s_nam = QWeakPointer< QNetworkAccessManager >( nam );
}


} // ns
