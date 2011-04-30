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

#include "infosystem.h"
#include "utils/tomahawkutils.h"
#include "infosystemcache.h"
#include "infoplugins/echonestplugin.h"
#include "infoplugins/musixmatchplugin.h"
#include "infoplugins/lastfmplugin.h"

namespace Tomahawk
{

namespace InfoSystem
{

InfoPlugin::InfoPlugin( InfoSystemWorker *parent )
    :QObject( parent )
{
    qDebug() << Q_FUNC_INFO;
}


InfoSystemWorker::InfoSystemWorker()
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


InfoSystem* InfoSystem::s_instance = 0;

InfoSystem*
InfoSystem::instance()
{
    return s_instance;
}


InfoSystem::InfoSystem(QObject *parent)
    : QObject(parent)
{
    s_instance = this;

    qDebug() << Q_FUNC_INFO;

    m_infoSystemCacheThreadController = new QThread( this );
    m_cache = new InfoSystemCache();
    m_cache->moveToThread( m_infoSystemCacheThreadController );
    m_infoSystemCacheThreadController->start( QThread::IdlePriority );

    m_infoSystemWorkerThreadController = new QThread( this );
    m_worker = new InfoSystemWorker();
    m_worker->moveToThread( m_infoSystemWorkerThreadController );
    m_infoSystemWorkerThreadController->start();
    
    connect( m_cache, SIGNAL( info( QString, Tomahawk::InfoSystem::InfoType, QVariant, QVariant, Tomahawk::InfoSystem::InfoCustomData ) ),
            this,       SLOT( infoSlot( QString, Tomahawk::InfoSystem::InfoType, QVariant, QVariant, Tomahawk::InfoSystem::InfoCustomData ) ), Qt::UniqueConnection );

    connect( m_worker, SIGNAL( info( QString, Tomahawk::InfoSystem::InfoType, QVariant, QVariant, Tomahawk::InfoSystem::InfoCustomData ) ),
            this,       SLOT( infoSlot( QString, Tomahawk::InfoSystem::InfoType, QVariant, QVariant, Tomahawk::InfoSystem::InfoCustomData ) ), Qt::UniqueConnection );
}

InfoSystem::~InfoSystem()
{
    qDebug() << Q_FUNC_INFO;

    if ( m_infoSystemCacheThreadController )
        m_infoSystemCacheThreadController->quit();
    if ( m_infoSystemWorkerThreadController )
        m_infoSystemWorkerThreadController->quit();
    
    if( m_infoSystemCacheThreadController )
    {
        while( !m_infoSystemCacheThreadController->isFinished() )
        {
            QCoreApplication::processEvents( QEventLoop::AllEvents, 200 );
            TomahawkUtils::Sleep::msleep( 100 );
        }

        if( m_cache )
        {
            delete m_cache;
            m_cache = 0;
        }

        delete m_infoSystemCacheThreadController;
        m_infoSystemCacheThreadController = 0;
    }

    if( m_infoSystemWorkerThreadController )
    {
        while( !m_infoSystemWorkerThreadController->isFinished() )
        {
            QCoreApplication::processEvents( QEventLoop::AllEvents, 200 );
            TomahawkUtils::Sleep::msleep( 100 );
        }

        if( m_worker )
        {
            delete m_worker;
            m_worker = 0;
        }

        delete m_infoSystemWorkerThreadController;
        m_infoSystemWorkerThreadController = 0;
    }
}


void InfoSystem::getInfo(const QString &caller, const InfoType type, const QVariant& input, InfoCustomData customData)
{
    qDebug() << Q_FUNC_INFO;

    m_dataTracker[caller][type] = m_dataTracker[caller][type] + 1;
    qDebug() << "current count in dataTracker for type" << type << "is" << m_dataTracker[caller][type];
    QMetaObject::invokeMethod( m_worker, "getInfo", Qt::QueuedConnection, Q_ARG( QString, caller ), Q_ARG( Tomahawk::InfoSystem::InfoType, type ), Q_ARG( QVariant, input ), Q_ARG( Tomahawk::InfoSystem::InfoCustomData, customData ) );
}


void InfoSystem::getInfo(const QString &caller, const InfoMap &input, InfoCustomData customData)
{
    Q_FOREACH( InfoType type, input.keys() )
        getInfo(caller, type, input[type], customData);
}


void InfoSystem::infoSlot(QString target, InfoType type, QVariant input, QVariant output, InfoCustomData customData)
{
    qDebug() << Q_FUNC_INFO;
    qDebug() << "current count in dataTracker is " << m_dataTracker[target][type];
    if (m_dataTracker[target][type] == 0)
    {
        qDebug() << "Caller was not waiting for that type of data!";
        return;
    }
    emit info(target, type, input, output, customData);

    m_dataTracker[target][type] = m_dataTracker[target][type] - 1;
    qDebug() << "current count in dataTracker is " << m_dataTracker[target][type];
    Q_FOREACH(InfoType testtype, m_dataTracker[target].keys())
    {
        if (m_dataTracker[target][testtype] != 0)
        {
            qDebug() << "found outstanding request of type" << testtype;
            return;
        }
    }
    qDebug() << "emitting finished with target" << target;
    emit finished(target);
}

} //namespace InfoSystem

} //namespace Tomahawk
