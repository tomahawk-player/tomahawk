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

#include "config.h"
#include "tomahawkutils.h"

#include "headlesscheck.h"
#include <QtCore/QCoreApplication>

#include <QtGui/QColor>
#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QMutex>
#include <QtGui/QLayout>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>
#include <QtNetwork/QNetworkConfiguration>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkProxy>

#ifdef Q_WS_WIN
    #include <windows.h>
    #include <shlobj.h>
#endif

#ifdef Q_WS_MAC
    #include <Carbon/Carbon.h>
    #include <sys/sysctl.h>
#endif

#ifndef TOMAHAWK_HEADLESS
    #include <QtGui/QApplication>
    #include <QtGui/QWidget>

    #ifdef Q_WS_X11
        #include <QtGui/QX11Info>
        #include <libqnetwm/netwm.h>
    #endif

    #ifdef Q_WS_WIN
        #include <windows.h>
        #include <windowsx.h>
    #endif
#endif

#include <tomahawksettings.h>
#include "utils/logger.h"
#include "config.h"

#ifdef LIBLASTFM_FOUND
#include <lastfm/ws.h>
#endif

namespace TomahawkUtils
{


static int s_headerHeight = 0;
static quint64 s_infosystemRequestId = 0;
static QMutex s_infosystemRequestIdMutex;

#ifdef Q_WS_MAC
QString
appSupportFolderPath()
{
    // honestly, it is *always* this --mxcl
    return QDir::home().filePath( "Library/Application Support" );
}
#endif // Q_WS_MAC


QString
appFriendlyVersion()
{
    QStringList l = QString( TOMAHAWK_VERSION ).split( ".", QString::SkipEmptyParts );
    while ( l.count() > 3 )
        l.removeLast();

    return l.join( "." );
}


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

    #ifdef Q_WS_WIN
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
sqlEscape( QString sql )
{
    return sql.replace( "'", "''" );
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
    if ( time.toTime_t() == 0 )
        return QString();

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
            return QObject::tr( "%1 years" ).arg( years );
        else
            return QObject::tr( "%1 year" ).arg( years );
    }

    if ( months )
    {
        if ( months > 1 )
            return QObject::tr( "%1 months" ).arg( months );
        else
            return QObject::tr( "%1 month" ).arg( months );
    }

    if ( weeks )
    {
        if ( weeks > 1 )
            return QObject::tr( "%1 weeks" ).arg( weeks );
        else
            return QObject::tr( "%1 week" ).arg( weeks );
    }

    if ( days )
    {
        if ( days > 1 )
            return QObject::tr( "%1 days" ).arg( days );
        else
            return QObject::tr( "%1 day" ).arg( days );
    }

    if ( hours )
    {
        if ( hours > 1 )
            return QObject::tr( "%1 hours" ).arg( hours );
        else
            return QObject::tr( "%1 hour" ).arg( hours );
    }

    if ( mins )
    {
        if ( mins > 1 )
            return QObject::tr( "%1 minutes" ).arg( mins );
    }

    return QObject::tr( "1 minute" );
}


QString
filesizeToString( unsigned int size )
{
    if ( size == 0 )
        return QString();

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
createDragPixmap( MediaType type, int itemCount )
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

    QPixmap pixmap;
    switch ( type )
    {
    case MediaTypeArtist:
        pixmap = QPixmap( ":/data/images/artist-icon.png" ).scaledToWidth( size, Qt::SmoothTransformation );
        break;
    case MediaTypeAlbum:
        pixmap = QPixmap( ":/data/images/album-icon.png" ).scaledToWidth( size, Qt::SmoothTransformation );
        break;
    case MediaTypeTrack:
        pixmap = QPixmap( QString( ":/data/images/track-icon-%2x%2.png" ).arg( size ) );
        break;
    }

    int x = 0;
    int y = 0;
    for( int i = 0; i < itemCount; ++i )
    {

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

void
drawBackgroundAndNumbers( QPainter* painter, const QString& text, const QRect& figRectIn )
{
    QRect figRect = figRectIn;
    if ( text.length() == 1 )
        figRect.adjust( -painter->fontMetrics().averageCharWidth(), 0, 0, 0 );

    QPen origpen = painter->pen();
    QPen pen = origpen;
    pen.setWidth( 1.0 );
    painter->setPen( pen );
    painter->drawRect( figRect );

    // circles look bad. make it an oval. (thanks, apple)
    const int bulgeWidth = 8;
    const int offset = 0; // number of pixels to begin, counting inwards from figRect.x() and figRect.width(). 0 means start at each end, negative means start inside the rect.

    QPainterPath ppath;
    ppath.moveTo( QPoint( figRect.x() + offset, figRect.y() + figRect.height() / 2 ) );
    QRect leftArcRect( figRect.x() + offset - bulgeWidth, figRect.y(), 2*bulgeWidth, figRect.height() );
    ppath.arcTo( leftArcRect, 90, 180 );
    painter->drawPath( ppath );

    ppath = QPainterPath();
    ppath.moveTo( figRect.x() + figRect.width() - offset, figRect.y() + figRect.height() / 2 );
    leftArcRect = QRect( figRect.x() + figRect.width() - offset - bulgeWidth, figRect.y(), 2*bulgeWidth, figRect.height() );
    ppath.arcTo( leftArcRect, 270, 180 );
    painter->drawPath( ppath );

    painter->setPen( origpen );

#ifdef Q_WS_MAC
    figRect.adjust( -1, 0, 0, 0 );
#endif

    QTextOption to( Qt::AlignCenter );
    painter->setPen( Qt::white );
    painter->drawText( figRect.adjusted( -5, 0, 6, 0 ), text, to );
}

void
drawQueryBackground( QPainter* p, const QPalette& palette, const QRect& r, qreal lightnessFactor )
{
    p->setPen( palette.mid().color().lighter( lightnessFactor * 100 ) );
    p->setBrush( palette.highlight().color().lighter( lightnessFactor * 100 ) );
    p->drawRoundedRect( r, 4.0, 4.0 );
}


void
unmarginLayout( QLayout* layout )
{
    layout->setContentsMargins( 0, 0, 0, 0 );
    layout->setMargin( 0 );
    layout->setSpacing( 0 );

    for ( int i = 0; i < layout->count(); i++ )
    {
        QLayout* childLayout = layout->itemAt( i )->layout();
        if ( childLayout )
            unmarginLayout( childLayout );
    }
}


NetworkProxyFactory::NetworkProxyFactory( const NetworkProxyFactory& other )
{
    m_noProxyHosts = QStringList( other.m_noProxyHosts );
    m_proxy = QNetworkProxy( other.m_proxy );
}


QList< QNetworkProxy >
NetworkProxyFactory::proxyForQuery( const QNetworkProxyQuery& query )
{
    Q_UNUSED( query );
    QList< QNetworkProxy > proxies;
    proxies << QNetworkProxy( QNetworkProxy::DefaultProxy ) << QNetworkProxy( QNetworkProxy::NoProxy );
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
    tDebug() << Q_FUNC_INFO << "No-proxy hosts:" << hosts;
    foreach( QString host, hosts )
    {
        QString munge = host.simplified();
        newList << munge;
        //TODO: wildcard support
    }
    tDebug() << Q_FUNC_INFO << "New no-proxy hosts:" << newList;
    m_noProxyHosts = newList;
}


void
NetworkProxyFactory::setProxy( const QNetworkProxy& proxy )
{
    m_proxy = proxy;
    if ( !TomahawkSettings::instance()->proxyDns() )
        m_proxy.setCapabilities( QNetworkProxy::TunnelingCapability | QNetworkProxy::ListeningCapability | QNetworkProxy::UdpTunnelingCapability );
    tDebug() << Q_FUNC_INFO << "Proxy using host" << proxy.hostName() << "and port" << proxy.port();
    tDebug() << Q_FUNC_INFO << "setting proxy to use proxy DNS?" << (TomahawkSettings::instance()->proxyDns() ? "true" : "false");
}


NetworkProxyFactory&
NetworkProxyFactory::operator=( const NetworkProxyFactory& rhs )
{
    tDebug() << Q_FUNC_INFO;
    if ( this != &rhs )
    {
        m_proxy = QNetworkProxy( rhs.m_proxy );
        m_noProxyHosts = QStringList( rhs.m_noProxyHosts );
    }

    return *this;
}


bool NetworkProxyFactory::operator==( const NetworkProxyFactory& other ) const
{
    tDebug() << Q_FUNC_INFO;
    if ( m_noProxyHosts != other.m_noProxyHosts || m_proxy != other.m_proxy )
        return false;

    return true;
}

static QMap< QThread*, QNetworkAccessManager* > s_threadNamHash;
static QMap< QThread*, NetworkProxyFactory* > s_threadProxyFactoryHash;
static QMutex s_namAccessMutex;

NetworkProxyFactory*
proxyFactory( bool noMutexLocker )
{
    // Don't lock if being called from nam()
    QMutex otherMutex;
    QMutexLocker locker( noMutexLocker ? &otherMutex : &s_namAccessMutex );

    if ( s_threadProxyFactoryHash.contains( QThread::currentThread() ) )
        return s_threadProxyFactoryHash[ QThread::currentThread() ];

    if ( !s_threadProxyFactoryHash.contains( TOMAHAWK_APPLICATION::instance()->thread() ) )
        return 0;

    // create a new proxy factory for this thread
    TomahawkUtils::NetworkProxyFactory *mainProxyFactory = s_threadProxyFactoryHash[ TOMAHAWK_APPLICATION::instance()->thread() ];
    TomahawkUtils::NetworkProxyFactory *newProxyFactory = new TomahawkUtils::NetworkProxyFactory();
    *newProxyFactory = *mainProxyFactory;

    s_threadProxyFactoryHash[ QThread::currentThread() ] = newProxyFactory;

    return newProxyFactory;
}


void
setProxyFactory( NetworkProxyFactory* factory, bool noMutexLocker )
{
    Q_ASSERT( factory );
    // Don't lock if being called from setNam()
    QMutex otherMutex;
    QMutexLocker locker( noMutexLocker ? &otherMutex : &s_namAccessMutex );

    if ( !s_threadProxyFactoryHash.contains( TOMAHAWK_APPLICATION::instance()->thread() ) )
        return;

    TomahawkUtils::NetworkProxyFactory *oldProxyFactory = s_threadProxyFactoryHash[ QThread::currentThread() ];
    if ( QThread::currentThread() == TOMAHAWK_APPLICATION::instance()->thread() )
    {
        // If setting new values on the main thread, clear the other entries
        // so that on next access new ones will be created with new proper values
        NetworkProxyFactory::setApplicationProxyFactory( factory );
        foreach( QThread* thread, s_threadProxyFactoryHash.keys() )
        {
            if ( thread != QThread::currentThread() )
            {
                TomahawkUtils::NetworkProxyFactory *currFactory = s_threadProxyFactoryHash[ thread ];
                *currFactory = *factory;
            }
        }
    }

    *s_threadProxyFactoryHash[ QThread::currentThread() ] = *factory;
}


QNetworkAccessManager*
nam()
{
    QMutexLocker locker( &s_namAccessMutex );
    if ( s_threadNamHash.contains(  QThread::currentThread() ) )
        return s_threadNamHash[ QThread::currentThread() ];

    if ( !s_threadNamHash.contains( TOMAHAWK_APPLICATION::instance()->thread() ) )
        return 0;

    // Create a nam for this thread based on the main thread's settings but with its own proxyfactory
    QNetworkAccessManager *mainNam = s_threadNamHash[ TOMAHAWK_APPLICATION::instance()->thread() ];
    QNetworkAccessManager* newNam;
#ifdef LIBLASTFM_FOUND
    newNam = lastfm::nam();
#else
    newNam = new QNetworkAccessManager();
#endif

    newNam->setConfiguration( QNetworkConfiguration( mainNam->configuration() ) );
    newNam->setNetworkAccessible( mainNam->networkAccessible() );
    newNam->setProxyFactory( proxyFactory( true ) );

    s_threadNamHash[ QThread::currentThread() ] = newNam;

    return newNam;
}


void
setNam( QNetworkAccessManager* nam )
{
    Q_ASSERT( nam );
    QMutexLocker locker( &s_namAccessMutex );
    if ( !s_threadNamHash.contains( TOMAHAWK_APPLICATION::instance()->thread() ) &&
            QThread::currentThread() == TOMAHAWK_APPLICATION::instance()->thread() )
    {
        // Should only get here on first initialization of the nam
        TomahawkSettings *s = TomahawkSettings::instance();
        TomahawkUtils::NetworkProxyFactory* proxyFactory = new TomahawkUtils::NetworkProxyFactory();
        if ( s->proxyType() != QNetworkProxy::NoProxy && !s->proxyHost().isEmpty() )
        {
            tDebug( LOGEXTRA ) << "Setting proxy to saved values";
            QNetworkProxy proxy( static_cast<QNetworkProxy::ProxyType>( s->proxyType() ), s->proxyHost(), s->proxyPort(), s->proxyUsername(), s->proxyPassword() );
            proxyFactory->setProxy( proxy );
            //TODO: On Windows and Mac because liblastfm sets an application level proxy it may override our factory, so may need to explicitly do
            //a QNetworkProxy::setApplicationProxy with our own proxy (but then also overriding our own factory :-( )
        }
        if ( !s->proxyNoProxyHosts().isEmpty() )
            proxyFactory->setNoProxyHosts( s->proxyNoProxyHosts().split( ',', QString::SkipEmptyParts ) );

        nam->setProxyFactory( proxyFactory );
        s_threadNamHash[ QThread::currentThread() ] = nam;
        s_threadProxyFactoryHash[ QThread::currentThread() ] = proxyFactory;
        NetworkProxyFactory::setApplicationProxyFactory( proxyFactory );
        return;
    }

    s_threadNamHash[ QThread::currentThread() ] = nam;

    if ( QThread::currentThread() == TOMAHAWK_APPLICATION::instance()->thread() )
        setProxyFactory( dynamic_cast< TomahawkUtils::NetworkProxyFactory* >( nam->proxyFactory() ), true );
}


#ifndef TOMAHAWK_HEADLESS

QWidget*
tomahawkWindow()
{
    QWidgetList widgetList = qApp->topLevelWidgets();
    int i = 0;
    while( i < widgetList.count() && widgetList.at( i )->objectName() != "TH_Main_Window" )
        i++;

    if ( i == widgetList.count() )
    {
        qDebug() << Q_FUNC_INFO << "could not find main Tomahawk mainwindow";
        Q_ASSERT( false );
        return 0;
    }

    QWidget *widget = widgetList.at( i );
    return widget;
}


#ifndef Q_WS_MAC
void
bringToFront()
{
#if defined(Q_WS_X11)
    {
        qDebug() << Q_FUNC_INFO;

        QWidget* widget = tomahawkWindow();
        if ( !widget )
            return;

        widget->show();
        widget->activateWindow();
        widget->raise();

        WId wid = widget->winId();
        NETWM::init();

        XEvent e;
        e.xclient.type = ClientMessage;
        e.xclient.message_type = NETWM::NET_ACTIVE_WINDOW;
        e.xclient.display = QX11Info::display();
        e.xclient.window = wid;
        e.xclient.format = 32;
        e.xclient.data.l[0] = 2;
        e.xclient.data.l[1] = QX11Info::appTime();
        e.xclient.data.l[2] = 0;
        e.xclient.data.l[3] = 0l;
        e.xclient.data.l[4] = 0l;

        XSendEvent( QX11Info::display(), RootWindow( QX11Info::display(), DefaultScreen( QX11Info::display() ) ), False, SubstructureRedirectMask | SubstructureNotifyMask, &e );
    }
#elif defined(Q_WS_WIN)
    {
        qDebug() << Q_FUNC_INFO;

        QWidget* widget = tomahawkWindow();
        if ( !widget )
            return;

        widget->show();
        widget->activateWindow();
        widget->raise();

        WId wid = widget->winId();

        HWND hwndActiveWin = GetForegroundWindow();
        int  idActive      = GetWindowThreadProcessId(hwndActiveWin, NULL);
        if ( AttachThreadInput(GetCurrentThreadId(), idActive, TRUE) )
        {
            SetForegroundWindow( wid );
            SetFocus( wid );
            AttachThreadInput(GetCurrentThreadId(), idActive, FALSE);
        }
    }
#endif
}
#endif

#endif


QPixmap
createAvatarFrame( const QPixmap &avatar )
{
    QPixmap frame( ":/data/images/avatar_frame.png" );
    QPixmap scaledAvatar = avatar.scaled( frame.height() * 75 / 100, frame.width() * 75 / 100, Qt::KeepAspectRatio, Qt::SmoothTransformation );

    QPainter painter( &frame );
    painter.drawPixmap( (frame.height() - scaledAvatar.height()) / 2, (frame.width() - scaledAvatar.width()) / 2, scaledAvatar );

    return frame;
}


void
crash()
{
    volatile int* a = (int*)(NULL);
    *a = 1;
}

int
headerHeight()
{
    return s_headerHeight;
}

void
setHeaderHeight( int height )
{
    s_headerHeight = height;
}

// taken from util/fileutils.cpp in kdevplatform
bool
removeDirectory( const QString& dir )
{
    const QDir aDir(dir);

    tLog() << "Deleting DIR:" << dir;
    bool has_err = false;
    if (aDir.exists()) {
        foreach(const QFileInfo& entry, aDir.entryInfoList(QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files | QDir::NoSymLinks)) {
            QString path = entry.absoluteFilePath();
            if (entry.isDir()) {
                has_err = !removeDirectory(path) || has_err;
            } else if (!QFile::remove(path)) {
                has_err = true;
            }
        }
        if (!aDir.rmdir(aDir.absolutePath())) {
            has_err = true;
        }
    }
    return !has_err;
}


quint64 infosystemRequestId()
{
    QMutexLocker locker( &s_infosystemRequestIdMutex );
    quint64 result = s_infosystemRequestId;
    s_infosystemRequestId++;
    return result;
}

} // ns
