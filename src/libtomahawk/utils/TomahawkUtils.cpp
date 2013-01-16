/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include "utils/TomahawkUtils.h"

#include "TomahawkVersion.h"
#include "config.h"
#include "TomahawkSettings.h"

#include "Source.h"
#include "BinaryExtractWorker.h"
#include "SharedTimeLine.h"

#ifdef LIBLASTFM_FOUND
    #include <lastfm/ws.h>
#endif

#include <quazip.h>
#include <quazipfile.h>

#include <QNetworkConfiguration>
#include <QNetworkAccessManager>
#include <QNetworkProxy>

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QMutex>
#include <QCryptographicHash>
#include <QProcess>
#include <QTranslator>

#ifdef Q_OS_WIN
    #include <windows.h>
    #include <shlobj.h>
#endif

#ifdef Q_WS_MAC
    #include <Carbon/Carbon.h>
    #include <sys/sysctl.h>
#endif

#ifdef QCA2_FOUND
    #include <QtCrypto>
#endif

#include "Logger.h"

namespace TomahawkUtils
{
static quint64 s_infosystemRequestId = 0;
static QMutex s_infosystemRequestIdMutex;
static bool s_headless = false;

#ifdef Q_WS_MAC
QString
appSupportFolderPath()
{
    // honestly, it is *always* this --mxcl
    return QDir::home().filePath( "Library/Application Support" );
}
#endif // Q_WS_MAC


bool
headless()
{
    return s_headless;
}


void
setHeadless( bool headless )
{
    tLog() << Q_FUNC_INFO << "headless is" << (headless? "true" : "false");
    s_headless = headless;
}


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
    if ( getenv( "HOME" ) )
    {
        return QDir( QString( "%1" ).arg( getenv( "HOME" ) ) );
    }
    else
    {
        tDebug() << "Error, $HOME not set.";
        throw "$HOME not set";
        return QDir( "/tmp" );
    }

#elif defined(Q_WS_WIN)
    throw "TODO";
    return QDir( "c:\\" ); //TODO refer to Qt documentation to get code to do this

#else
    if ( getenv( "XDG_CONFIG_HOME" ) )
    {
        ret = QDir( QString( "%1/Tomahawk" ).arg( getenv( "XDG_CONFIG_HOME" ) ) );
    }
    else if ( getenv( "HOME" ) )
    {
        ret = QDir( QString( "%1/.config/Tomahawk" ).arg( getenv( "HOME" ) ) );
    }
    else
    {
        tDebug() << "Error, $HOME or $XDG_CONFIG_HOME not set.";
        throw "Error, $HOME or $XDG_CONFIG_HOME not set.";
        ret = QDir( "/tmp" );
    }

    if ( !ret.exists() )
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
ageToString( const QDateTime& time, bool appendAgoString )
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

    if ( mins > 0 )
    {
        if ( years )
        {
            if ( appendAgoString )
                return QObject::tr( "%n year(s) ago", "", years );
            else
                return QObject::tr( "%n year(s)", "", years );
        }

        if ( months )
        {
            if ( appendAgoString )
                return QObject::tr( "%n month(s) ago", "", months );
            else
                return QObject::tr( "%n month(s)", "", months );
        }

        if ( weeks )
        {
            if ( appendAgoString )
                return QObject::tr( "%n week(s) ago", "", weeks );
            else
                return QObject::tr( "%n week(s)", "", weeks );
        }

        if ( days )
        {
            if ( appendAgoString )
                return QObject::tr( "%n day(s) ago", "", days );
            else if ( hours >= 24 )
                return QObject::tr( "%n day(s)", "", days );
        }

        if ( hours )
        {
            if ( appendAgoString )
                return QObject::tr( "%n hour(s) ago", "", hours );
            else
                return QObject::tr( "%n hour(s)", "", hours );
        }

        if ( mins > 1 )
        {
            if ( appendAgoString )
                return QObject::tr( "%1 minutes ago" ).arg( mins );
            else
                return QObject::tr( "%1 minutes" ).arg( mins );
        }
    }

    return QObject::tr( "just now" );
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
        s_ext2mime.insert( "oga",  "application/ogg" );
        s_ext2mime.insert( "mpc",  "audio/x-musepack" );
        s_ext2mime.insert( "wma",  "audio/x-ms-wma" );
        s_ext2mime.insert( "aac",  "audio/mp4" );
        s_ext2mime.insert( "m4a",  "audio/mp4" );
        s_ext2mime.insert( "mp4",  "audio/mp4" );
        s_ext2mime.insert( "flac", "audio/flac" );
        s_ext2mime.insert( "aiff", "audio/aiff" );
        s_ext2mime.insert( "aif",  "audio/aiff" );
    }

    return s_ext2mime.value( extension, "unknown" );
}


void
msleep( unsigned int ms )
{
  #ifdef WIN32
    Sleep( ms );
  #else
    ::usleep( ms * 1000 );
  #endif
}


int
levenshtein( const QString& source, const QString& target )
{
    // Step 1
    const int n = source.length();
    const int m = target.length();

    if ( n == 0 )
        return m;
    if ( m == 0 )
        return n;

    // Good form to declare a TYPEDEF
    typedef QVector< QVector<int> > Tmatrix;
    Tmatrix matrix;
    matrix.resize( n + 1 );

    // Size the vectors in the 2.nd dimension. Unfortunately C++ doesn't
    // allow for allocation on declaration of 2.nd dimension of vec of vec
    for ( int i = 0; i <= n; i++ )
    {
        QVector<int> tmp;
        tmp.resize( m + 1 );
        matrix.insert( i, tmp );
    }

    // Step 2
    for ( int i = 0; i <= n; i++ )
        matrix[i][0] = i;
    for ( int j = 0; j <= m; j++ )
        matrix[0][j] = j;

    // Step 3
    for ( int i = 1; i <= n; i++ )
    {
        const QChar s_i = source[i - 1];

        // Step 4
        for ( int j = 1; j <= m; j++ )
        {
            const QChar t_j = target[j - 1];

            // Step 5
            int cost;
            if ( s_i == t_j )
                cost = 0;
            else
                cost = 1;

            // Step 6
            const int above = matrix[i - 1][j];
            const int left = matrix[i][j - 1];
            const int diag = matrix[i - 1][j - 1];

            int cell = ( ( ( left + 1 ) > ( diag + cost ) ) ? diag + cost : left + 1 );
            if ( above + 1 < cell )
                cell = above + 1;

            // Step 6A: Cover transposition, in addition to deletion,
            // insertion and substitution. This step is taken from:
            // Berghel, Hal ; Roach, David : "An Extension of Ukkonen's
            // Enhanced Dynamic Programming ASM Algorithm"
            // (http://www.acm.org/~hlb/publications/asm/asm.html)
            if ( i > 2 && j > 2 )
            {
                int trans = matrix[i - 2][j - 2] + 1;

                if ( source[ i - 2 ] != t_j ) trans++;
                if ( s_i != target[ j - 2 ] ) trans++;
                if ( cell > trans ) cell = trans;
            }
            matrix[i][j] = cell;
        }
    }

    // Step 7
    return matrix[n][m];
}


static QMutex s_noProxyHostsMutex;
static QStringList s_noProxyHosts;

NetworkProxyFactory::NetworkProxyFactory( const NetworkProxyFactory& other )
{
    m_proxy = QNetworkProxy( other.m_proxy );
}


QList< QNetworkProxy >
NetworkProxyFactory::queryProxy( const QNetworkProxyQuery& query )
{
    //tDebug() << Q_FUNC_INFO << "query hostname is" << query.peerHostName() << ", proxy host is" << m_proxy.hostName();

    QList< QNetworkProxy > proxies;
    QString hostname = query.peerHostName();
    s_noProxyHostsMutex.lock();
    if ( !hostname.isEmpty() && s_noProxyHosts.contains( hostname ) )
        proxies << QNetworkProxy::NoProxy << systemProxyForQuery( query );
    else if ( m_proxy.hostName().isEmpty() || TomahawkSettings::instance()->proxyType() == QNetworkProxy::NoProxy )
        proxies << systemProxyForQuery( query );
    else
        proxies << m_proxy << systemProxyForQuery( query );
    s_noProxyHostsMutex.unlock();
    return proxies;
}


void
NetworkProxyFactory::setNoProxyHosts( const QStringList& hosts )
{
    QStringList newList;
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "No-proxy hosts:" << hosts;
    foreach ( const QString& host, hosts )
    {
        QString munge = host.simplified();
        newList << munge;
        //TODO: wildcard support
    }

    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "New no-proxy hosts:" << newList;

    s_noProxyHostsMutex.lock();
    s_noProxyHosts = newList;
    s_noProxyHostsMutex.unlock();
}


void
NetworkProxyFactory::setProxy( const QNetworkProxy& proxy )
{
    m_proxyChanged = false;
    if ( m_proxy != proxy )
        m_proxyChanged = true;

    m_proxy = proxy;
    QFlags< QNetworkProxy::Capability > proxyCaps;
    proxyCaps |= QNetworkProxy::TunnelingCapability;
    proxyCaps |= QNetworkProxy::ListeningCapability;
    if ( TomahawkSettings::instance()->proxyDns() )
        proxyCaps |= QNetworkProxy::HostNameLookupCapability;

    m_proxy.setCapabilities( proxyCaps );
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Proxy using host" << proxy.hostName() << "and port" << proxy.port();
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "setting proxy to use proxy DNS?" << (TomahawkSettings::instance()->proxyDns() ? "true" : "false");
}


NetworkProxyFactory&
NetworkProxyFactory::operator=( const NetworkProxyFactory& rhs )
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO;
    if ( this != &rhs )
    {
        m_proxy = QNetworkProxy( rhs.m_proxy );
    }

    return *this;
}


bool NetworkProxyFactory::operator==( const NetworkProxyFactory& other ) const
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO;
    if ( m_proxy != other.m_proxy )
        return false;

    return true;
}

static QMap< QThread*, QNetworkAccessManager* > s_threadNamHash;
static QMap< QThread*, NetworkProxyFactory* > s_threadProxyFactoryHash;
static QMutex s_namAccessMutex;

NetworkProxyFactory*
proxyFactory( bool makeClone, bool noMutexLocker )
{
    // Don't lock if being called from nam()
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO;
    QMutex otherMutex;
    QMutexLocker locker( noMutexLocker ? &otherMutex : &s_namAccessMutex );

    if ( !makeClone )
    {
        if ( s_threadProxyFactoryHash.contains( QThread::currentThread() ) )
            return s_threadProxyFactoryHash[ QThread::currentThread() ];
    }

    // create a new proxy factory for this thread
    TomahawkUtils::NetworkProxyFactory *newProxyFactory = new TomahawkUtils::NetworkProxyFactory();
    if ( s_threadProxyFactoryHash.contains( QCoreApplication::instance()->thread() ) )
    {
        TomahawkUtils::NetworkProxyFactory *mainProxyFactory = s_threadProxyFactoryHash[ QCoreApplication::instance()->thread() ];
        *newProxyFactory = *mainProxyFactory;
    }

    if ( !makeClone )
        s_threadProxyFactoryHash[ QThread::currentThread() ] = newProxyFactory;

    return newProxyFactory;
}


void
setProxyFactory( NetworkProxyFactory* factory, bool noMutexLocker )
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO;
    Q_ASSERT( factory );
    // Don't lock if being called from setNam()
    QMutex otherMutex;
    QMutexLocker locker( noMutexLocker ? &otherMutex : &s_namAccessMutex );

    if ( !s_threadProxyFactoryHash.contains( QCoreApplication::instance()->thread() ) )
        return;

    if ( QThread::currentThread() == QCoreApplication::instance()->thread() )
    {
        foreach ( QThread* thread, s_threadProxyFactoryHash.keys() )
        {
            if ( thread != QThread::currentThread() )
            {
                TomahawkUtils::NetworkProxyFactory *currFactory = s_threadProxyFactoryHash[ thread ];
                *currFactory = *factory;
            }
        }
        QNetworkProxyFactory::setApplicationProxyFactory( factory );
    }

    *s_threadProxyFactoryHash[ QThread::currentThread() ] = *factory;
}


QNetworkAccessManager*
nam()
{
    QMutexLocker locker( &s_namAccessMutex );
    if ( s_threadNamHash.contains(  QThread::currentThread() ) )
    {
        //tDebug() << Q_FUNC_INFO << "Found current thread in nam hash";
        return s_threadNamHash[ QThread::currentThread() ];
    }

    if ( !s_threadNamHash.contains( QCoreApplication::instance()->thread() ) )
    {
        if ( QThread::currentThread() == QCoreApplication::instance()->thread() )
        {
            setNam( new QNetworkAccessManager(), true );
            return s_threadNamHash[ QThread::currentThread() ];
        }
        else
            return 0;
    }
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Found gui thread in nam hash";

    // Create a nam for this thread based on the main thread's settings but with its own proxyfactory
    QNetworkAccessManager *mainNam = s_threadNamHash[ QCoreApplication::instance()->thread() ];
    QNetworkAccessManager* newNam = new QNetworkAccessManager();

    newNam->setConfiguration( QNetworkConfiguration( mainNam->configuration() ) );
    newNam->setNetworkAccessible( mainNam->networkAccessible() );
    newNam->setProxyFactory( proxyFactory( false, true ) );

    s_threadNamHash[ QThread::currentThread() ] = newNam;

    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "created new nam for thread" << QThread::currentThread();
    //QNetworkProxy proxy = dynamic_cast< TomahawkUtils::NetworkProxyFactory* >( newNam->proxyFactory() )->proxy();
    //tDebug() << Q_FUNC_INFO << "reply proxy properties:" << proxy.type() << proxy.hostName() << proxy.port();

    return newNam;
}


void
setNam( QNetworkAccessManager* nam, bool noMutexLocker )
{
    Q_ASSERT( nam );
    // Don't lock if being called from nam()()
    QMutex otherMutex;
    QMutexLocker locker( noMutexLocker ? &otherMutex : &s_namAccessMutex );
    if ( !s_threadNamHash.contains( QCoreApplication::instance()->thread() ) &&
            QThread::currentThread() == QCoreApplication::instance()->thread() )
    {
        tDebug( LOGVERBOSE ) << "creating initial gui thread (" << QCoreApplication::instance()->thread() << ") nam";
        // Should only get here on first initialization of the nam
        TomahawkSettings *s = TomahawkSettings::instance();
        TomahawkUtils::NetworkProxyFactory* proxyFactory = new TomahawkUtils::NetworkProxyFactory();
        if ( s->proxyType() != QNetworkProxy::NoProxy && !s->proxyHost().isEmpty() )
        {
            tDebug( LOGVERBOSE ) << "Setting proxy to saved values";
            QNetworkProxy proxy( s->proxyType(), s->proxyHost(), s->proxyPort(), s->proxyUsername(), s->proxyPassword() );
            proxyFactory->setProxy( proxy );
            //FIXME: Jreen is broke without this
            //QNetworkProxy::setApplicationProxy( proxy );
            s_noProxyHostsMutex.lock();
            if ( !s->proxyNoProxyHosts().isEmpty() && s_noProxyHosts.isEmpty() )
            {
                s_noProxyHostsMutex.unlock();
                proxyFactory->setNoProxyHosts( s->proxyNoProxyHosts().split( ',', QString::SkipEmptyParts ) );
            }
            else
                s_noProxyHostsMutex.unlock();
        }

        QNetworkProxyFactory::setApplicationProxyFactory( proxyFactory );
        nam->setProxyFactory( proxyFactory );
        s_threadNamHash[ QThread::currentThread() ] = nam;
        s_threadProxyFactoryHash[ QThread::currentThread() ] = proxyFactory;
        return;
    }

    s_threadNamHash[ QThread::currentThread() ] = nam;

    if ( QThread::currentThread() == QCoreApplication::instance()->thread() )
        setProxyFactory( dynamic_cast< TomahawkUtils::NetworkProxyFactory* >( nam->proxyFactory() ), true );
}


bool
newerVersion( const QString& oldVersion, const QString& newVersion )
{
    if ( oldVersion.isEmpty() || newVersion.isEmpty() )
        return false;

    QStringList oldVList = oldVersion.split( ".", QString::SkipEmptyParts );
    QStringList newVList = newVersion.split( ".", QString::SkipEmptyParts );

    int i = 0;
    foreach ( const QString& nvPart, newVList )
    {
        if ( i + 1 > oldVList.count() )
            return true;

        int nviPart = nvPart.toInt();
        int oviPart = oldVList.at( i++ ).toInt();

        if ( nviPart > oviPart )
            return true;

        if ( nviPart < oviPart )
            return false;
    }

    return false;
}


QList< Tomahawk::query_ptr >
mergePlaylistChanges( const QList< Tomahawk::query_ptr >& orig, const QList< Tomahawk::query_ptr >& newTracks, bool& changed )
{
    int sameCount = 0;
    QList< Tomahawk::query_ptr > tosave = newTracks;
    changed = false;

    foreach ( const Tomahawk::query_ptr& newquery, newTracks )
    {
        foreach ( const Tomahawk::query_ptr& oldq, orig )
        {
            if ( newquery->track() == oldq->track() &&
                newquery->artist() == oldq->artist() &&
                newquery->album() == oldq->album() )
            {
                sameCount++;
                if ( tosave.contains( newquery ) )
                    tosave.replace( tosave.indexOf( newquery ), oldq );

                break;
            }
        }
    }

    // No work to be done if all are the same
    if ( orig.size() == newTracks.size() && sameCount == orig.size() )
        return orig;

    changed = true;
    return tosave;
}


// taken from util/fileutils.cpp in kdevplatform
bool
removeDirectory( const QString& dir )
{
    const QDir aDir( dir );

    tLog() << "Deleting DIR:" << dir;
    bool has_err = false;
    if ( aDir.exists() )
    {
        foreach ( const QFileInfo& entry, aDir.entryInfoList( QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files | QDir::NoSymLinks ) )
        {
            QString path = entry.absoluteFilePath();
            if ( entry.isDir() )
            {
                has_err = !removeDirectory( path ) || has_err;
            }
            else if ( !QFile::remove( path ) )
            {
                has_err = true;
            }
        }
        if ( !aDir.rmdir( aDir.absolutePath() ) )
        {
            has_err = true;
        }
    }

    return !has_err;
}


quint64
infosystemRequestId()
{
    QMutexLocker locker( &s_infosystemRequestIdMutex );
    quint64 result = s_infosystemRequestId;
    s_infosystemRequestId++;
    return result;
}


QString
md5( const QByteArray& data )
{
    QByteArray const digest = QCryptographicHash::hash( data, QCryptographicHash::Md5 );
    return QString::fromLatin1( digest.toHex() ).rightJustified( 32, '0' );
}


void
crash()
{
    volatile int* a = (int*)(NULL);
    *a = 1;
}


void
installTranslator( QObject* parent )
{
#if QT_VERSION >= 0x040800
    QString locale = QLocale::system().uiLanguages().first().replace( "-", "_" );
#else
    QString locale = QLocale::system().name();
#endif
    if ( locale == "C" )
        locale = "en";

    // Tomahawk translations
    QTranslator* translator = new QTranslator( parent );
    if ( translator->load( QString( ":/lang/tomahawk_" ) + locale ) )
    {
        qDebug() << "Translation: Tomahawk: Using system locale:" << locale;
    }
    else
    {
        qDebug() << "Translation: Tomahawk: Using default locale, system locale one not found:" << locale;
        translator->load( QString( ":/lang/tomahawk_en" ) );
    }

    QCoreApplication::installTranslator( translator );

    // Qt translations
    translator = new QTranslator( parent );
    if ( translator->load( QString( ":/lang/qt_" ) + locale ) )
    {
        qDebug() << "Translation: Qt: Using system locale:" << locale;
    }
    else
    {
        qDebug() << "Translation: Qt: Using default locale, system locale one not found:" << locale;
    }

    QCoreApplication::installTranslator( translator );
}


bool
verifyFile( const QString& filePath, const QString& signature )
{
    QCA::Initializer init;

    if ( !QCA::isSupported( "sha1" ) )
    {
        qWarning() << "SHA1 not supported by QCA, aborting.";
        return false;
    }

    // The signature for the resolver.zip was created like so:
    // openssl dgst -sha1 -binary < "#{tarball}" | openssl dgst -dss1 -sign "#{ARGV[2]}" | openssl enc -base64
    // which means we need to decode it with QCA's DSA public key signature verification tools
    // The input data is:
    // file -> SHA1 binary format -> DSS1/DSA signed -> base64 encoded.

    // Step 1: Load the public key
    // Public key is in :/data/misc/tomahawk_pubkey.pem
    QFile f( ":/data/misc/tomahawk_pubkey.pem" );
    if ( !f.open( QIODevice::ReadOnly ) )
    {
        qWarning() << "Unable to read public key from resources!";
        return false;
    }

    const QString pubkeyData = QString::fromUtf8( f.readAll() );
    QCA::ConvertResult conversionResult;
    QCA::PublicKey publicKey = QCA::PublicKey::fromPEM( pubkeyData, &conversionResult );
    if ( QCA::ConvertGood != conversionResult)
    {
        qWarning() << "Public key reading/loading failed! Tried to load public key:" << pubkeyData;
        return false;
    }

    if ( !publicKey.canVerify() )
    {
        qWarning() << "Loaded Tomahawk public key but cannot use it to verify! What is up....";
        return false;
    }

    // Step 2: Get the SHA1 of the file contents
    QFile toVerify( filePath );
    if ( !toVerify.exists() || !toVerify.open( QIODevice::ReadOnly ) )
    {
        qWarning() << "Failed to open file we are trying to verify!" << filePath;
        return false;
    }

    const QByteArray fileHashData = QCA::Hash( "sha1" ).hash( toVerify.readAll() ).toByteArray();
    toVerify.close();

    // Step 3: Base64 decode the signature
    QCA::Base64 decoder( QCA::Decode );
    const QByteArray decodedSignature = decoder.decode( QCA::SecureArray( signature.trimmed().toUtf8() ) ).toByteArray();
    if ( decodedSignature.isEmpty() )
    {
        qWarning() << "Got empty signature after we tried to decode it from Base64:" << signature.trimmed().toUtf8() << decodedSignature.toBase64();
        return false;
    }

    // Step 4: Do the actual verifying!
    const bool result = publicKey.verifyMessage( fileHashData, decodedSignature, QCA::EMSA1_SHA1, QCA::DERSequence );
    if ( !result )
    {
        qWarning() << "File" << filePath << "FAILED VERIFICATION against our input signature!";
        return false;
    }

    tDebug( LOGVERBOSE ) << "Successfully verified signature of downloaded file:" << filePath;

    return true;
}


QString
extractScriptPayload( const QString& filename, const QString& resolverId )
{
    // uses QuaZip to extract the temporary zip file to the user's tomahawk data/resolvers directory
    QDir resolverDir = appDataDir();
    if ( !resolverDir.mkpath( QString( "atticaresolvers/%1" ).arg( resolverId ) ) )
    {
        tLog() << "Failed to mkdir resolver save dir:" << TomahawkUtils::appDataDir().absoluteFilePath( QString( "atticaresolvers/%1" ).arg( resolverId ) );
        return QString();
    }
    resolverDir.cd( QString( "atticaresolvers/%1" ).arg( resolverId ) );

    if ( !unzipFileInFolder( filename, resolverDir ) )
    {
        qWarning() << "Failed to unzip resolver. Ooops.";
        return QString();
    }

    return resolverDir.absolutePath();
}


bool
unzipFileInFolder( const QString& zipFileName, const QDir& folder )
{
    Q_ASSERT( !zipFileName.isEmpty() );
    Q_ASSERT( folder.exists() );

    QuaZip zipFile( zipFileName );
    if ( !zipFile.open( QuaZip::mdUnzip ) )
    {
        qWarning() << "Failed to QuaZip open:" << zipFile.getZipError();
        return false;
    }

    if ( !zipFile.goToFirstFile() )
    {
        tLog() << "Failed to go to first file in zip archive:" << zipFile.getZipError();
        return false;
    }

    tDebug( LOGVERBOSE ) << "Unzipping files to:" << folder.absolutePath();

    QuaZipFile fileInZip( &zipFile );
    do
    {
        QuaZipFileInfo info;
        zipFile.getCurrentFileInfo( &info );

        if ( !fileInZip.open( QIODevice::ReadOnly ) )
        {
            tLog() << "Failed to open file inside zip archive:" << info.name << zipFile.getZipName() << "with error:" << zipFile.getZipError();
            continue;
        }

        QFile out( folder.absoluteFilePath( fileInZip.getActualFileName() ) );

        // make dir if there is one needed
        QStringList parts = fileInZip.getActualFileName().split( "/" );
        if ( parts.size() > 1 )
        {
            QStringList dirs = parts.mid( 0, parts.size() - 1 );
            QString dirPath = dirs.join( "/" ); // QDir translates / to \ internally if necessary
            folder.mkpath( dirPath );
        }

        tDebug( LOGVERBOSE ) << "Writing to output file..." << out.fileName();
        if ( !out.open( QIODevice::WriteOnly ) )
        {
            tLog() << "Failed to open zip extract file:" << out.errorString() << info.name;
            fileInZip.close();
            continue;
        }


        out.write( fileInZip.readAll() );
        out.close();
        fileInZip.close();

    } while ( zipFile.goToNextFile() );

    return true;
}


void
extractBinaryResolver( const QString& zipFilename, QObject* receiver )
{
    BinaryExtractWorker* worker = new BinaryExtractWorker( zipFilename, receiver );
    worker->start( QThread::LowPriority );
}


bool
whitelistedHttpResultHint( const QString& url )
{
    // For now, just http/https
    return url.startsWith( "http" );
}


} // ns

#include "TomahawkUtils.moc"
