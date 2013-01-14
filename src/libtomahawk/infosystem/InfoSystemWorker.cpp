/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2012       Leo Franchi <lfranchi@kde.org>
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

#include "InfoSystemWorker.h"

#include "config.h"
#include "InfoSystemCache.h"
#include "GlobalActionManager.h"
#include "utils/TomahawkUtils.h"
#include "utils/Logger.h"
#include "Source.h"


#include <QCoreApplication>
#include <QDir>
#include <QLibrary>
#include <QNetworkConfiguration>
#include <QNetworkProxy>
#include <QPluginLoader>

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
    m_shortLinksWaiting = 0;
    m_cache = cache;

    loadInfoPlugins( findInfoPlugins() );
}


void
InfoSystemWorker::addInfoPlugin( Tomahawk::InfoSystem::InfoPluginPtr plugin )
{
    tDebug() << Q_FUNC_INFO << plugin;
    foreach ( InfoPluginPtr ptr, m_plugins )
    {
        if ( ptr == plugin )
        {
            tDebug() << Q_FUNC_INFO << "This plugin is already added to the infosystem.";
            return;
        }
    }

    if ( plugin.isNull() )
    {
        tDebug() << Q_FUNC_INFO << "passed-in plugin is null";
        return;
    }

    plugin.data()->moveToThread( this->thread() );
    m_plugins.append( plugin );
    registerInfoTypes( plugin, plugin.data()->supportedGetTypes(), plugin.data()->supportedPushTypes() );

    connect(
        plugin.data(),
            SIGNAL( info( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ),
            this,
            SLOT( infoSlot( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ),
            Qt::QueuedConnection
    );

    connect(
        plugin.data(),
            SIGNAL( getCachedInfo( Tomahawk::InfoSystem::InfoStringHash, qint64, Tomahawk::InfoSystem::InfoRequestData ) ),
            m_cache,
            SLOT( getCachedInfoSlot( Tomahawk::InfoSystem::InfoStringHash, qint64, Tomahawk::InfoSystem::InfoRequestData ) ),
            Qt::QueuedConnection
    );
    connect(
        plugin.data(),
            SIGNAL( updateCache( Tomahawk::InfoSystem::InfoStringHash, qint64, Tomahawk::InfoSystem::InfoType, QVariant ) ),
            m_cache,
            SLOT( updateCacheSlot( Tomahawk::InfoSystem::InfoStringHash, qint64, Tomahawk::InfoSystem::InfoType, QVariant ) ),
            Qt::QueuedConnection
    );
    
    QMetaObject::invokeMethod( plugin.data(), "init", Qt::QueuedConnection );

    emit updatedSupportedGetTypes( QSet< InfoType >::fromList( m_infoGetMap.keys() ) );
    emit updatedSupportedPushTypes( QSet< InfoType >::fromList( m_infoPushMap.keys() ) );
}


void
InfoSystemWorker::removeInfoPlugin( Tomahawk::InfoSystem::InfoPluginPtr plugin )
{
    tDebug() << Q_FUNC_INFO << plugin;

    if ( plugin.isNull() )
    {
        tDebug() << Q_FUNC_INFO << "passed-in plugin is null";
        return;
    }

    foreach ( InfoPluginPtr ptr, m_plugins )
    {
        if ( ptr == plugin )
            break;

        tDebug() << Q_FUNC_INFO << "This plugin does not exist in the infosystem.";
        return;
    }

    m_plugins.removeOne( plugin );
    deregisterInfoTypes( plugin, plugin.data()->supportedGetTypes(), plugin.data()->supportedPushTypes() );
    delete plugin.data();
}


QStringList
InfoSystemWorker::findInfoPlugins()
{
    QStringList paths;
    QList< QDir > pluginDirs;

    QDir appDir( qApp->applicationDirPath() );
#ifdef Q_WS_MAC
    if ( appDir.dirName() == "MacOS" )
    {
        // Development convenience-hack
        appDir.cdUp();
        appDir.cdUp();
        appDir.cdUp();
    }
#endif

    QDir libDir( CMAKE_INSTALL_PREFIX "/lib" );

    QDir lib64Dir( appDir );
    lib64Dir.cdUp();
    lib64Dir.cd( "lib64" );

    pluginDirs << appDir << libDir << lib64Dir << QDir( qApp->applicationDirPath() );
    foreach ( const QDir& pluginDir, pluginDirs )
    {
        tDebug() << Q_FUNC_INFO << "Checking directory for plugins:" << pluginDir;
        foreach ( QString fileName, pluginDir.entryList( QStringList() << "*tomahawk_infoplugin_*.so" << "*tomahawk_infoplugin_*.dylib" << "*tomahawk_infoplugin_*.dll", QDir::Files ) )
        {
            if ( fileName.startsWith( "libtomahawk_infoplugin" ) )
            {
                const QString path = pluginDir.absoluteFilePath( fileName );
                if ( !paths.contains( path ) )
                    paths << path;
            }
        }
    }

    return paths;
}


void
InfoSystemWorker::loadInfoPlugins( const QStringList& pluginPaths )
{
    tDebug() << Q_FUNC_INFO << "Attempting to load the following plugin paths:" << pluginPaths;

    if ( pluginPaths.isEmpty() )
        return;

    foreach ( const QString fileName, pluginPaths )
    {
        if ( !QLibrary::isLibrary( fileName ) )
            continue;

        tDebug() << Q_FUNC_INFO << "Trying to load plugin:" << fileName;

        QPluginLoader loader( fileName );
        QObject* plugin = loader.instance();
        if ( !plugin )
        {
            tDebug() << Q_FUNC_INFO << "Error loading plugin:" << loader.errorString();
            continue;
        }

        InfoPlugin* infoPlugin = qobject_cast< InfoPlugin* >( plugin );
        if ( infoPlugin )
        {
            tDebug() << Q_FUNC_INFO << "Loaded info plugin:" << loader.fileName();
            addInfoPlugin( InfoPluginPtr( infoPlugin ) );
        }
        else
            tDebug() << Q_FUNC_INFO << "Loaded invalid plugin:" << loader.fileName();
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


void
InfoSystemWorker::deregisterInfoTypes( const InfoPluginPtr &plugin, const QSet< InfoType >& getTypes, const QSet< InfoType >& pushTypes )
{
    Q_FOREACH( InfoType type, getTypes )
        m_infoGetMap[type].removeOne( plugin );
    Q_FOREACH( InfoType type, pushTypes )
        m_infoPushMap[type].removeOne( plugin );
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
InfoSystemWorker::pushInfo( Tomahawk::InfoSystem::InfoPushData pushData )
{
    tDebug() << Q_FUNC_INFO << "type is " << pushData.type;

    if ( pushData.pushFlags != PushNoFlag )
    {
        if ( pushData.pushFlags & PushShortUrlFlag )
        {
            pushData.pushFlags = Tomahawk::InfoSystem::PushInfoFlags( pushData.pushFlags & ~PushShortUrlFlag );
            QMetaObject::invokeMethod( this, "getShortUrl", Qt::QueuedConnection, Q_ARG( Tomahawk::InfoSystem::InfoPushData, pushData ) );
            return;
        }
    }

    tDebug() << Q_FUNC_INFO << "number of matching plugins: " << m_infoPushMap[ pushData.type ].size();

    Q_FOREACH( InfoPluginPtr ptr, m_infoPushMap[ pushData.type ] )
    {
        if( ptr )
            QMetaObject::invokeMethod( ptr.data(), "pushInfo", Qt::QueuedConnection, Q_ARG( Tomahawk::InfoSystem::InfoPushData, pushData ) );
    }
}


void
InfoSystemWorker::getShortUrl( Tomahawk::InfoSystem::InfoPushData pushData )
{
    tDebug() << Q_FUNC_INFO << "type is " << pushData.type;
    if ( !pushData.infoPair.second.canConvert< Tomahawk::InfoSystem::InfoStringHash >() )
    {
        QMetaObject::invokeMethod( this, "pushInfo", Qt::QueuedConnection, Q_ARG( Tomahawk::InfoSystem::InfoPushData, pushData ) );
        return;
    }

    Tomahawk::InfoSystem::InfoStringHash hash = pushData.infoPair.second.value< Tomahawk::InfoSystem::InfoStringHash >();

    if ( hash.isEmpty() || !hash.contains( "title" ) || !hash.contains( "artist" ) )
    {
        QMetaObject::invokeMethod( this, "pushInfo", Qt::QueuedConnection, Q_ARG( Tomahawk::InfoSystem::InfoPushData, pushData ) );
        return;
    }

    QString title, artist, album;
    title = hash[ "title" ];
    artist = hash[ "artist" ];
    if( hash.contains( "album" ) )
        album = hash[ "album" ];

    QUrl longUrl = GlobalActionManager::instance()->openLink( title, artist, album );

    GlobalActionManager::instance()->shortenLink( longUrl, QVariant::fromValue< Tomahawk::InfoSystem::InfoPushData >( pushData ) );
    connect( GlobalActionManager::instance(), SIGNAL( shortLinkReady( QUrl, QUrl, QVariant ) ), this, SLOT( shortLinkReady( QUrl, QUrl, QVariant ) ), Qt::UniqueConnection );
    m_shortLinksWaiting++;
}


void
InfoSystemWorker::shortLinkReady( QUrl longUrl, QUrl shortUrl, QVariant callbackObj )
{
    tDebug() << Q_FUNC_INFO << "long url = " << longUrl << ", shortUrl = " << shortUrl;
    m_shortLinksWaiting--;
    if ( !m_shortLinksWaiting )
        disconnect( GlobalActionManager::instance(), SIGNAL( shortLinkReady( QUrl, QUrl, QVariant ) ) );

    if ( !callbackObj.isValid() )
    {
        tDebug() << Q_FUNC_INFO << "callback object was not valid, cannot continue";
        return;
    }

    Tomahawk::InfoSystem::InfoPushData pushData = callbackObj.value< Tomahawk::InfoSystem::InfoPushData >();

    if ( !shortUrl.isEmpty() && longUrl != shortUrl )
    {
        QVariantMap flagProps = pushData.infoPair.first;
        flagProps[ "shorturl" ] = shortUrl;
        pushData.infoPair.first = flagProps;
    }

    tDebug() << Q_FUNC_INFO << "pushInfoPair first is: " << pushData.infoPair.first.keys();

    QMetaObject::invokeMethod( this, "pushInfo", Qt::QueuedConnection, Q_ARG( Tomahawk::InfoSystem::InfoPushData, pushData ) );
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
