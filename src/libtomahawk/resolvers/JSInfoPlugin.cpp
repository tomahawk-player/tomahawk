/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2014,      Dominik Schmidt <domme@tomahawk-player.org>
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

#include "JSInfoPlugin_p.h"

#include "JSResolver.h"
#include "Typedefs.h"

#include "../utils/Logger.h"
#include "../utils/Json.h"

using namespace Tomahawk;

JSInfoPlugin::JSInfoPlugin( int id, JSPlugin *resolver )
    : d_ptr( new JSInfoPluginPrivate( this, id, resolver ) )
{
    Q_ASSERT( resolver );

    // read in supported GetTypes and PushTypes - we can do this safely because we are still in WebKit thread here
    m_supportedGetTypes = parseSupportedTypes( callMethodOnInfoPluginWithResult( "supportedGetTypes" ) );
    m_supportedPushTypes = parseSupportedTypes( callMethodOnInfoPluginWithResult( "supportedPushTypes" ) );

    setFriendlyName( QString( "JSInfoPlugin: %1" ) ); // TODO: .arg( resolver->name() )
}


JSInfoPlugin::~JSInfoPlugin()
{
}


void
JSInfoPlugin::init()
{
}


void
JSInfoPlugin::getInfo( Tomahawk::InfoSystem::InfoRequestData requestData )
{
    Q_D( JSInfoPlugin );

    d->requestDataCache[ requestData.requestId ] = requestData;

    QString eval = QString( "_getInfo(%1, %2, %3, %4)" )
        .arg( d->id ) // infoPluginId
        .arg( requestData.requestId ) // requestId
        .arg( requestData.type )  // type
        .arg( serializeQVariantMap(  convertInfoStringHashToQVariantMap( requestData.input.value<Tomahawk::InfoSystem::InfoStringHash>() ) ) ); // infoHash

    callMethodOnInfoPlugin( eval );
}


void
JSInfoPlugin::pushInfo( Tomahawk::InfoSystem::InfoPushData pushData )
{
    QString eval = QString( "pushInfo({ type: %1, pushFlags: %2, input: %3, additionalInput: %4})" )
        .arg( pushData.type )
        .arg( pushData.pushFlags )
        .arg( serializeQVariantMap ( pushData.infoPair.second.toMap() ) )
        .arg( serializeQVariantMap( pushData.infoPair.first ) );

    callMethodOnInfoPlugin( eval );
}


void
JSInfoPlugin::notInCacheSlot( Tomahawk::InfoSystem::InfoStringHash criteria, Tomahawk::InfoSystem::InfoRequestData requestData )
{
    Q_D( JSInfoPlugin );

    d->requestDataCache[ requestData.requestId ] = requestData;
    d->criteriaCache[ requestData.requestId ] = criteria;


    QString eval = QString( "_notInCache(%1, %2, %3, %4)" )
        .arg( d->id )
        .arg( requestData.requestId )
        .arg( requestData.type )
        .arg( serializeQVariantMap( convertInfoStringHashToQVariantMap( criteria ) ) );

    callMethodOnInfoPlugin( eval );
}


void
JSInfoPlugin::addInfoRequestResult( int requestId, qint64 maxAge, const QVariantMap& returnedData )
{
    if ( QThread::currentThread() != thread() )
    {
        QMetaObject::invokeMethod( this, "addInfoRequestResult", Qt::QueuedConnection, Q_ARG( int, requestId ), Q_ARG( qint64, maxAge ), Q_ARG( QVariantMap, returnedData ) );
        return;
    }

    Q_D( JSInfoPlugin );

    // retrieve requestData from cache and delete it
    Tomahawk::InfoSystem::InfoRequestData requestData = d->requestDataCache[ requestId ];
    d->requestDataCache.remove( requestId );

    emit info( requestData, returnedData );

    // retrieve criteria from cache and delete it
    Tomahawk::InfoSystem::InfoStringHash criteria = d->criteriaCache[ requestId ];
    d->criteriaCache.remove( requestId );

    emit updateCache( criteria, maxAge, requestData.type, returnedData );
}


void
JSInfoPlugin::emitGetCachedInfo( int requestId, const QVariantMap& criteria, int newMaxAge )
{
    if ( QThread::currentThread() != thread() )
    {
        QMetaObject::invokeMethod( this, "emitGetCachedInfo", Qt::QueuedConnection, Q_ARG( int, requestId ), Q_ARG( QVariantMap, criteria ), Q_ARG( int, newMaxAge ) );
        return;
    }

    Q_D( JSInfoPlugin );


    emit getCachedInfo( convertQVariantMapToInfoStringHash( criteria ), newMaxAge, d->requestDataCache[ requestId ]);
}


void
JSInfoPlugin::emitInfo( int requestId, const QVariantMap& output )
{
    if ( QThread::currentThread() != thread() )
    {
        QMetaObject::invokeMethod( this, "emitInfo", Qt::QueuedConnection, Q_ARG( int, requestId ), Q_ARG( QVariantMap, output ) );
        return;
    }

    Q_D( JSInfoPlugin );

    emit info( d->requestDataCache[ requestId ], output );
}


QString
JSInfoPlugin::serviceGetter() const
{
    Q_D( const JSInfoPlugin );

    return QString( "Tomahawk.InfoSystem.getInfoPlugin(%1)" ).arg( d->id );
}

// TODO: DRY, really move things into base class
void
JSInfoPlugin::callMethodOnInfoPlugin( const QString& scriptSource )
{
    Q_D( JSInfoPlugin );

    QString eval = QString( "%1.%2" ).arg( serviceGetter() ).arg( scriptSource );

    tLog() << Q_FUNC_INFO << eval;

    d->resolver->evaluateJavaScript( eval );
}


QVariant
JSInfoPlugin::callMethodOnInfoPluginWithResult(const QString& scriptSource)
{
    Q_D( JSInfoPlugin );

    QString eval = QString( "%1.%2" ).arg( serviceGetter() ).arg( scriptSource );

    tLog() << Q_FUNC_INFO << eval;

    return d->resolver->evaluateJavaScriptWithResult( eval );
}



QSet< Tomahawk::InfoSystem::InfoType >
JSInfoPlugin::parseSupportedTypes( const QVariant& variant )
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
        tLog() << type << intType;
    }

    return results;
}


QString
JSInfoPlugin::serializeQVariantMap( const QVariantMap& map )
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

    return JSPlugin::serializeQVariantMap( localMap );
}


QVariantMap
JSInfoPlugin::convertInfoStringHashToQVariantMap( const Tomahawk::InfoSystem::InfoStringHash& hash )
{
    QVariantMap map;

    foreach( const QString& key, hash.keys() )
    {
        map[key] = QVariant::fromValue< QString >( hash.value( key ) );
    }

    return map;
}


Tomahawk::InfoSystem::InfoStringHash
JSInfoPlugin::convertQVariantMapToInfoStringHash( const QVariantMap& map )
{
    Tomahawk::InfoSystem::InfoStringHash hash;

    foreach( const QString& key, map.keys() )
    {
        hash.insert( key, map[ key ].toString() );
    }

    return hash;
}

