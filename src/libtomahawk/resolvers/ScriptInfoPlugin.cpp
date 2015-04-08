/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2014, Dominik Schmidt <domme@tomahawk-player.org>
 *   Copyright 2015, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "ScriptInfoPlugin_p.h"

#include "JSAccount.h"
#include "Typedefs.h"
#include "ScriptObject.h"
#include "ScriptJob.h"

#include "../utils/Logger.h"
#include "../utils/Json.h"
#include "../utils/NetworkAccessManager.h"
#include "../utils/NetworkReply.h"

using namespace Tomahawk;

ScriptInfoPlugin::ScriptInfoPlugin( const scriptobject_ptr& scriptObject, const QString& name )
    : d_ptr( new ScriptInfoPluginPrivate( this ) )
    , ScriptPlugin( scriptObject )
{
    Q_ASSERT( scriptObject );

    // read in supported GetTypes and PushTypes - we can do this safely because we are still in WebKit thread here
    m_supportedGetTypes = parseSupportedTypes( m_scriptObject->syncInvoke( "supportedGetTypes" ) );
    m_supportedPushTypes = parseSupportedTypes( m_scriptObject->syncInvoke( "supportedPushTypes" ) );

    setFriendlyName( QString( "ScriptInfoPlugin: %1" ).arg( name ) );

    connect( scriptObject.data(), SIGNAL( destroyed( QObject* ) ), SLOT( onScriptObjectDeleted() ) );
}


ScriptInfoPlugin::~ScriptInfoPlugin()
{
}


void
ScriptInfoPlugin::onScriptObjectDeleted()
{
    deleteLater();
}


void
ScriptInfoPlugin::init()
{
}


void
ScriptInfoPlugin::getInfo( Tomahawk::InfoSystem::InfoRequestData requestData )
{
    Q_D( ScriptInfoPlugin );

    QVariantMap arguments;
    arguments[ "type" ] = requestData.type;
    arguments[ "data" ] = convertInfoStringHashToQVariantMap( requestData.input.value<Tomahawk::InfoSystem::InfoStringHash>() );

    ScriptJob* job = m_scriptObject->invoke( "_getInfo", arguments );
    connect( job, SIGNAL( done( QVariantMap ) ), SLOT( onGetInfoRequestDone( QVariantMap ) ) );

    d->requestDataCache[ job->id().toInt() ] = requestData;
    job->start();
}


void
ScriptInfoPlugin::pushInfo( Tomahawk::InfoSystem::InfoPushData pushData )
{
    QVariantMap arguments;
    arguments[ "type" ] = pushData.type;
    arguments[ "pushFlags" ] = pushData.pushFlags;
    arguments[ "input" ] = pushData.infoPair.second.toMap();
    arguments[ "additionalInput" ] = pushData.infoPair.first;

    m_scriptObject->invoke( "pushInfo", arguments );
}


void
ScriptInfoPlugin::notInCacheSlot( Tomahawk::InfoSystem::InfoStringHash criteria, Tomahawk::InfoSystem::InfoRequestData requestData )
{
    Q_D( ScriptInfoPlugin );

    QVariantMap arguments;
    arguments[ "type" ] = requestData.type;
    arguments[ "criteria" ] = convertInfoStringHashToQVariantMap( criteria );

    ScriptJob* job = m_scriptObject->invoke( "_notInCache", arguments );
    connect( job, SIGNAL( done( QVariantMap ) ), SLOT( onNotInCacheRequestDone( QVariantMap ) ) );
    d->requestDataCache[ job->id().toInt() ] = requestData;
    d->criteriaCache[ job->id().toInt() ] = criteria;

    job->start();
}


void
ScriptInfoPlugin::onGetInfoRequestDone( const QVariantMap& result )
{
    Q_ASSERT( QThread::currentThread() == thread() );
    Q_D( ScriptInfoPlugin );

    ScriptJob* job = qobject_cast< ScriptJob* >( sender() );
    if ( job->error() )
    {
        emit info( d->requestDataCache[ job->id().toInt() ], QVariantMap() );
    }
    else
    {
        emit getCachedInfo( convertQVariantMapToInfoStringHash( result[ "criteria" ].toMap() ), result[ "newMaxAge" ].toLongLong(), d->requestDataCache[ job->id().toInt() ] );
    }

    d->requestDataCache.remove( job->id().toInt() );
    sender()->deleteLater();
}


void
ScriptInfoPlugin::onNotInCacheRequestDone( const QVariantMap& result )
{
    Q_ASSERT( QThread::currentThread() == thread() );

    Q_D( ScriptInfoPlugin );

    ScriptJob* job = qobject_cast< ScriptJob* >( sender() );
    sender()->deleteLater();

    // retrieve requestData from cache and delete it
    Tomahawk::InfoSystem::InfoRequestData requestData = d->requestDataCache[ job->id().toInt() ];
    d->requestDataCache.remove( job->id().toInt() );

    // retrieve criteria from cache and delete it
    Tomahawk::InfoSystem::InfoStringHash criteria = d->criteriaCache[ job->id().toInt() ];
    d->criteriaCache.remove( job->id().toInt() );

    QVariantMap resultData = result[ "data" ].toMap();
    switch ( requestData.type )
    {
        case InfoSystem::InfoAlbumCoverArt:
        {
            QNetworkRequest req( resultData[ "url" ].toUrl() );
            NetworkReply* reply = new NetworkReply( Tomahawk::Utils::nam()->get( req ) );
            reply->setProperty( "requestData", QVariant::fromValue< Tomahawk::InfoSystem::InfoRequestData >( requestData ) );
            reply->setProperty( "criteria", convertInfoStringHashToQVariantMap( criteria ) );
            reply->setProperty( "maxAge", result[ "maxAge" ] );

            connect( reply, SIGNAL( finished() ), SLOT( onCoverArtReturned() ) );
        }
        break;

        default:
        {
            emit info( requestData, result[ "data" ].toMap() );
            emit updateCache( criteria, result[ "maxAge" ].toLongLong(), requestData.type, resultData );
            break;
        }
    }
}


void
ScriptInfoPlugin::onCoverArtReturned()
{
    NetworkReply* reply = qobject_cast< NetworkReply* >( sender() );
    reply->deleteLater();

    Tomahawk::InfoSystem::InfoRequestData requestData = reply->property( "requestData" ).value< Tomahawk::InfoSystem::InfoRequestData >();
    Tomahawk::InfoSystem::InfoStringHash criteria = convertQVariantMapToInfoStringHash( reply->property( "criteria" ).toMap() );

    QByteArray ba = reply->reply()->readAll();
    if ( ba.isNull() || ba.isEmpty() )
    {
        tLog() << Q_FUNC_INFO << "Null byte array for cover of" << criteria["artist"] << criteria["album"];
        emit info( requestData, QVariant() );
        return;
    }

    QVariantMap returnedData;
    returnedData["imgbytes"] = ba;
    returnedData["url"] = reply->reply()->url().toString();

    emit info( requestData, returnedData );
    emit updateCache( criteria, reply->property( "maxAge" ).toLongLong(), requestData.type, returnedData );
}


QSet< Tomahawk::InfoSystem::InfoType >
ScriptInfoPlugin::parseSupportedTypes( const QVariant& variant )
{
    QVariantList list = variant.toList();

    QSet < Tomahawk::InfoSystem::InfoType > results;
    foreach( const QVariant& type, list )
    {
        bool ok;
        int intType = type.toInt( &ok );
        if ( ok )
        {
            results.insert( static_cast< Tomahawk::InfoSystem::InfoType >( intType ) );
        }
    }

    return results;
}


QString
ScriptInfoPlugin::serializeQVariantMap( const QVariantMap& map )
{
    QVariantMap localMap = map;

    foreach( const QString& key, localMap.keys() )
    {
        QVariant currentVariant = localMap[ key ];

        // convert InfoStringHash to QVariantMap
        if( currentVariant.canConvert< Tomahawk::InfoSystem::InfoStringHash >() )
        {
            Tomahawk::InfoSystem::InfoStringHash currentHash = currentVariant.value< Tomahawk::InfoSystem::InfoStringHash >();
            localMap[ key ] = convertInfoStringHashToQVariantMap( currentHash );
        }
    }

    return JSAccount::serializeQVariantMap( localMap );
}


QVariantMap
ScriptInfoPlugin::convertInfoStringHashToQVariantMap( const Tomahawk::InfoSystem::InfoStringHash& hash )
{
    QVariantMap map;

    foreach( const QString& key, hash.keys() )
    {
        map[key] = QVariant::fromValue< QString >( hash.value( key ) );
    }

    return map;
}


Tomahawk::InfoSystem::InfoStringHash
ScriptInfoPlugin::convertQVariantMapToInfoStringHash( const QVariantMap& map )
{
    Tomahawk::InfoSystem::InfoStringHash hash;

    foreach( const QString& key, map.keys() )
    {
        hash.insert( key, map[ key ].toString() );
    }

    return hash;
}

