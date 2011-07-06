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
#include "utils/tomahawkutils.h"
#include "infosystemcache.h"
#include "infosystemworker.h"

namespace Tomahawk
{

namespace InfoSystem
{

InfoPlugin::InfoPlugin()
    : QObject()
{
    qDebug() << Q_FUNC_INFO;
}

InfoPlugin::~InfoPlugin()
{
    qDebug() << Q_FUNC_INFO;
}

InfoSystem* InfoSystem::s_instance = 0;

InfoSystem*
InfoSystem::instance()
{
    return s_instance;
}


InfoSystem::InfoSystem(QObject *parent)
    : QObject(parent)
    , m_infoSystemCacheThreadController( 0 )
    , m_infoSystemWorkerThreadController( 0 )
{
    s_instance = this;

    qDebug() << Q_FUNC_INFO;

    m_infoSystemCacheThreadController = new QThread( this );
    m_cache = QWeakPointer< InfoSystemCache >( new InfoSystemCache() );
    m_cache.data()->moveToThread( m_infoSystemCacheThreadController );
    m_infoSystemCacheThreadController->start( QThread::IdlePriority );

    m_infoSystemWorkerThreadController = new QThread( this );
    m_worker = QWeakPointer< InfoSystemWorker>( new InfoSystemWorker() );
    m_worker.data()->moveToThread( m_infoSystemWorkerThreadController );
    m_infoSystemWorkerThreadController->start();

    QMetaObject::invokeMethod( m_worker.data(), "init", Qt::QueuedConnection, Q_ARG( QWeakPointer< Tomahawk::InfoSystem::InfoSystemCache >, m_cache ) );

    connect( TomahawkSettings::instance(), SIGNAL( changed() ), SLOT( newNam() ) );

    connect( m_cache.data(), SIGNAL( info( uint, QString, Tomahawk::InfoSystem::InfoType, QVariant, QVariant, QVariantMap ) ),
             m_worker.data(), SLOT( infoSlot( uint, QString, Tomahawk::InfoSystem::InfoType, QVariant, QVariant, QVariantMap ) ), Qt::UniqueConnection );

    connect( m_worker.data(), SIGNAL( info( QString, Tomahawk::InfoSystem::InfoType, QVariant, QVariant, QVariantMap ) ),
             this,       SIGNAL( info( QString, Tomahawk::InfoSystem::InfoType, QVariant, QVariant, QVariantMap ) ), Qt::UniqueConnection );
    connect( m_worker.data(), SIGNAL( finished( QString ) ), this, SIGNAL( finished( QString) ), Qt::UniqueConnection );
}

InfoSystem::~InfoSystem()
{
    qDebug() << Q_FUNC_INFO << " beginning";

    if ( !m_worker.isNull() )
    {
        m_infoSystemWorkerThreadController->quit();
        m_infoSystemWorkerThreadController->wait( 60000 );

        delete m_worker.data();
        delete m_infoSystemWorkerThreadController;
        m_infoSystemWorkerThreadController = 0;
    }
    qDebug() << Q_FUNC_INFO << " done deleting worker";

    if( m_infoSystemCacheThreadController )
    {
        m_infoSystemCacheThreadController->quit();
        m_infoSystemCacheThreadController->wait( 60000 );

        delete m_cache.data();
        delete m_infoSystemCacheThreadController;
        m_infoSystemCacheThreadController = 0;
    }

    qDebug() << Q_FUNC_INFO << " done deleting cache";
}


void
InfoSystem::newNam() const
{
    qDebug() << Q_FUNC_INFO;
    QMetaObject::invokeMethod( m_worker.data(), "newNam", Qt::QueuedConnection );
}


void
InfoSystem::getInfo( const QString &caller, const InfoType type, const QVariant& input, QVariantMap customData, uint timeoutMillis )
{
    qDebug() << Q_FUNC_INFO;
    QMetaObject::invokeMethod( m_worker.data(), "getInfo", Qt::QueuedConnection, Q_ARG( QString, caller ), Q_ARG( Tomahawk::InfoSystem::InfoType, type ), Q_ARG( QVariant, input ), Q_ARG( QVariantMap, customData ), Q_ARG( uint, timeoutMillis ) );
}


void
InfoSystem::getInfo( const QString &caller, const InfoTypeMap &inputMap, QVariantMap customData, const InfoTimeoutMap &timeoutMap )
{
    Q_FOREACH( InfoType type, inputMap.keys() )
        QMetaObject::invokeMethod( m_worker.data(), "getInfo", Qt::QueuedConnection, Q_ARG( QString, caller ), Q_ARG( Tomahawk::InfoSystem::InfoType, type ), Q_ARG( QVariant, inputMap[ type ] ), Q_ARG( QVariantMap, customData ), Q_ARG( uint, ( timeoutMap.contains( type ) ? timeoutMap[ type ] : 3000 ) ) );
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

} //namespace InfoSystem

} //namespace Tomahawk
