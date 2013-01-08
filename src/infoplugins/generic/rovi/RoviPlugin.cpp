/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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

#include "RoviPlugin.h"

#include "utils/Logger.h"

#include <qjson/parser.h>

#include <QDateTime>
#include <QNetworkReply>

using namespace Tomahawk::InfoSystem;


RoviPlugin::RoviPlugin()
    : InfoPlugin()
{
    m_supportedGetTypes << InfoAlbumSongs;

    /*
     *    Your API Key is 7jxr9zggt45h6rg2n4ss3mrj
     *    Your secret is XUnYutaAW6
     */
    m_apiKey = "7jxr9zggt45h6rg2n4ss3mrj";
    m_secret = "XUnYutaAW6";
}


RoviPlugin::~RoviPlugin()
{
}


void
RoviPlugin::getInfo( Tomahawk::InfoSystem::InfoRequestData requestData )
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

    Tomahawk::InfoSystem::InfoStringHash criteria;
    criteria["artist"] = hash["artist"];
    criteria["album"] = hash["album"];

    emit getCachedInfo( criteria, 0, requestData );
}


void
RoviPlugin::notInCacheSlot( Tomahawk::InfoSystem::InfoStringHash criteria, Tomahawk::InfoSystem::InfoRequestData requestData )
{
    switch ( requestData.type )
    {
        case InfoAlbumSongs:
        {
            QUrl baseUrl = QUrl( "http://api.rovicorp.com/search/v2/music/search" );

            TomahawkUtils::urlAddQueryItem( baseUrl, "query", QString( "%1 %2" ).arg( criteria[ "artist" ] ).arg( criteria[ "album" ] ) );
            TomahawkUtils::urlAddQueryItem( baseUrl, "entitytype", "album" );
            TomahawkUtils::urlAddQueryItem( baseUrl, "include", "album:tracks" );

            QNetworkReply* reply = makeRequest( baseUrl );

            reply->setProperty( "requestData", QVariant::fromValue< Tomahawk::InfoSystem::InfoRequestData >( requestData ) );
            connect( reply, SIGNAL( finished() ), this, SLOT( albumLookupFinished() ) );
            connect( reply, SIGNAL( error( QNetworkReply::NetworkError ) ), this, SLOT( albumLookupError( QNetworkReply::NetworkError ) ) );
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
RoviPlugin::albumLookupError( QNetworkReply::NetworkError error )
{
    if ( error == QNetworkReply::NoError )
        return;

    QNetworkReply* reply = qobject_cast<QNetworkReply*>( sender() );
    Q_ASSERT( reply );

    Tomahawk::InfoSystem::InfoRequestData requestData = reply->property( "requestData" ).value< Tomahawk::InfoSystem::InfoRequestData >();

    emit info( requestData, QVariant() );

}


void
RoviPlugin::albumLookupFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>( sender() );
    Q_ASSERT( reply );

    if ( reply->error() != QNetworkReply::NoError )
        return;

    Tomahawk::InfoSystem::InfoRequestData requestData = reply->property( "requestData" ).value< Tomahawk::InfoSystem::InfoRequestData >();

    QJson::Parser p;
    bool ok;
    QVariantMap response = p.parse( reply, &ok ).toMap();

    if ( !ok || response.isEmpty() || !response.contains( "searchResponse" ) )
    {
        tLog() << "Error parsing JSON from Rovi!" << p.errorString() << response;
        emit info( requestData, QVariant() );
        return;
    }

    QVariantList resultList = response[ "searchResponse" ].toMap().value( "results" ).toList();
    if ( resultList.size() == 0 )
    {
        emit info( requestData, QVariant() );
        return;
    }

    QVariantMap results = resultList.first().toMap();
    QVariantList tracks = results[ "album" ].toMap()[ "tracks" ].toList();

    if ( tracks.isEmpty() )
    {
        tLog() << "Error parsing JSON from Rovi!" << p.errorString() << response;
        emit info( requestData, QVariant() );
    }


    QStringList trackNameList;
    foreach ( const QVariant& track, tracks )
    {
        const QVariantMap trackData = track.toMap();
        if ( trackData.contains( "title" ) )
            trackNameList << trackData[ "title" ].toString();
    }

    QVariantMap returnedData;
    returnedData["tracks"] = trackNameList;

    emit info( requestData, returnedData );

    Tomahawk::InfoSystem::InfoStringHash criteria;
    criteria["artist"] = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash>()["artist"];
    criteria["album"] = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash>()["album"];

    emit updateCache( criteria, 0, requestData.type, returnedData );
}


QNetworkReply*
RoviPlugin::makeRequest( QUrl url )
{
    TomahawkUtils::urlAddQueryItem( url, "apikey", m_apiKey );
    TomahawkUtils::urlAddQueryItem( url, "sig", generateSig() );

    qDebug() << "Rovi request url:" << url.toString();
    return TomahawkUtils::nam()->get( QNetworkRequest( url ) );
}


QByteArray
RoviPlugin::generateSig() const
{
    QByteArray raw = m_apiKey + m_secret + QString::number( QDateTime::currentMSecsSinceEpoch() / 1000 ).toLatin1();
    return TomahawkUtils::md5( raw ).toLatin1();
}


Q_EXPORT_PLUGIN2( Tomahawk::InfoSystem::InfoPlugin, Tomahawk::InfoSystem::RoviPlugin )
