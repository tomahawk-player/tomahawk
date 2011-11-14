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
#include "infoplugins/generic/spotifyPlugin.h"
#include "infoplugins/generic/lastfmplugin.h"
#include "infoplugins/generic/musicbrainzPlugin.h"
#include "infoplugins/generic/hypemPlugin.h"
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
#include "infoplugins/generic/RoviPlugin.h"

namespace Tomahawk
{

namespace InfoSystem
{

InfoSystemWorker::InfoSystemWorker()
    : QObject()
{
    tDebug() << Q_FUNC_INFO;

    m_checkTimeoutsTimer.setInterval( 1000 );
    m_checkTimeoutsTimer.setSingleShot( false );
    connect( &m_checkTimeoutsTimer, SIGNAL( timeout() ), SLOT( checkTimeoutsTimerFired() ) );
    m_checkTimeoutsTimer.start();
}


InfoSystemWorker::~InfoSystemWorker()
{
    tDebug() << Q_FUNC_INFO << " beginning";
    Q_FOREACH( InfoPluginPtr plugin, m_plugins )
    {
        if( plugin )
            delete plugin.data();
    }
    tDebug() << Q_FUNC_INFO << " finished";
}


void
InfoSystemWorker::init( Tomahawk::InfoSystem::InfoSystemCache* cache )
{
    tDebug() << Q_FUNC_INFO;

    InfoPluginPtr enptr( new EchoNestPlugin() );
    m_plugins.append( enptr );
    registerInfoTypes( enptr, enptr.data()->supportedGetTypes(), enptr.data()->supportedPushTypes() );
    InfoPluginPtr mmptr( new MusixMatchPlugin() );
    m_plugins.append( mmptr );
    registerInfoTypes( mmptr, mmptr.data()->supportedGetTypes(), mmptr.data()->supportedPushTypes() );
    InfoPluginPtr mbptr( new MusicBrainzPlugin() );
    m_plugins.append( mbptr );
    registerInfoTypes( mbptr, mbptr.data()->supportedGetTypes(), mbptr.data()->supportedPushTypes() );
    InfoPluginPtr lfmptr( new LastFmPlugin() );
    m_plugins.append( lfmptr );
    registerInfoTypes( lfmptr, lfmptr.data()->supportedGetTypes(), lfmptr.data()->supportedPushTypes() );
    InfoPluginPtr sptr( new ChartsPlugin() );
    m_plugins.append( sptr );
    registerInfoTypes( sptr, sptr.data()->supportedGetTypes(), sptr.data()->supportedPushTypes() );
    InfoPluginPtr roviptr( new RoviPlugin() );
    m_plugins.append( roviptr );
    registerInfoTypes( roviptr, roviptr.data()->supportedGetTypes(), roviptr.data()->supportedPushTypes() );
    InfoPluginPtr spotptr( new SpotifyPlugin() );
    m_plugins.append( spotptr );
    registerInfoTypes( spotptr, spotptr.data()->supportedGetTypes(), spotptr.data()->supportedPushTypes() );
    InfoPluginPtr hypeptr( new hypemPlugin() );
    m_plugins.append( hypeptr );
    registerInfoTypes( hypeptr, hypeptr.data()->supportedGetTypes(), hypeptr.data()->supportedPushTypes() );


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
                SIGNAL( info( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ),
                this,
                SLOT( infoSlot( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ),
                Qt::UniqueConnection
            );

        connect(
                plugin.data(),
                SIGNAL( getCachedInfo( Tomahawk::InfoSystem::InfoStringHash, qint64, Tomahawk::InfoSystem::InfoRequestData ) ),
                cache,
                SLOT( getCachedInfoSlot( Tomahawk::InfoSystem::InfoStringHash, qint64, Tomahawk::InfoSystem::InfoRequestData ) )
            );
        connect(
                plugin.data(),
                SIGNAL( updateCache( Tomahawk::InfoSystem::InfoStringHash, qint64, Tomahawk::InfoSystem::InfoType, QVariant ) ),
                cache,
                SLOT( updateCacheSlot( Tomahawk::InfoSystem::InfoStringHash, qint64, Tomahawk::InfoSystem::InfoType, QVariant ) )
            );
    }
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
InfoSystemWorker::getInfo( Tomahawk::InfoSystem::InfoRequestData requestData )
{
    //qDebug() << Q_FUNC_INFO << "type is " << requestData.type << " and allSources = " << (allSources ? "true" : "false" );

    QList< InfoPluginPtr > providers = determineOrderedMatches( requestData.type );
    if ( providers.isEmpty() )
    {
        emit info( requestData, QVariant() );
        checkFinished( requestData );
        return;
    }

    if ( !requestData.allSources )
        providers = QList< InfoPluginPtr >( providers.mid( 0, 1 ) );

    bool foundOne = false;
    foreach ( InfoPluginPtr ptr, providers )
    {
        if ( !ptr )
            continue;

        foundOne = true;

        if ( requestData.allSources || m_savedRequestMap.contains( requestData.requestId ) )
        {
            if ( m_savedRequestMap.contains( requestData.requestId ) )
                tDebug() << Q_FUNC_INFO << "Warning: reassigning requestId because it already exists";
            requestData.internalId = TomahawkUtils::infosystemRequestId();
        }
        else
            requestData.internalId = requestData.requestId;

        quint64 requestId = requestData.internalId;
        m_requestSatisfiedMap[ requestId ] = false;
        if ( requestData.timeoutMillis != 0 )
        {
            qint64 currMs = QDateTime::currentMSecsSinceEpoch();
            m_timeRequestMapper.insert( currMs + requestData.timeoutMillis, requestId );
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

        QMetaObject::invokeMethod( ptr.data(), "getInfo", Qt::QueuedConnection, Q_ARG( Tomahawk::InfoSystem::InfoRequestData, requestData ) );
    }

    if ( !foundOne )
    {
        emit info( requestData, QVariant() );
        checkFinished( requestData );
    }
}


void
InfoSystemWorker::pushInfo( QString caller, Tomahawk::InfoSystem::InfoType type, QVariant input )
{
//    qDebug() << Q_FUNC_INFO;

    Q_FOREACH( InfoPluginPtr ptr, m_infoPushMap[ type ] )
    {
        if( ptr )
            QMetaObject::invokeMethod( ptr.data(), "pushInfo", Qt::QueuedConnection, Q_ARG( QString, caller ), Q_ARG( Tomahawk::InfoSystem::InfoType, type ), Q_ARG( QVariant, input ) );
    }
}


void
InfoSystemWorker::infoSlot( Tomahawk::InfoSystem::InfoRequestData requestData, QVariant output )
{
//    qDebug() << Q_FUNC_INFO << "with requestId" << requestId;

    quint64 requestId = requestData.internalId;

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
    checkFinished( requestData );
}


void
InfoSystemWorker::checkFinished( const Tomahawk::InfoSystem::InfoRequestData &requestData )
{
    if ( m_dataTracker[ requestData.caller ][ requestData.type ] == 0 )
        emit finished( requestData.caller, requestData.type );
    
    Q_FOREACH( InfoType testtype, m_dataTracker[ requestData.caller ].keys() )
    {
        if ( m_dataTracker[ requestData.caller ][ testtype ] != 0 )
            return;
    }
//    qDebug() << "Emitting finished with target" << target;
    emit finished( requestData.caller );
}


void
InfoSystemWorker::checkTimeoutsTimerFired()
{
    qint64 currTime = QDateTime::currentMSecsSinceEpoch();
    Q_FOREACH( qint64 time, m_timeRequestMapper.keys() )
    {
        Q_FOREACH( quint64 requestId, m_timeRequestMapper.values( time ) )
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

                checkFinished( returnData );
            }
            else
            {
                //we've caught up, the remaining requets still have time to work
                return;
            }
        }
    }
}


} //namespace InfoSystem

} //namespace Tomahawk
