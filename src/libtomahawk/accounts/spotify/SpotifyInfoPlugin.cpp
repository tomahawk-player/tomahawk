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

#include "SpotifyInfoPlugin.h"

#include "SpotifyAccount.h"
#include "utils/Closure.h"
#include "utils/Logger.h"

using namespace Tomahawk;
using namespace Tomahawk::InfoSystem;


SpotifyInfoPlugin::SpotifyInfoPlugin( Accounts::SpotifyAccount* account )
    : InfoPlugin()
    , m_account( QWeakPointer< Accounts::SpotifyAccount >( account ) )
{
    if ( !m_account.isNull() )
    {
        m_supportedGetTypes << InfoAlbumSongs;
        m_supportedPushTypes << InfoLove << InfoUnLove;
    }
}


SpotifyInfoPlugin::~SpotifyInfoPlugin()
{

}

void
SpotifyInfoPlugin::pushInfo( Tomahawk::InfoSystem::InfoPushData pushData )
{
    if ( m_account.isNull() || !m_account.data()->loggedIn() )
        return;

    switch ( pushData.type )
    {
        case InfoLove:
        case InfoUnLove:
            sendLoveSong(pushData.type, pushData.infoPair.second);
            break;

        default:
            return;
    }
}

void
SpotifyInfoPlugin::sendLoveSong( const InfoType type, QVariant input )
{

    if ( m_account.isNull() || !m_account.data()->loggedIn() )
        return;

    if( !m_account.data()->loveSync() )
        return;

    if ( !input.toMap().contains( "trackinfo" ) || !input.toMap()[ "trackinfo" ].canConvert< Tomahawk::InfoSystem::InfoStringHash >() )
    {
        tLog( LOGVERBOSE ) << "SpotifyInfoPlugin::sendLoveSong cannot convert input!";
        return;
    }

    InfoStringHash hash = input.toMap()[ "trackinfo" ].value< Tomahawk::InfoSystem::InfoStringHash >();
    if ( !hash.contains( "title" ) || !hash.contains( "artist" ) || !hash.contains( "album" ) )
        return;

    if ( type == Tomahawk::InfoSystem::InfoLove )
    {
        m_account.data()->starTrack( hash["artist"], hash["title"], true );
    }
    else if ( type == Tomahawk::InfoSystem::InfoUnLove )
    {
        m_account.data()->starTrack( hash["artist"], hash["title"], false );
    }
}

void
SpotifyInfoPlugin::getInfo( InfoRequestData requestData )
{
    switch ( requestData.type )
    {
    case InfoAlbumSongs:
    {
        if ( !requestData.input.canConvert< Tomahawk::InfoSystem::InfoStringHash >() )
        {
            dataError( requestData );
            return;
        }

        InfoStringHash hash = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash >();
        if ( !hash.contains( "album" ) )
        {
            dataError( requestData );
            return;
        }

        Tomahawk::InfoSystem::InfoStringHash criteria;
        criteria[ "album" ] = hash[ "album" ];
        if ( hash.contains( "artist" ) )
            criteria["artist"] = hash["artist"];

        emit getCachedInfo( criteria, 2419200000, requestData );

        return;
    }
    default:
        dataError( requestData );
    }
}


void
SpotifyInfoPlugin::notInCacheSlot( InfoStringHash criteria, InfoRequestData requestData )
{
    switch ( requestData.type )
    {
    case InfoAlbumSongs:
    {
        const QString album = criteria[ "album" ];
        const QString artist = criteria[ "artist" ];

        if ( m_account.isNull() || !m_account.data()->loggedIn() )
        {
            // No running spotify account, use our webservice
            QUrl lookupUrl( "http://ws.spotify.com/search/1/album.json" );
            lookupUrl.addQueryItem( "q", QString( "%1 %2" ).arg( artist ).arg( album ) );

            QNetworkReply * reply = TomahawkUtils::nam()->get( QNetworkRequest( lookupUrl ) );
            NewClosure( reply, SIGNAL( finished() ), this, SLOT( albumIdLookupFinished( QNetworkReply*, Tomahawk::InfoSystem::InfoRequestData ) ), reply, requestData );
        }
        else
        {
            // Running resolver, so do the lookup through that
            tLog( LOGVERBOSE ) << Q_FUNC_INFO << "Doing album lookup through spotify:" << album << artist;
            QVariantMap message;
            message[ "_msgtype" ] = "albumListing";
            message[ "artist" ] = artist;
            message[ "album" ] = album;

            QMetaObject::invokeMethod( m_account.data(), "sendMessage", Qt::QueuedConnection, Q_ARG( QVariantMap, message ),
                                                                                              Q_ARG( QObject*, this ),
                                                                                              Q_ARG( QString, "albumListingResult" ),
                                                                                              Q_ARG( QVariant, QVariant::fromValue< InfoRequestData >( requestData ) ) );
        }
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
SpotifyInfoPlugin::albumListingResult( const QString& msgType, const QVariantMap& msg, const QVariant& extraData )
{
    Q_UNUSED( msgType );
    Q_ASSERT( msg.contains( "qid" ) );
    Q_ASSERT( extraData.canConvert< InfoRequestData >() );

    const InfoRequestData requestData = extraData.value< InfoRequestData >();

    QVariantList tracks = msg.value( "tracks" ).toList();
    QStringList trackNameList;

    foreach ( const QVariant track, tracks )
    {
        const QVariantMap trackData = track.toMap();
        if ( trackData.contains( "track" ) && !trackData[ "track" ].toString().isEmpty() )
            trackNameList << trackData[ "track" ].toString();
    }

    tLog( LOGVERBOSE ) << Q_FUNC_INFO << "Successfully got album listing from spotify resolver";
    trackListResult( trackNameList, requestData );
}


void
SpotifyInfoPlugin::albumIdLookupFinished( QNetworkReply* reply, const InfoRequestData& requestData )
{
    Q_ASSERT( reply );

    reply->deleteLater();

    if ( reply->error() == QNetworkReply::NoError )
    {
        QJson::Parser p;
        const QVariantMap response = p.parse( reply ).toMap();
        if ( !response.contains( "albums" ) )
        {
            dataError( requestData );
            return;
        }

        const QVariantList albums = response.value( "albums" ).toList();
        if ( albums.isEmpty() )
        {
            dataError( requestData );
            return;
        }

        const QVariantMap album = albums.first().toMap();
        const QString id = album.value( "href" ).toString();
        if ( id.isEmpty() || !id.contains( "spotify:album" ) )
        {
            tLog( LOGVERBOSE ) << "Empty or malformed spotify album ID from json:" << id << response;
            dataError( requestData );
            return;
        }

        tLog( LOGVERBOSE ) << "Doing spotify album lookup via webservice with ID:" << id;

        QUrl lookupUrl( QString( "http://spotikea.tomahawk-player.org/browse/%1" ).arg( id ) );


        QNetworkReply * reply = TomahawkUtils::nam()->get( QNetworkRequest( lookupUrl ) );
        NewClosure( reply, SIGNAL( finished() ), this, SLOT( albumContentsLookupFinished( QNetworkReply*, Tomahawk::InfoSystem::InfoRequestData ) ), reply, requestData );
    }
    else
    {
        tLog( LOGVERBOSE ) << "Network Error retrieving ID from spotify metadata service:" << reply->error() << reply->errorString() << reply->url();
    }
}


void
SpotifyInfoPlugin::albumContentsLookupFinished( QNetworkReply* reply, const InfoRequestData& requestData )
{
    Q_ASSERT( reply );

    reply->deleteLater();

    if ( reply->error() == QNetworkReply::NoError )
    {
        QJson::Parser p;
        const QVariantMap response = p.parse( reply ).toMap();

        if ( !response.contains( "album" ) )
        {
            dataError( requestData );
            return;
        }

        const QVariantMap album = response.value( "album" ).toMap();
        if ( !album.contains( "result" ) || album.value( "result" ).toList().isEmpty() )
        {
            dataError( requestData );
            return;
        }

        const QVariantList albumTracks = album.value( "result" ).toList();
        QStringList trackNameList;

        foreach ( const QVariant& track, albumTracks )
        {
            const QVariantMap trackMap = track.toMap();
            if ( trackMap.contains( "title" ) )
                trackNameList << trackMap.value( "title" ).toString();
        }

        tLog( LOGVERBOSE ) << Q_FUNC_INFO << "Successfully got album listing from spotikea service!";

        if ( trackNameList.isEmpty() )
            dataError( requestData );
        else
            trackListResult( trackNameList, requestData );
    }
    else
    {
        tLog( LOGVERBOSE ) << "Network Error retrieving ID from spotify metadata service:" << reply->error() << reply->errorString() << reply->url();
    }
}

void
SpotifyInfoPlugin::dataError( InfoRequestData requestData )
{
    emit info( requestData, QVariant() );
}


void
SpotifyInfoPlugin::trackListResult( const QStringList& trackNameList, const InfoRequestData& requestData )
{
    QVariantMap returnedData;
    returnedData["tracks"] = trackNameList;

    emit info( requestData, returnedData );

    Tomahawk::InfoSystem::InfoStringHash criteria;
    criteria["artist"] = requestData.input.value< InfoStringHash>()["artist"];
    criteria["album"] = requestData.input.value< InfoStringHash>()["album"];

    emit updateCache( criteria, 0, requestData.type, returnedData );
}
