/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2012       Leo Franchi            <lfranchi@kde.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include "InfoSystem.h"
#include "TomahawkSettings.h"
#include "InfoSystemCache.h"
#include "InfoSystemWorker.h"
#include "utils/TomahawkUtils.h"
#include "utils/Logger.h"
#include "Source.h"

namespace Tomahawk
{

namespace InfoSystem
{

static const int DEFAULT_TIMEOUT_MILLIS = 10000;

InfoRequestData::InfoRequestData()
    : requestId( TomahawkUtils::infosystemRequestId() )
{
    init( QString() , Tomahawk::InfoSystem::InfoNoInfo, QVariant(), QVariantMap() );
}


InfoRequestData::InfoRequestData( const quint64 rId, const QString& callr, const InfoType typ, const QVariant& inputvar, const QVariantMap& custom )
    : requestId( rId )
{
    init( callr, typ, inputvar, custom );
}


void
InfoRequestData::init( const QString& callr, const InfoType typ, const QVariant& inputvar, const QVariantMap& custom )
{
    internalId = TomahawkUtils::infosystemRequestId();
    caller = callr;
    type = typ;
    input = inputvar;
    customData = custom;
    timeoutMillis = DEFAULT_TIMEOUT_MILLIS;
    allSources = false;
}


InfoPlugin::InfoPlugin()
    : QObject()
{
}


InfoPlugin::~InfoPlugin()
{
}


void
InfoPlugin::setFriendlyName( const QString& friendlyName )
{
    m_friendlyName = friendlyName;
}

const QString
InfoPlugin::friendlyName() const
{
    return m_friendlyName;
}


InfoSystem* InfoSystem::s_instance = 0;

InfoSystem*
InfoSystem::instance()
{
    if ( !s_instance )
        s_instance = new InfoSystem( 0 );

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

    if ( m_infoSystemWorkerThreadController )
    {
        m_infoSystemWorkerThreadController->quit();
        m_infoSystemWorkerThreadController->wait( 60000 );

        delete m_infoSystemWorkerThreadController;
        m_infoSystemWorkerThreadController = 0;
    }
    tDebug() << Q_FUNC_INFO << " done deleting worker";

    if( m_infoSystemCacheThreadController )
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
    if ( m_inited )
        return;

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

    qRegisterMetaType< Tomahawk::InfoSystem::InfoTypeSet >();
    connect( worker, SIGNAL( updatedSupportedGetTypes( Tomahawk::InfoSystem::InfoTypeSet ) ),
             this,   SLOT(   receiveUpdatedSupportedGetTypes( Tomahawk::InfoSystem::InfoTypeSet ) ) );
    connect( worker, SIGNAL( updatedSupportedPushTypes( Tomahawk::InfoSystem::InfoTypeSet ) ),
             this,   SLOT(   receiveUpdatedSupportedPushTypes( Tomahawk::InfoSystem::InfoTypeSet ) ) );

    QMetaObject::invokeMethod( worker, "init", Qt::QueuedConnection, Q_ARG( Tomahawk::InfoSystem::InfoSystemCache*, cache ) );

    m_inited = true;
}


bool
InfoSystem::getInfo( const InfoRequestData &requestData )
{
    //qDebug() << Q_FUNC_INFO;
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
        requestData.timeoutMillis = timeoutMap.contains( type ) ? timeoutMap[ type ] : DEFAULT_TIMEOUT_MILLIS;
        QMetaObject::invokeMethod( m_infoSystemWorkerThreadController->worker(), "getInfo", Qt::QueuedConnection, Q_ARG( Tomahawk::InfoSystem::InfoRequestData, requestData ) );
    }
    return false;
}


bool
InfoSystem::pushInfo( InfoPushData pushData )
{
    tDebug() << Q_FUNC_INFO << "type is " << pushData.type;
    if ( !m_inited || !m_infoSystemWorkerThreadController->worker() )
    {
        init();
        return false;
    }

    PushInfoPair pushInfoPair( QVariantMap(), pushData.input );
    pushData.infoPair = pushInfoPair;
    QMetaObject::invokeMethod( m_infoSystemWorkerThreadController->worker(), "pushInfo", Qt::QueuedConnection, Q_ARG( Tomahawk::InfoSystem::InfoPushData, pushData ) );

    return true;
}


bool
InfoSystem::pushInfo( const QString &caller, const InfoTypeMap &input, const PushInfoFlags pushFlags )
{
    if ( !m_inited || !m_infoSystemWorkerThreadController->worker() )
    {
        init();
        return false;
    }

    Q_FOREACH( InfoType type, input.keys() )
    {
        InfoPushData pushData( caller, type, input[ type ], pushFlags );
        pushData.infoPair = PushInfoPair( QVariantMap(), pushData.input );
        QMetaObject::invokeMethod( m_infoSystemWorkerThreadController->worker(), "pushInfo", Qt::QueuedConnection, Q_ARG( Tomahawk::InfoSystem::InfoPushData, pushData ) );
    }

    return true;
}


void
InfoSystem::addInfoPlugin( Tomahawk::InfoSystem::InfoPluginPtr plugin )
{
    // Init is not complete (waiting for worker thread to start and create worker object) so keep trying till then
    if ( !m_inited || !m_infoSystemWorkerThreadController->worker() )
    {
        QMetaObject::invokeMethod( this, "addInfoPlugin", Qt::QueuedConnection, Q_ARG( Tomahawk::InfoSystem::InfoPluginPtr, plugin ) );
        return;
    }

    if ( plugin.isNull() )
    {
        tDebug() << Q_FUNC_INFO << "Given plugin is null!";
        return;
    }

    if ( plugin.data()->thread() != m_infoSystemWorkerThreadController->worker()->thread() )
    {
        Q_ASSERT( false );
        tDebug() << Q_FUNC_INFO << "The object must be moved to the worker thread first, see InfoSystem::workerThread()";
        return;
    }

    tDebug() << Q_FUNC_INFO << plugin.data();
    QMetaObject::invokeMethod( m_infoSystemWorkerThreadController->worker(), "addInfoPlugin", Qt::QueuedConnection, Q_ARG( Tomahawk::InfoSystem::InfoPluginPtr, plugin ) );
}


void
InfoSystem::removeInfoPlugin( Tomahawk::InfoSystem::InfoPluginPtr plugin )
{
    // Init is not complete (waiting for worker th read to start and create worker object) so keep trying till then
    if ( !m_inited || !m_infoSystemWorkerThreadController->worker() )
    {
        QMetaObject::invokeMethod( this, "removeInfoPlugin", Qt::QueuedConnection, Q_ARG( Tomahawk::InfoSystem::InfoPluginPtr, plugin ) );
        return;
    }

    if ( plugin.isNull() )
    {
        tDebug() << Q_FUNC_INFO << "Given plugin is null!";
        return;
    }

    if ( plugin.data()->thread() != m_infoSystemWorkerThreadController->worker()->thread() )
    {
        tDebug() << Q_FUNC_INFO << "The object must be moved to the worker thread first, see InfoSystem::workerThread()";
        return;
    }

    tDebug() << Q_FUNC_INFO << plugin.data();
    QMetaObject::invokeMethod( m_infoSystemWorkerThreadController->worker(), "removeInfoPlugin", Qt::QueuedConnection, Q_ARG( Tomahawk::InfoSystem::InfoPluginPtr, plugin ) );
}


void
InfoSystem::receiveUpdatedSupportedGetTypes( InfoTypeSet supportedTypes )
{
    m_supportedGetTypes = supportedTypes;
    emit updatedSupportedGetTypes( m_supportedGetTypes );
}


void
InfoSystem::receiveUpdatedSupportedPushTypes( InfoTypeSet supportedTypes )
{
    m_supportedPushTypes = supportedTypes;
    emit updatedSupportedPushTypes( m_supportedPushTypes );
}


QPointer< QThread >
InfoSystem::workerThread() const
{
    if ( m_infoSystemWorkerThreadController->isRunning() && m_infoSystemWorkerThreadController->worker() )
        return QPointer< QThread >( m_infoSystemWorkerThreadController->worker()->thread() );

    return QPointer< QThread >();
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
    m_cache = QPointer< InfoSystemCache >( new InfoSystemCache() );
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
    m_worker = QPointer< InfoSystemWorker >( new InfoSystemWorker() );
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
