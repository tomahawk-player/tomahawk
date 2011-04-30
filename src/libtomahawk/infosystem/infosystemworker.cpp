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

namespace Tomahawk
{

namespace InfoSystem
{

InfoSystemWorker::InfoSystemWorker()
    : m_nam( 0 )
{
    InfoPluginPtr enptr( new EchoNestPlugin( this ) );
    m_plugins.append( enptr );
    InfoPluginPtr mmptr( new MusixMatchPlugin( this ) );
    m_plugins.append( mmptr );
    InfoPluginPtr lfmptr( new LastFmPlugin( this ) );
    m_plugins.append( lfmptr );

    InfoSystemCache *cache = InfoSystem::instance()->getCache();
    
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
                cache,
                SLOT( getCachedInfoSlot( Tomahawk::InfoSystem::InfoCriteriaHash, qint64, QString, Tomahawk::InfoSystem::InfoType, QVariant, Tomahawk::InfoSystem::InfoCustomData ) )
            );
        connect(
                cache,
                SIGNAL( notInCache( Tomahawk::InfoSystem::InfoCriteriaHash, QString, Tomahawk::InfoSystem::InfoType, QVariant, Tomahawk::InfoSystem::InfoCustomData ) ),
                plugin.data(),
                SLOT( notInCacheSlot( Tomahawk::InfoSystem::InfoCriteriaHash, QString, Tomahawk::InfoSystem::InfoType, QVariant, Tomahawk::InfoSystem::InfoCustomData ) )
            );
        connect(
                plugin.data(),
                SIGNAL( updateCache( Tomahawk::InfoSystem::InfoCriteriaHash, qint64, Tomahawk::InfoSystem::InfoType, QVariant ) ),
                cache,
                SLOT( updateCacheSlot( Tomahawk::InfoSystem::InfoCriteriaHash, qint64, Tomahawk::InfoSystem::InfoType, QVariant ) )
            );
    }
}


InfoSystemWorker::~InfoSystemWorker()
{
    qDebug() << Q_FUNC_INFO;
    Q_FOREACH( InfoPluginPtr plugin, m_plugins )
    {
        if( plugin )
            delete plugin.data();
    }
}


void
InfoSystemWorker::registerInfoTypes(const InfoPluginPtr &plugin, const QSet< InfoType >& types)
{
    qDebug() << Q_FUNC_INFO;
    Q_FOREACH(InfoType type, types)
        m_infoMap[type].append(plugin);
}


QLinkedList< InfoPluginPtr >
InfoSystemWorker::determineOrderedMatches(const InfoType type) const
{
    //Dummy function for now that returns the various items in the QSet; at some point this will
    //probably need to support ordering based on the data source
    QLinkedList< InfoPluginPtr > providers;
    Q_FOREACH(InfoPluginPtr ptr, m_infoMap[type])
        providers << ptr;
    return providers;
}


void
InfoSystemWorker::getInfo( QString caller, InfoType type, QVariant input, InfoCustomData customData )
{
    qDebug() << Q_FUNC_INFO;
    QLinkedList< InfoPluginPtr > providers = determineOrderedMatches(type);
    if (providers.isEmpty())
    {
        emit info(caller, type, QVariant(), QVariant(), customData);
        return;
    }

    InfoPluginPtr ptr = providers.first();
    if (!ptr)
    {
        emit info(caller, type, QVariant(), QVariant(), customData);
        return;
    }

    QMetaObject::invokeMethod( ptr.data(), "getInfo", Qt::QueuedConnection, Q_ARG( QString, caller ), Q_ARG( Tomahawk::InfoSystem::InfoType, type ), Q_ARG( QVariant, input ), Q_ARG( Tomahawk::InfoSystem::InfoCustomData, customData ) );
}


QNetworkAccessManager*
InfoSystemWorker::nam() const
{
    return m_nam;
}


void
InfoSystemWorker::newNam()
{
    QNetworkAccessManager *newNam = new QNetworkAccessManager( this );
    if ( m_nam )
    {
        delete m_nam;
    }
    QNetworkAccessManager *oldNam = TomahawkUtils::nam();
    if ( !oldNam )
    {
        m_nam = newNam;
        return;
    }
    newNam->setCache( oldNam->cache() );
    newNam->setConfiguration( oldNam->configuration() );
    newNam->setCookieJar( oldNam->cookieJar() );
    newNam->setNetworkAccessible( oldNam->networkAccessible() );
    newNam->setProxy( oldNam->proxy() );
    newNam->setProxyFactory( oldNam->proxyFactory() );
    m_nam = newNam;
}


} //namespace InfoSystem

} //namespace Tomahawk
