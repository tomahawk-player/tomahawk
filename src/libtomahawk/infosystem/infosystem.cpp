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
#include "tomahawksettings.h"
#include "infosystemcache.h"
#include "infosystemworker.h"
#include "utils/tomahawkutils.h"
#include "utils/logger.h"

namespace Tomahawk
{

namespace InfoSystem
{

InfoPlugin::InfoPlugin()
    : QObject()
{
}

InfoPlugin::~InfoPlugin()
{
}

InfoSystem* InfoSystem::s_instance = 0;

InfoSystem*
InfoSystem::instance()
{
    return s_instance;
}


InfoSystem::InfoSystem( QObject *parent )
    : QObject( parent )
    , m_infoSystemCacheThreadController( 0 )
    , m_infoSystemWorkerThreadController( 0 )
{
    s_instance = this;

    qDebug() << Q_FUNC_INFO;

    m_infoSystemCacheThreadController = new InfoSystemCacheThread( this );
    m_cache = QWeakPointer< InfoSystemCache >( new InfoSystemCache() );
    m_cache.data()->moveToThread( m_infoSystemCacheThreadController );
    m_infoSystemCacheThreadController->setCache( m_cache );
    m_infoSystemCacheThreadController->start( QThread::IdlePriority );

    m_infoSystemWorkerThreadController = new InfoSystemWorkerThread( this );
    m_worker = QWeakPointer< InfoSystemWorker >( new InfoSystemWorker() );
    m_worker.data()->moveToThread( m_infoSystemWorkerThreadController );
    m_infoSystemWorkerThreadController->setWorker( m_worker );
    m_infoSystemWorkerThreadController->start();

    QMetaObject::invokeMethod( m_worker.data(), "init", Qt::QueuedConnection, Q_ARG( QWeakPointer< Tomahawk::InfoSystem::InfoSystemCache >, m_cache ) );

    connect( m_cache.data(), SIGNAL( info( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ),
             m_worker.data(), SLOT( infoSlot( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ), Qt::UniqueConnection );

    connect( m_worker.data(), SIGNAL( info( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ),
             this,       SIGNAL( info( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ), Qt::UniqueConnection );

    connect( m_worker.data(), SIGNAL( finished( QString ) ), this, SIGNAL( finished( QString ) ), Qt::UniqueConnection );

    connect( m_worker.data(), SIGNAL( finished( QString, Tomahawk::InfoSystem::InfoType ) ),
             this, SIGNAL( finished( QString, Tomahawk::InfoSystem::InfoType ) ), Qt::UniqueConnection );
}

InfoSystem::~InfoSystem()
{
    qDebug() << Q_FUNC_INFO << " beginning";

    if ( !m_worker.isNull() )
    {
        m_infoSystemWorkerThreadController->quit();
        m_infoSystemWorkerThreadController->wait( 60000 );

        //delete m_worker.data();
        delete m_infoSystemWorkerThreadController;
        m_infoSystemWorkerThreadController = 0;
    }
    qDebug() << Q_FUNC_INFO << " done deleting worker";

    if( m_infoSystemCacheThreadController )
    {
        m_infoSystemCacheThreadController->quit();
        m_infoSystemCacheThreadController->wait( 60000 );

        //delete m_cache.data();
        delete m_infoSystemCacheThreadController;
        m_infoSystemCacheThreadController = 0;
    }

    qDebug() << Q_FUNC_INFO << " done deleting cache";
}


void
InfoSystem::getInfo( const InfoRequestData &requestData )
{
    qDebug() << Q_FUNC_INFO;
    QMetaObject::invokeMethod( m_worker.data(), "getInfo", Qt::QueuedConnection, Q_ARG( Tomahawk::InfoSystem::InfoRequestData, requestData ) );
}


void
InfoSystem::getInfo( const QString &caller, const QVariantMap &customData, const InfoTypeMap &inputMap, const InfoTimeoutMap &timeoutMap, bool allSources )
{
    InfoRequestData requestData;
    requestData.caller = caller;
    requestData.customData = customData;
    requestData.allSources = allSources;
    Q_FOREACH( InfoType type, inputMap.keys() )
    {
        requestData.type = type;
        requestData.input = inputMap[ type ];
        requestData.timeoutMillis = timeoutMap.contains( type ) ? timeoutMap[ type ] : 10000;
        QMetaObject::invokeMethod( m_worker.data(), "getInfo", Qt::QueuedConnection, Q_ARG( Tomahawk::InfoSystem::InfoRequestData, requestData ) );
    }
}


void
InfoSystem::pushInfo( const QString &caller, const InfoType type, const QVariant& input )
{
    qDebug() << Q_FUNC_INFO;
    QMetaObject::invokeMethod( m_worker.data(), "pushInfo", Qt::QueuedConnection, Q_ARG( QString, caller ), Q_ARG( Tomahawk::InfoSystem::InfoType, type ), Q_ARG( QVariant, input ) );
}


void
InfoSystem::pushInfo( const QString &caller, const InfoTypeMap &input )
{
    Q_FOREACH( InfoType type, input.keys() )
        QMetaObject::invokeMethod( m_worker.data(), "pushInfo", Qt::QueuedConnection, Q_ARG( QString, caller ), Q_ARG( Tomahawk::InfoSystem::InfoType, type ), Q_ARG( QVariant, input[ type ] ) );
}


InfoSystemCacheThread::InfoSystemCacheThread( QObject *parent )
    : QThread( parent )
{
}

InfoSystemCacheThread::~InfoSystemCacheThread()
{
    delete m_cache.data();
}

void
InfoSystemCacheThread::InfoSystemCacheThread::run()
{
    exec();
}

QWeakPointer< InfoSystemCache >
InfoSystemCacheThread::cache() const
{
    return m_cache;
}

void
InfoSystemCacheThread::setCache( QWeakPointer< InfoSystemCache >  cache )
{
    m_cache = cache;
}

InfoSystemWorkerThread::InfoSystemWorkerThread( QObject *parent )
    : QThread( parent )
{
}

InfoSystemWorkerThread::~InfoSystemWorkerThread()
{
}

void
InfoSystemWorkerThread::InfoSystemWorkerThread::run()
{
    exec();
    if( m_worker )
        delete m_worker.data();
}

QWeakPointer< InfoSystemWorker >
InfoSystemWorkerThread::worker() const
{
    return m_worker;
}

void
InfoSystemWorkerThread::setWorker( QWeakPointer< InfoSystemWorker >  worker )
{
    m_worker = worker;
}


} //namespace InfoSystem

} //namespace Tomahawk
