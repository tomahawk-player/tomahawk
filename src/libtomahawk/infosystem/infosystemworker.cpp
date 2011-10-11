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

#include "config.h"
#include "infosystemworker.h"
#include "infosystemcache.h"
#include "infoplugins/generic/echonestplugin.h"
#include "infoplugins/generic/musixmatchplugin.h"
#include "infoplugins/generic/chartsplugin.h"
#include "infoplugins/generic/lastfmplugin.h"
#include "infoplugins/generic/musicbrainzPlugin.h"
#include "utils/tomahawkutils.h"
#include "utils/logger.h"

#ifdef Q_WS_MAC
#include "infoplugins/mac/adiumplugin.h"
#endif
#ifdef Q_WS_X11
#include "infoplugins/unix/fdonotifyplugin.h"
#include "infoplugins/unix/mprisplugin.h"
#endif

#include "lastfm/NetworkAccessManager"

namespace Tomahawk
{

namespace InfoSystem
{

InfoSystemWorker::InfoSystemWorker()
    : QObject()
    , m_nextRequest( 0 )
{
//    qDebug() << Q_FUNC_INFO;

    m_checkTimeoutsTimer.setInterval( 1000 );
    m_checkTimeoutsTimer.setSingleShot( false );
    connect( &m_checkTimeoutsTimer, SIGNAL( timeout() ), SLOT( checkTimeoutsTimerFired() ) );
    m_checkTimeoutsTimer.start();
}


InfoSystemWorker::~InfoSystemWorker()
{
//    qDebug() << Q_FUNC_INFO << " beginning";
    Q_FOREACH( InfoPluginPtr plugin, m_plugins )
    {
        if( plugin )
            delete plugin.data();
    }
//    qDebug() << Q_FUNC_INFO << " finished";
}


void
InfoSystemWorker::init( QWeakPointer< Tomahawk::InfoSystem::InfoSystemCache> cache )
{
//    qDebug() << Q_FUNC_INFO << "and cache is" << cache.data();

    InfoPluginPtr enptr( new EchoNestPlugin() );
    m_plugins.append( enptr );
    registerInfoTypes( enptr, enptr.data()->supportedGetTypes(), enptr.data()->supportedPushTypes() );
    InfoPluginPtr mmptr( new MusixMatchPlugin() );
    m_plugins.append( mmptr );
    registerInfoTypes( mmptr, mmptr.data()->supportedGetTypes(), mmptr.data()->supportedPushTypes() );
    InfoPluginPtr mbptr( new MusicBrainzPlugin() );
    m_plugins.append( mbptr );
    registerInfoTypes( mbptr, mbptr.data()->supportedGetTypes(), mbptr.data()->supportedPushTypes() );

    InfoPluginPtr sptr( new ChartsPlugin() );
    m_plugins.append( sptr );
    registerInfoTypes( sptr, sptr.data()->supportedGetTypes(), sptr.data()->supportedPushTypes() );

    InfoPluginPtr lfmptr( new LastFmPlugin() );
    m_plugins.append( lfmptr );
    registerInfoTypes( lfmptr, lfmptr.data()->supportedGetTypes(), lfmptr.data()->supportedPushTypes() );


    #ifdef Q_WS_MAC
    InfoPluginPtr admptr( new AdiumPlugin() );
    m_plugins.append( admptr );
    registerInfoTypes( admptr, admptr.data()->supportedGetTypes(), admptr.data()->supportedPushTypes() );
    #endif
    #ifdef Q_WS_X11
    InfoPluginPtr fdonotifyptr( new FdoNotifyPlugin() );
    m_plugins.append( fdonotifyptr );
    registerInfoTypes( fdonotifyptr, fdonotifyptr.data()->supportedGetTypes(), fdonotifyptr.data()->supportedPushTypes() );
    InfoPluginPtr mprisptr( new MprisPlugin() );
    m_plugins.append( mprisptr );
    registerInfoTypes( mprisptr, mprisptr.data()->supportedGetTypes(), mprisptr.data()->supportedPushTypes() );    
    #endif

    Q_FOREACH( InfoPluginPtr plugin, m_plugins )
    {
        connect(
                plugin.data(),
                SIGNAL( info( uint, Tomahawk::InfoSystem::InfoRequestData, QVariant ) ),
                this,
                SLOT( infoSlot( uint, Tomahawk::InfoSystem::InfoRequestData, QVariant ) ),
                Qt::UniqueConnection
            );

        connect(
                plugin.data(),
                SIGNAL( getCachedInfo( uint, Tomahawk::InfoSystem::InfoCriteriaHash, qint64, Tomahawk::InfoSystem::InfoRequestData ) ),
                cache.data(),
                SLOT( getCachedInfoSlot( uint, Tomahawk::InfoSystem::InfoCriteriaHash, qint64, Tomahawk::InfoSystem::InfoRequestData ) )
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
    Q_FOREACH( InfoType type, getTypes )
        m_infoGetMap[type].append( plugin );
    Q_FOREACH( InfoType type, pushTypes )
        m_infoPushMap[type].append( plugin );
}


QList< InfoPluginPtr >
InfoSystemWorker::determineOrderedMatches( const InfoType type ) const
{
    //Dummy function for now that returns the various items in the QSet; at some point this will
    //probably need to support ordering based on the data source
    QList< InfoPluginPtr > providers;
    Q_FOREACH( InfoPluginPtr ptr, m_infoGetMap[type] )
        providers << ptr;
    return providers;
}


void
InfoSystemWorker::getInfo( Tomahawk::InfoSystem::InfoRequestData requestData, uint timeoutMillis, bool allSources )
{
//    qDebug() << Q_FUNC_INFO;

    QList< InfoPluginPtr > providers = determineOrderedMatches( requestData.type );
    if ( providers.isEmpty() )
    {
        emit info( requestData, QVariant() );
        checkFinished( requestData.caller );
        return;
    }

    if ( !allSources )
        providers = QList< InfoPluginPtr >( providers.mid( 0, 1 ) );

    bool foundOne = false;
    foreach ( InfoPluginPtr ptr, providers )
    {
        if ( !ptr )
            continue;

        foundOne = true;
        uint requestId = ++m_nextRequest;
        m_requestSatisfiedMap[ requestId ] = false;
        if ( timeoutMillis != 0 )
        {
            qint64 currMs = QDateTime::currentMSecsSinceEpoch();
            m_timeRequestMapper.insert( currMs + timeoutMillis, requestId );
        }
    //    qDebug() << "Assigning request with requestId" << requestId << "and type" << requestData.type;
        m_dataTracker[ requestData.caller ][ requestData.type ] = m_dataTracker[ requestData.caller ][ requestData.type ] + 1;
    //    qDebug() << "Current count in dataTracker for target" << requestData.caller << "and type" << requestData.type << "is" << m_dataTracker[ requestData.caller ][ requestData.type ];

        InfoRequestData* data = new InfoRequestData;
        data->caller = requestData.caller;
        data->type = requestData.type;
        data->input = requestData.input;
        data->customData = requestData.customData;
        m_savedRequestMap[ requestId ] = data;

        QMetaObject::invokeMethod( ptr.data(), "getInfo", Qt::QueuedConnection, Q_ARG( uint, requestId ), Q_ARG( Tomahawk::InfoSystem::InfoRequestData, requestData ) );
    }

    if ( !foundOne )
    {
        emit info( requestData, QVariant() );
        checkFinished( requestData.caller );
    }
}


void
InfoSystemWorker::pushInfo( QString caller, InfoType type, QVariant input )
{
//    qDebug() << Q_FUNC_INFO;

    Q_FOREACH( InfoPluginPtr ptr, m_infoPushMap[ type ] )
    {
        if( ptr )
            QMetaObject::invokeMethod( ptr.data(), "pushInfo", Qt::QueuedConnection, Q_ARG( QString, caller ), Q_ARG( Tomahawk::InfoSystem::InfoType, type ), Q_ARG( QVariant, input ) );
    }
}


void
InfoSystemWorker::infoSlot( uint requestId, Tomahawk::InfoSystem::InfoRequestData requestData, QVariant output )
{
//    qDebug() << Q_FUNC_INFO << "with requestId" << requestId;

    if ( m_dataTracker[ requestData.caller ][ requestData.type ] == 0 )
    {
//        qDebug() << Q_FUNC_INFO << "Caller was not waiting for that type of data!";
        return;
    }
    if ( !m_requestSatisfiedMap.contains( requestId ) || m_requestSatisfiedMap[ requestId ] )
    {
//        qDebug() << Q_FUNC_INFO << "Request was already taken care of!";
        return;
    }

    m_requestSatisfiedMap[ requestId ] = true;
    emit info( requestData, output );

    m_dataTracker[ requestData.caller ][ requestData.type ] = m_dataTracker[ requestData.caller ][ requestData.type ] - 1;
//    qDebug() << "Current count in dataTracker for target" << requestData.caller << "and type" << requestData.type << "is" << m_dataTracker[ requestData.caller ][ requestData.type ];
    delete m_savedRequestMap[ requestId ];
    m_savedRequestMap.remove( requestId );
    checkFinished( requestData.caller );
}


void
InfoSystemWorker::checkFinished( const QString &target )
{
    Q_FOREACH( InfoType testtype, m_dataTracker[ target ].keys() )
    {
        if ( m_dataTracker[ target ][ testtype ] != 0)
        {
//            qDebug() << "Found outstanding request of type" << testtype;
            return;
        }
    }
//    qDebug() << "Emitting finished with target" << target;
    emit finished( target );
}


void
InfoSystemWorker::checkTimeoutsTimerFired()
{
    qint64 currTime = QDateTime::currentMSecsSinceEpoch();
    Q_FOREACH( qint64 time, m_timeRequestMapper.keys() )
    {
        Q_FOREACH( uint requestId, m_timeRequestMapper.values( time ) )
        {
            if ( time < currTime )
            {
                if ( m_requestSatisfiedMap[ requestId ] )
                {
//                    qDebug() << Q_FUNC_INFO << "Removing mapping of" << requestId << "which expired at time" << time << "and was already satisfied";
                    m_timeRequestMapper.remove( time, requestId );
                    if ( !m_timeRequestMapper.count( time ) )
                        m_timeRequestMapper.remove( time );
                    continue;
                }

                //doh, timed out
//                qDebug() << Q_FUNC_INFO << "Doh, timed out for requestId" << requestId;
                InfoRequestData *savedData = m_savedRequestMap[ requestId ];

                InfoRequestData returnData;
                returnData.caller = savedData->caller;
                returnData.type = savedData->type;
                returnData.input = savedData->input;
                returnData.customData = savedData->customData;
                emit info( returnData, QVariant() );

                delete savedData;
                m_savedRequestMap.remove( requestId );

                m_dataTracker[ returnData.caller ][ returnData.type ] = m_dataTracker[ returnData.caller ][ returnData.type ] - 1;
//                qDebug() << "Current count in dataTracker for target" << returnData.caller << "is" << m_dataTracker[ returnData.caller ][ returnData.type ];

                m_requestSatisfiedMap[ requestId ] = true;
                m_timeRequestMapper.remove( time, requestId );
                if ( !m_timeRequestMapper.count( time ) )
                    m_timeRequestMapper.remove( time );

                checkFinished( returnData.caller );
            }
            else
            {
                //we've caught up, the remaining requets still have time to work
                return;
            }
        }
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
//    qDebug() << Q_FUNC_INFO << " begin";

    QNetworkAccessManager *oldNam = TomahawkUtils::nam();
    if ( oldNam && oldNam->thread() == thread() )
    {
//        qDebug() << Q_FUNC_INFO << "Using old nam as it's the same thread (GUI) as me";
        m_nam = QWeakPointer< QNetworkAccessManager >( oldNam );
        emit namChanged( m_nam.data() );
        return;
    }

//    qDebug() << Q_FUNC_INFO << "No nam exists, or it's a different thread, creating a new one";
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
