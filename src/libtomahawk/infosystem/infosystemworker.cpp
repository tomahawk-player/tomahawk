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

#include <QCoreApplication>
#include <QNetworkConfiguration>
#include <QNetworkProxy>

#include "infosystemworker.h"
#include "utils/tomahawkutils.h"
#include "infosystemcache.h"
#include "infoplugins/echonestplugin.h"
#include "infoplugins/musixmatchplugin.h"
#include "infoplugins/lastfmplugin.h"

#include "lastfm/NetworkAccessManager"

namespace Tomahawk
{

namespace InfoSystem
{

InfoSystemWorker::InfoSystemWorker()
{
    qDebug() << Q_FUNC_INFO;
}


InfoSystemWorker::~InfoSystemWorker()
{
    qDebug() << Q_FUNC_INFO << " beginning";
    Q_FOREACH( InfoPluginPtr plugin, m_plugins )
    {
        if( plugin )
            delete plugin.data();
    }
}


void
InfoSystemWorker::init( QWeakPointer< Tomahawk::InfoSystem::InfoSystemCache> cache )
{
    qDebug() << Q_FUNC_INFO << " and cache is " << cache.data();
    InfoPluginPtr enptr( new EchoNestPlugin() );
    m_plugins.append( enptr );
    registerInfoTypes( enptr, enptr.data()->supportedGetTypes(), enptr.data()->supportedPushTypes() );
    InfoPluginPtr mmptr( new MusixMatchPlugin() );
    m_plugins.append( mmptr );
    registerInfoTypes( mmptr, mmptr.data()->supportedGetTypes(), mmptr.data()->supportedPushTypes() );
    InfoPluginPtr lfmptr( new LastFmPlugin() );
    m_plugins.append( lfmptr );
    registerInfoTypes( lfmptr, lfmptr.data()->supportedGetTypes(), lfmptr.data()->supportedPushTypes() );

    Q_FOREACH( InfoPluginPtr plugin, m_plugins )
    {
        connect(
                plugin.data(),
                SIGNAL( info( QString, Tomahawk::InfoSystem::InfoType, QVariant, QVariant, Tomahawk::InfoSystem::InfoCustomData ) ),
                InfoSystem::instance(),
                SLOT( infoSlot( QString, Tomahawk::InfoSystem::InfoType, QVariant, QVariant, Tomahawk::InfoSystem::InfoCustomData ) ),
                Qt::UniqueConnection
            );

        connect(
                plugin.data(),
                SIGNAL( getCachedInfo( Tomahawk::InfoSystem::InfoCriteriaHash, qint64, QString, Tomahawk::InfoSystem::InfoType, QVariant, Tomahawk::InfoSystem::InfoCustomData ) ),
                cache.data(),
                SLOT( getCachedInfoSlot( Tomahawk::InfoSystem::InfoCriteriaHash, qint64, QString, Tomahawk::InfoSystem::InfoType, QVariant, Tomahawk::InfoSystem::InfoCustomData ) )
            );
        connect(
                cache.data(),
                SIGNAL( notInCache( Tomahawk::InfoSystem::InfoCriteriaHash, QString, Tomahawk::InfoSystem::InfoType, QVariant, Tomahawk::InfoSystem::InfoCustomData ) ),
                plugin.data(),
                SLOT( notInCacheSlot( Tomahawk::InfoSystem::InfoCriteriaHash, QString, Tomahawk::InfoSystem::InfoType, QVariant, Tomahawk::InfoSystem::InfoCustomData ) )
            );
        connect(
                plugin.data(),
                SIGNAL( updateCache( Tomahawk::InfoSystem::InfoCriteriaHash, qint64, Tomahawk::InfoSystem::InfoType, QVariant ) ),
                cache.data(),
                SLOT( updateCacheSlot( Tomahawk::InfoSystem::InfoCriteriaHash, qint64, Tomahawk::InfoSystem::InfoType, QVariant ) )
            );
        connect(
                this,
                SIGNAL( namChanged( QNetworkAccessManager* ) ),
                plugin.data(),
                SLOT( namChangedSlot( QNetworkAccessManager* ) )
            );
    }

    QMetaObject::invokeMethod( this, "newNam" );
}


void
InfoSystemWorker::registerInfoTypes( const InfoPluginPtr &plugin, const QSet< InfoType >& getTypes, const QSet< InfoType >& pushTypes )
{
    qDebug() << Q_FUNC_INFO;
    Q_FOREACH( InfoType type, getTypes )
        m_infoGetMap[type].append( plugin );
    Q_FOREACH( InfoType type, pushTypes )
        m_infoPushMap[type].append( plugin );
}


QLinkedList< InfoPluginPtr >
InfoSystemWorker::determineOrderedMatches( const InfoType type ) const
{
    //Dummy function for now that returns the various items in the QSet; at some point this will
    //probably need to support ordering based on the data source
    QLinkedList< InfoPluginPtr > providers;
    Q_FOREACH( InfoPluginPtr ptr, m_infoGetMap[type] )
        providers << ptr;
    return providers;
}


void
InfoSystemWorker::getInfo( QString caller, InfoType type, QVariant input, InfoCustomData customData )
{
    qDebug() << Q_FUNC_INFO;
    QLinkedList< InfoPluginPtr > providers = determineOrderedMatches(type);
    if ( providers.isEmpty() )
    {
        emit info( caller, type, QVariant(), QVariant(), customData );
        return;
    }

    InfoPluginPtr ptr = providers.first();
    if ( !ptr )
    {
        emit info( caller, type, QVariant(), QVariant(), customData );
        return;
    }

    QMetaObject::invokeMethod( ptr.data(), "getInfo", Qt::QueuedConnection, Q_ARG( QString, caller ), Q_ARG( Tomahawk::InfoSystem::InfoType, type ), Q_ARG( QVariant, input ), Q_ARG( Tomahawk::InfoSystem::InfoCustomData, customData ) );
}


void
InfoSystemWorker::pushInfo( const QString caller, const InfoType type, const QVariant input )
{
    qDebug() << Q_FUNC_INFO;

    Q_FOREACH( InfoPluginPtr ptr, m_infoPushMap[type] )
    {
        if( ptr )
            QMetaObject::invokeMethod( ptr.data(), "pushInfo", Qt::QueuedConnection, Q_ARG( QString, caller ), Q_ARG( Tomahawk::InfoSystem::InfoType, type ), Q_ARG( QVariant, input ) );
    }
}


QNetworkAccessManager*
InfoSystemWorker::nam() const
{
    if ( m_nam.isNull() )
        return 0;
    
    return m_nam.data();
}


void
InfoSystemWorker::newNam()
{
    qDebug() << Q_FUNC_INFO << " begin";

    QNetworkAccessManager *oldNam = TomahawkUtils::nam();
    if ( oldNam && oldNam->thread() == thread() )
    {
        qDebug() << Q_FUNC_INFO << " using old nam as it's the same thread (GUI) as me";
        m_nam = QWeakPointer< QNetworkAccessManager >( oldNam );
        emit namChanged( m_nam.data() );
        return;
    }
    
    qDebug() << Q_FUNC_INFO << " no nam exists, or it's a different thread, creating a new one";
    QNetworkAccessManager* newNam;
#ifdef LIBLASTFM_FOUND
    newNam = new lastfm::NetworkAccessManager( this );
#else
    newNam = new QNetworkAccessManager( this );
#endif
    if ( !m_nam.isNull() )
        delete m_nam.data();

    if ( !oldNam )
        oldNam = new QNetworkAccessManager();

    TomahawkUtils::NetworkProxyFactory* oldProxyFactory = TomahawkUtils::proxyFactory();
    if ( !oldProxyFactory )
        oldProxyFactory = new TomahawkUtils::NetworkProxyFactory();

    newNam->setConfiguration( oldNam->configuration() );
    newNam->setNetworkAccessible( oldNam->networkAccessible() );
    TomahawkUtils::NetworkProxyFactory* newProxyFactory = new TomahawkUtils::NetworkProxyFactory();
    newProxyFactory->setNoProxyHosts( oldProxyFactory->noProxyHosts() );
    newProxyFactory->setProxy( oldProxyFactory->proxy() );
    newNam->setProxyFactory( newProxyFactory );
    m_nam = QWeakPointer< QNetworkAccessManager >( newNam );

    emit namChanged( m_nam.data() );

    //FIXME: Currently leaking nam/proxyfactory above -- how to change in a thread-safe way?
}


} //namespace InfoSystem

} //namespace Tomahawk
