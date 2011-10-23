/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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

#include "utils/logger.h"

#include <QDateTime>
#include <QNetworkReply>
#include <parser.h>

using namespace Tomahawk::InfoSystem;

static QString
md5( const QByteArray& src )
{
    QByteArray const digest = QCryptographicHash::hash( src, QCryptographicHash::Md5 );
    return QString::fromLatin1( digest.toHex() ).rightJustified( 32, '0' );
}

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
RoviPlugin::namChangedSlot( QNetworkAccessManager* nam )
{
    if ( !nam )
        return;

    m_nam = nam;
}


void
RoviPlugin::getInfo( uint requestId, Tomahawk::InfoSystem::InfoRequestData requestData )
{
    if ( !requestData.input.canConvert< Tomahawk::InfoSystem::InfoStringHash >() )
    {
        emit info( requestId, requestData, QVariant() );
        return;
    }
    InfoStringHash hash = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash >();
    if ( !hash.contains( "artist" ) || !hash.contains( "album" ) )
    {
        emit info( requestId, requestData, QVariant() );
        return;
    }

    Tomahawk::InfoSystem::InfoStringHash criteria;
    criteria["album"] = hash["album"];

    emit getCachedInfo( requestId, criteria, 2419200000, requestData );
}

void
RoviPlugin::notInCacheSlot( uint requestId, Tomahawk::InfoSystem::InfoStringHash criteria, Tomahawk::InfoSystem::InfoRequestData requestData )
{
    switch ( requestData.type )
    {
        case InfoAlbumSongs:
        {
            QUrl baseUrl = QUrl( "http://api.rovicorp.com/data/v1/album/tracks" );
            baseUrl.addQueryItem( "album", criteria[ "album" ] );

            QNetworkReply* reply = makeRequest( baseUrl );

            reply->setProperty( "requestId", requestId );
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
    int requestId = reply->property( "requestId" ).toUInt();

    emit info( requestId, requestData, QVariant() );

}

void
RoviPlugin::albumLookupFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>( sender() );
    Q_ASSERT( reply );

    if ( reply->error() != QNetworkReply::NoError )
        return;

    Tomahawk::InfoSystem::InfoRequestData requestData = reply->property( "requestData" ).value< Tomahawk::InfoSystem::InfoRequestData >();
    int requestId = reply->property( "requestId" ).toUInt();

    QByteArray data = reply->readAll();
    qDebug() << "data:" << data;
    QJson::Parser p;
    bool ok;
    QVariantMap result = p.parse( data, &ok ).toMap();

    if ( !ok || result.isEmpty() || !result.contains( "tracks" ) )
    {
        tLog() << "Error parsing JSON from Rovi!" << p.errorString() << result;
        emit info( requestId, requestData, QVariant() );
    }

    tDebug() << "Got Rovi results:" << result[ "tracks" ];
    QVariantList tracks = result[ "tracks" ].toList();
    QStringList trackNameList;
    foreach ( const QVariant& track, tracks )
    {
        const QVariantMap trackData = track.toMap();
        if ( trackData.contains( "title" ) )
            trackNameList << trackData[ "title" ].toString();
    }
    tDebug() << "FOUND TRACK LIST FROM ROVI:" << trackNameList;

    QVariantMap returnedData;
    returnedData["tracks"] = trackNameList;

    emit info( requestId, requestData, returnedData );

    Tomahawk::InfoSystem::InfoStringHash criteria;
    criteria["artist"] = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash>()["artist"];

    emit updateCache( criteria, 0, requestData.type, returnedData );
}


QNetworkReply*
RoviPlugin::makeRequest( QUrl url )
{
    url.addQueryItem( "apikey", m_apiKey );
    url.addEncodedQueryItem( "sig", generateSig() );

    qDebug() << "url:" << url.toString();
    return m_nam->get( QNetworkRequest( url ) );
}


QByteArray
RoviPlugin::generateSig() const
{
    QByteArray raw = m_apiKey + m_secret + QString::number( QDateTime::currentMSecsSinceEpoch() / 1000 ).toLatin1();
    return md5( raw ).toLatin1();

}
