/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2012       Leo Franchi            <lfranchi@kde.org>
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
    , m_inited( false )
    , m_infoSystemCacheThreadController( 0 )
    , m_infoSystemWorkerThreadController( 0 )
{
    s_instance = this;

    qDebug() << Q_FUNC_INFO;

    m_infoSystemCacheThreadController = new InfoSystemCacheThread( this );
    m_infoSystemCacheThreadController->start( QThread::IdlePriority );

    m_infoSystemWorkerThreadController = new InfoSystemWorkerThread( this );
    m_infoSystemWorkerThreadController->start();

    QTimer::singleShot( 0, this, SLOT( init() ) );
}


InfoSystem::~InfoSystem()
{
    tDebug() << Q_FUNC_INFO << " beginning";

    if ( m_infoSystemWorkerThreadController->worker() )
    {
        m_infoSystemWorkerThreadController->quit();
        m_infoSystemWorkerThreadController->wait( 60000 );

        delete m_infoSystemWorkerThreadController;
        m_infoSystemWorkerThreadController = 0;
    }
    tDebug() << Q_FUNC_INFO << " done deleting worker";

    if( m_infoSystemCacheThreadController->cache() )
    {
        m_infoSystemCacheThreadController->quit();
        m_infoSystemCacheThreadController->wait( 60000 );

        delete m_infoSystemCacheThreadController;
        m_infoSystemCacheThreadController = 0;
    }

    tDebug() << Q_FUNC_INFO << " done deleting cache";
}


void
InfoSystem::init()
{
    tDebug() << Q_FUNC_INFO;
    if ( !m_infoSystemCacheThreadController->cache() || !m_infoSystemWorkerThreadController->worker() )
    {
        QTimer::singleShot( 0, this, SLOT( init() ) );
        return;
    }

    Tomahawk::InfoSystem::InfoSystemCache* cache = m_infoSystemCacheThreadController->cache();
    Tomahawk::InfoSystem::InfoSystemWorker* worker = m_infoSystemWorkerThreadController->worker();

    connect( cache, SIGNAL( info( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ),
             worker, SLOT( infoSlot( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ), Qt::UniqueConnection );

    connect( worker, SIGNAL( info( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ),
             this,       SIGNAL( info( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ), Qt::UniqueConnection );

    connect( worker, SIGNAL( finished( QString ) ), this, SIGNAL( finished( QString ) ), Qt::UniqueConnection );

    connect( worker, SIGNAL( finished( QString, Tomahawk::InfoSystem::InfoType ) ),
             this, SIGNAL( finished( QString, Tomahawk::InfoSystem::InfoType ) ), Qt::UniqueConnection );

    QMetaObject::invokeMethod( worker, "init", Qt::QueuedConnection, Q_ARG( Tomahawk::InfoSystem::InfoSystemCache*, cache ) );

    m_inited = true;
}


bool
InfoSystem::getInfo( const InfoRequestData &requestData )
{
    qDebug() << Q_FUNC_INFO;
    if ( !m_inited || !m_infoSystemWorkerThreadController->worker() )
    {
        init();
        return false;
    }
    QMetaObject::invokeMethod( m_infoSystemWorkerThreadController->worker(), "getInfo", Qt::QueuedConnection, Q_ARG( Tomahawk::InfoSystem::InfoRequestData, requestData ) );
    return true;
}


bool
InfoSystem::getInfo( const QString &caller, const QVariantMap &customData, const InfoTypeMap &inputMap, const InfoTimeoutMap &timeoutMap, bool allSources )
{
    if ( !m_inited || !m_infoSystemWorkerThreadController->worker() )
    {
        init();
        return false;
    }
    InfoRequestData requestData;
    requestData.caller = caller;
    requestData.customData = customData;
    requestData.allSources = allSources;
    Q_FOREACH( InfoType type, inputMap.keys() )
    {
        requestData.type = type;
        requestData.input = inputMap[ type ];
        requestData.timeoutMillis = timeoutMap.contains( type ) ? timeoutMap[ type ] : 10000;
        QMetaObject::invokeMethod( m_infoSystemWorkerThreadController->worker(), "getInfo", Qt::QueuedConnection, Q_ARG( Tomahawk::InfoSystem::InfoRequestData, requestData ) );
    }
    return false;
}


bool
InfoSystem::pushInfo( const QString &caller, const InfoType type, const QVariant& input )
{
    tDebug() << Q_FUNC_INFO;
    if ( !m_inited || !m_infoSystemWorkerThreadController->worker() )
    {
        init();
        return false;
    }

    QMetaObject::invokeMethod( m_infoSystemWorkerThreadController->worker(), "pushInfo", Qt::QueuedConnection, Q_ARG( QString, caller ), Q_ARG( Tomahawk::InfoSystem::InfoType, type ), Q_ARG( QVariant, input ) );

    return true;
}


bool
InfoSystem::pushInfo( const QString &caller, const InfoTypeMap &input )
{
    if ( !m_inited || !m_infoSystemWorkerThreadController->worker() )
    {
        init();
        return false;
    }

    Q_FOREACH( InfoType type, input.keys() )
        QMetaObject::invokeMethod( m_infoSystemWorkerThreadController->worker(), "pushInfo", Qt::QueuedConnection, Q_ARG( QString, caller ), Q_ARG( Tomahawk::InfoSystem::InfoType, type ), Q_ARG( QVariant, input[ type ] ) );

    return true;
}


void
InfoSystem::addInfoPlugin( InfoPlugin* plugin )
{
    QMetaObject::invokeMethod( m_infoSystemWorkerThreadController->worker(), "addInfoPlugin", Qt::QueuedConnection, Q_ARG( Tomahawk::InfoSystem::InfoPlugin*, plugin ) );
}


InfoSystemCacheThread::InfoSystemCacheThread( QObject *parent )
    : QThread( parent )
{
    tDebug() << Q_FUNC_INFO;
}


InfoSystemCacheThread::~InfoSystemCacheThread()
{
    tDebug() << Q_FUNC_INFO;
}


void
InfoSystemCacheThread::InfoSystemCacheThread::run()
{
    m_cache = QWeakPointer< InfoSystemCache >( new InfoSystemCache() );
    exec();
    if ( !m_cache.isNull() )
        delete m_cache.data();
}


InfoSystemCache*
InfoSystemCacheThread::cache() const
{
    if ( m_cache.isNull() )
        return 0;
    return m_cache.data();
}


InfoSystemWorkerThread::InfoSystemWorkerThread( QObject *parent )
    : QThread( parent )
{
    tDebug() << Q_FUNC_INFO;
}

InfoSystemWorkerThread::~InfoSystemWorkerThread()
{
    tDebug() << Q_FUNC_INFO;
}

void
InfoSystemWorkerThread::InfoSystemWorkerThread::run()
{
    m_worker = QWeakPointer< InfoSystemWorker >( new InfoSystemWorker() );
    exec();
    if( !m_worker.isNull() )
        delete m_worker.data();
}

InfoSystemWorker*
InfoSystemWorkerThread::worker() const
{
    if ( m_worker.isNull() )
        return 0;
    return m_worker.data();
}


} //namespace InfoSystem

} //namespace Tomahawk
