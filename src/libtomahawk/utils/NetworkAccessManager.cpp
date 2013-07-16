/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 * 
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright 2013,      Dominik Schmidt <domme@tomahawk-player.org>
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
#include "NetworkAccessManager.h"

#include "NetworkProxyFactory.h"
#include "utils/Logger.h"

#include <QMutex>
#include <QStringList>
#include <QThread>
#include <QCoreApplication>
#include <QNetworkConfiguration>
#include <QNetworkAccessManager>

namespace Tomahawk
{
namespace Utils
{

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
    else if ( m_proxy.hostName().isEmpty() || proxyType() == QNetworkProxy::NoProxy )
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
NetworkProxyFactory::setProxy( const QNetworkProxy& proxy, bool useProxyDns )
{
    m_proxyChanged = false;
    if ( m_proxy != proxy )
        m_proxyChanged = true;
    
    m_proxy = proxy;
    QFlags< QNetworkProxy::Capability > proxyCaps;
    proxyCaps |= QNetworkProxy::TunnelingCapability;
    proxyCaps |= QNetworkProxy::ListeningCapability;
    if ( useProxyDns )
        proxyCaps |= QNetworkProxy::HostNameLookupCapability;
    
    m_proxy.setCapabilities( proxyCaps );
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Proxy using host" << proxy.hostName() << "and port" << proxy.port();
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "setting proxy to use proxy DNS?" << useProxyDns;
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
    NetworkProxyFactory *newProxyFactory = new NetworkProxyFactory();
    if ( s_threadProxyFactoryHash.contains( QCoreApplication::instance()->thread() ) )
    {
        NetworkProxyFactory *mainProxyFactory = s_threadProxyFactoryHash[ QCoreApplication::instance()->thread() ];
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
                NetworkProxyFactory *currFactory = s_threadProxyFactoryHash[ thread ];
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
    //QNetworkProxy proxy = dynamic_cast< NetworkProxyFactory* >( newNam->proxyFactory() )->proxy();
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

        NetworkProxyFactory* proxyFactory = new NetworkProxyFactory();
        if ( proxyType() != QNetworkProxy::NoProxy && !proxyHost().isEmpty() )
        {
            tDebug( LOGVERBOSE ) << "Setting proxy to saved values";
            QNetworkProxy proxy( proxyType(), proxyHost(), proxyPort(), proxyUsername(), proxyPassword() );
            proxyFactory->setProxy( proxy, proxyDns() );
            //FIXME: Jreen is broke without this
            //QNetworkProxy::setApplicationProxy( proxy );
            s_noProxyHostsMutex.lock();
            if ( !proxyNoProxyHosts().isEmpty() && s_noProxyHosts.isEmpty() )
            {
                s_noProxyHostsMutex.unlock();
                proxyFactory->setNoProxyHosts( proxyNoProxyHosts().split( ',', QString::SkipEmptyParts ) );
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
        setProxyFactory( dynamic_cast< NetworkProxyFactory* >( nam->proxyFactory() ), true );
}


static bool s_proxyDns;
static QNetworkProxy::ProxyType s_proxyType;
static QString s_proxyHost;
static int s_proxyPort;
static QString s_proxyUsername;
static QString s_proxyPassword;
static QString s_proxyNoProxyHosts;

QString
proxyHost()
{
    return s_proxyHost;
}


void
setProxyHost( const QString& host )
{
    s_proxyHost = host;
}


QString
proxyNoProxyHosts()
{
    return s_proxyNoProxyHosts;
}


void
setProxyNoProxyHosts( const QString& hosts )
{
    s_proxyNoProxyHosts = hosts;
}


qulonglong
proxyPort()
{
    return s_proxyPort;
}


void
setProxyPort( const qulonglong port )
{
    s_proxyPort = port;
}


QString
proxyUsername()
{
    return s_proxyUsername;
}


void
setProxyUsername( const QString& username )
{
    s_proxyUsername = username;
}


QString
proxyPassword()
{
    return s_proxyPassword;
}


void
setProxyPassword( const QString& password )
{
    s_proxyPassword = password;
}


QNetworkProxy::ProxyType
proxyType()
{
    return s_proxyType;
}


void
setProxyType( const QNetworkProxy::ProxyType type )
{
    s_proxyType = type;
}


bool
proxyDns()
{
    return s_proxyDns;
}


void
setProxyDns( bool proxyDns )
{
    s_proxyDns = proxyDns;
}


}
}
