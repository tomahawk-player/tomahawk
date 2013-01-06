/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012 Leo Franchi <lfranchi@kde.org>
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

#include "DiscogsPlugin.h"


#include "utils/TomahawkUtils.h"
#include "utils/Logger.h"
#include "utils/Closure.h"

#include <qjson/parser.h>

#include <QNetworkReply>
#include <QDomDocument>

using namespace Tomahawk::InfoSystem;


DiscogsPlugin::DiscogsPlugin()
    : InfoPlugin()
{
    qDebug() << Q_FUNC_INFO;
    m_supportedGetTypes << Tomahawk::InfoSystem::InfoAlbumSongs;
}


DiscogsPlugin::~DiscogsPlugin() {}


void
DiscogsPlugin::getInfo( Tomahawk::InfoSystem::InfoRequestData requestData )
{
    if ( !requestData.input.canConvert< Tomahawk::InfoSystem::InfoStringHash >() )
    {
        emit info( requestData, QVariant() );
        return;
    }

    InfoStringHash hash = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash >();
    if ( !hash.contains( "artist" ) || !hash.contains( "album" ) )
    {
        emit info( requestData, QVariant() );
        return;
    }

    switch ( requestData.type )
    {
        case InfoAlbumSongs:
        {

            Tomahawk::InfoSystem::InfoStringHash criteria;
            criteria["artist"] = hash["artist"];
            criteria["album"] = hash["album"];

            emit getCachedInfo( criteria, 2419200000, requestData );

            break;
        }

        default:
        {
            Q_ASSERT( false );
            break;
        }
    }
}


void
DiscogsPlugin::notInCacheSlot( InfoStringHash criteria, InfoRequestData requestData )
{
    switch ( requestData.type )
    {
        case InfoAlbumSongs:
        {
            QString requestString( "http://api.discogs.com/database/search" );
            QUrl url( requestString );

            TomahawkUtils::urlAddQueryItem( url, "type", "release" );
            TomahawkUtils::urlAddQueryItem( url, "release_title", criteria[ "album" ] );
            TomahawkUtils::urlAddQueryItem( url, "artist", criteria[ "artist" ] );

            QNetworkRequest req( url );
            req.setRawHeader( "User-Agent", "TomahawkPlayer/1.0 +http://tomahawk-player.org" );
            QNetworkReply* reply = TomahawkUtils::nam()->get( req );

            NewClosure( reply, SIGNAL( finished() ),  this, SLOT( albumSearchSlot( Tomahawk::InfoSystem::InfoRequestData, QNetworkReply* ) ), requestData, reply );
            break;
        }

        default:
        {
            Q_ASSERT( false );
            break;
        }
    }
}


void
DiscogsPlugin::albumSearchSlot( const InfoRequestData &requestData, QNetworkReply *reply )
{
    QJson::Parser p;
    QVariantMap results = p.parse( reply ).toMap();

    if ( !results.contains( "results" ) || results.value( "results" ).toList().isEmpty() )
    {
        emit info( requestData, QVariant() );
        return;
    }

    const QVariantMap result = results.value( "results" ).toList().first().toMap();
    if ( !result.contains( "id" ) )
    {
        emit info( requestData, QVariant() );
        return;
    }

    const int id = result.value( "id" ).toInt();
    QUrl url( QString( "http://api.discogs.com/release/%1" ).arg( id ) );
    QNetworkRequest req( url );
    req.setRawHeader( "User-Agent", "TomahawkPlayer/1.0 +http://tomahawk-player.org" );

    QNetworkReply* reply2 = TomahawkUtils::nam()->get( req );
    NewClosure( reply2, SIGNAL( finished() ),  this, SLOT( albumInfoSlot( Tomahawk::InfoSystem::InfoRequestData, QNetworkReply* ) ), requestData, reply2 );
}


void
DiscogsPlugin::albumInfoSlot( const InfoRequestData& requestData, QNetworkReply* reply )
{
    QJson::Parser p;
    QVariantMap results = p.parse( reply ).toMap();

    if ( !results.contains( "resp" ) )
    {
        emit info( requestData, QVariant() );
        return;
    }

    const QVariantMap resp = results[ "resp" ].toMap();
    if ( !resp.contains( "release" ) )
    {
        emit info( requestData, QVariant() );
        return;
    }

    const QVariantMap release = resp[ "release" ].toMap();
    if ( !release.contains( "tracklist" ) || release[ "tracklist" ].toList().isEmpty() )
    {
        emit info( requestData, QVariant() );
        return;
    }

    QStringList trackNameList;
    foreach ( const QVariant& v, release[ "tracklist" ].toList() )
    {
        const QVariantMap track = v.toMap();
        if ( track.contains( "title" ) )
            trackNameList << track[ "title" ].toString();
    }

    QVariantMap returnedData;
    returnedData["tracks"] = trackNameList;

    emit info( requestData, returnedData );

    Tomahawk::InfoSystem::InfoStringHash criteria;
    criteria["artist"] = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash>()["artist"];
    criteria["album"] = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash>()["album"];

    emit updateCache( criteria, 0, requestData.type, returnedData );
}



Q_EXPORT_PLUGIN2( Tomahawk::InfoSystem::InfoPlugin, Tomahawk::InfoSystem::DiscogsPlugin )
