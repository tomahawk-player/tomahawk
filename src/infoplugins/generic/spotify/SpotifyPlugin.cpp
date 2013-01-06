/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Hugo Lindström <hugolm84@gmail.com>
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

#include "SpotifyPlugin.h"

#include <QDir>
#include <QSettings>
#include <QCryptographicHash>
#include <QNetworkConfiguration>
#include <QNetworkReply>

#include "Album.h"
#include "Typedefs.h"
#include "audio/AudioEngine.h"
#include "TomahawkSettings.h"
#include "utils/TomahawkUtils.h"
#include "utils/Logger.h"
#include "CountryUtils.h"
#include "Source.h"

#define SPOTIFY_API_URL "http://spotikea.tomahawk-player.org/"
#include <qjson/parser.h>
#include <qjson/serializer.h>

using namespace Tomahawk::InfoSystem;


SpotifyPlugin::SpotifyPlugin()
    : InfoPlugin()
    , m_chartsFetchJobs( 0 )
{
    m_supportedGetTypes << InfoChart << InfoChartCapabilities;

}


SpotifyPlugin::~SpotifyPlugin()
{
    qDebug() << Q_FUNC_INFO;
}


void
SpotifyPlugin::dataError( Tomahawk::InfoSystem::InfoRequestData requestData )
{
    emit info( requestData, QVariant() );
    return;
}


void
SpotifyPlugin::getInfo( Tomahawk::InfoSystem::InfoRequestData requestData )
{
    qDebug() << Q_FUNC_INFO << requestData.caller;
    qDebug() << Q_FUNC_INFO << requestData.customData;

    InfoStringHash hash = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash >();

    switch ( requestData.type )
    {
        case InfoChart:
            if ( !hash.contains( "chart_source" ) || hash["chart_source"] != "spotify" )
            {
                dataError( requestData );
                break;
            }
            qDebug() << Q_FUNC_INFO << "InfoCHart req for" << hash["chart_source"];
            fetchChart( requestData );
            break;

        case InfoChartCapabilities:
            fetchChartCapabilities( requestData );
            break;

        default:
            dataError( requestData );
    }
}


void
SpotifyPlugin::fetchChart( Tomahawk::InfoSystem::InfoRequestData requestData )
{
    if ( !requestData.input.canConvert< Tomahawk::InfoSystem::InfoStringHash >() )
    {
        dataError( requestData );
        return;
    }

    InfoStringHash hash = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash >();
    Tomahawk::InfoSystem::InfoStringHash criteria;
    /// Each request needs to contain both a id and source
    if ( !hash.contains( "chart_id" ) && !hash.contains( "chart_source" ) )
    {
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Hash did not contain required params!";
        dataError( requestData );
        return;
    }
    /// Set the criterias for current chart
    criteria["chart_id"] = hash["chart_id"];
    criteria["chart_source"] = hash["chart_source"];

    emit getCachedInfo( criteria, 86400000 /* Expire chart cache in 1 day */, requestData );
}


void
SpotifyPlugin::fetchChartCapabilities( Tomahawk::InfoSystem::InfoRequestData requestData )
{
    if ( !requestData.input.canConvert< Tomahawk::InfoSystem::InfoStringHash >() )
    {
        dataError( requestData );
        return;
    }

    Tomahawk::InfoSystem::InfoStringHash criteria;
    criteria[ "InfoChartCapabilities" ] = "spotifyplugin";
    emit getCachedInfo( criteria, 604800000, requestData );
}


void
SpotifyPlugin::notInCacheSlot( Tomahawk::InfoSystem::InfoStringHash criteria, Tomahawk::InfoSystem::InfoRequestData requestData )
{
    switch ( requestData.type )
    {
        case InfoChart:
        {
            /// Fetch the chart, we need source and id
            tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "InfoChart not in cache! Fetching...";
            QUrl url = QUrl( QString( SPOTIFY_API_URL "toplist/%1/" ).arg( criteria["chart_id"] ) );
            qDebug() << Q_FUNC_INFO << "Getting chart url" << url;

            QNetworkReply* reply = TomahawkUtils::nam()->get( QNetworkRequest( url ) );
            reply->setProperty( "requestData", QVariant::fromValue< Tomahawk::InfoSystem::InfoRequestData >( requestData ) );
            connect( reply, SIGNAL( finished() ), SLOT( chartReturned() ) );
            return;
        }
        case InfoChartCapabilities:
        {
            tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "InfoChartCapabilities not in cache! Fetching...";

            // we never need to re-fetch
            if ( !m_allChartsMap.isEmpty() )
                return;

            /// We need to fetch possible types before they are asked for
            tDebug() << "SpotifyPlugin: InfoChart fetching possible resources";

            QUrl url = QUrl( QString( SPOTIFY_API_URL "toplist/charts" )  );
            QNetworkReply* reply = TomahawkUtils::nam()->get( QNetworkRequest( url ) );
            tDebug() << Q_FUNC_INFO << "fetching:" << url;
            connect( reply, SIGNAL( finished() ), SLOT( chartTypes() ) );
            m_chartsFetchJobs++;

            if ( m_chartsFetchJobs > 0 )
            {
                qDebug() << Q_FUNC_INFO << "InfoChartCapabilities still fetching!";
                m_cachedRequests.append( requestData );
                return;
            }

            emit info( requestData, m_allChartsMap );
            return;
        }

        default:
        {
            tLog() << Q_FUNC_INFO << "Couldn't figure out what to do with this type of request after cache miss";
            emit info( requestData, QVariant() );
            return;
        }
    }
}


void
SpotifyPlugin::chartTypes()
{
    /// Get possible chart type for specificSpotifyPlugin: InfoChart types returned chart source
    tDebug() << Q_FUNC_INFO << "Got spotifychart type result";
    QNetworkReply* reply = qobject_cast<QNetworkReply*>( sender() );

    if ( reply->error() == QNetworkReply::NoError )
    {
        QJson::Parser p;
        bool ok;
        const QVariantMap res = p.parse( reply, &ok ).toMap();
        const QVariantMap chartObj = res;

        if ( !ok )
        {
            tLog() << Q_FUNC_INFO << "Failed to parse resources" << p.errorString() << "On line" << p.errorLine();

            return;
        }

        QVariantMap charts;
        foreach( QVariant geos, chartObj.value( "Charts" ).toList().takeLast().toMap().value( "geo" ).toList() )
        {
           const QString geo = geos.toMap().value( "name" ).toString();
           const QString geoId = geos.toMap().value( "id" ).toString();
           QString country;

           if( geo == "For me" )
              continue; /// country = geo; Lets use this later, when we can get the spotify username from tomahawk
           else if( geo == "Everywhere" )
               country = geo;
           else
           {
               QLocale l( QString( "en_%1" ).arg( geo ) );
               country = Tomahawk::CountryUtils::fullCountryFromCode( geo );

               for ( int i = 1; i < country.size(); i++ )
               {
                   if ( country.at( i ).isUpper() )
                   {
                       country.insert( i, " " );
                       i++;
                   }
               }
           }

           QList< InfoStringHash > chart_types;
           foreach( QVariant types, chartObj.value( "Charts" ).toList().takeFirst().toMap().value( "types" ).toList() )
           {
               QString type = types.toMap().value( "id" ).toString();
               QString label = types.toMap().value( "name" ).toString();

               InfoStringHash c;
               c[ "id" ] = type + "/" + geoId;
               c[ "label" ] = label;
               c[ "type" ] = type;

               chart_types.append( c );
           }

           charts.insert( country.toUtf8(), QVariant::fromValue<QList< InfoStringHash > >( chart_types ) );
        }

        QVariantMap defaultMap;
        defaultMap[ "spotify" ] = QStringList() << "United States" << "Top Albums";
        m_allChartsMap[ "defaults" ] = defaultMap;
        m_allChartsMap.insert( "Spotify", QVariant::fromValue<QVariantMap>( charts ) );
    }
    else
    {
        tLog() << Q_FUNC_INFO << "Error fetching charts:" << reply->errorString();
    }

    m_chartsFetchJobs--;
    if ( !m_cachedRequests.isEmpty() && m_chartsFetchJobs == 0 )
    {
        foreach ( InfoRequestData request, m_cachedRequests )
        {
            emit info( request, m_allChartsMap );
            Tomahawk::InfoSystem::InfoStringHash criteria;
            criteria[ "InfoChartCapabilities" ] = "spotifyplugin";
            emit updateCache( criteria,604800000, request.type, m_allChartsMap );
        }
        m_cachedRequests.clear();
    }
}


void
SpotifyPlugin::chartReturned()
{
    /// Chart request returned something! Woho
    QNetworkReply* reply = qobject_cast<QNetworkReply*>( sender() );
    QString url = reply->url().toString();
    QVariantMap returnedData;
    if ( reply->error() == QNetworkReply::NoError )
    {
        QJson::Parser p;
        bool ok;
        QVariantMap res = p.parse( reply, &ok ).toMap();

        if ( !ok )
        {
            tLog() << "Failed to parse json from chart lookup:" << p.errorString() << "On line" << p.errorLine();
            return;
        }

        /// SO we have a result, parse it!
        QList< InfoStringHash > top_tracks;
        QList< InfoStringHash > top_albums;
        QStringList top_artists;

        if( url.contains( "albums" ) )
            setChartType( Album );
        else if( url.contains( "tracks" ) )
            setChartType( Track );
        else if( url.contains( "artists" ) )
            setChartType( Artist );
        else
            setChartType( None );

        foreach( QVariant result, res.value( "toplist" ).toMap().value( "result" ).toList() )
        {
            QString title, artist;
            QVariantMap chartMap = result.toMap();

            if ( !chartMap.isEmpty() )
            {
                title = chartMap.value( "title" ).toString();
                artist = chartMap.value( "artist" ).toString();

                if( chartType() == Track )
                {
                    InfoStringHash pair;
                    pair["artist"] = artist;
                    pair["track"] = title;
                    top_tracks << pair;

                    qDebug() << "SpotifyChart type is track";
                }

                if( chartType() == Album )
                {
                    InfoStringHash pair;
                    pair["artist"] = artist;
                    pair["album"] = title;
                    top_albums << pair;
                    qDebug() << "SpotifyChart type is album";
                }

                if( chartType() == Artist )
                {
                    top_artists << chartMap.value( "name" ).toString();
                    qDebug() << "SpotifyChart type is artist";
                }
            }
        }

        if( chartType() == Track )
        {
            tDebug() << "ChartsPlugin:" << "\tgot " << top_tracks.size() << " tracks";
            returnedData["tracks"] = QVariant::fromValue( top_tracks );
            returnedData["type"] = "tracks";
        }

        if( chartType() == Album )
        {
            tDebug() << "ChartsPlugin:" << "\tgot " << top_albums.size() << " albums";
            returnedData["albums"] = QVariant::fromValue( top_albums );
            returnedData["type"] = "albums";
        }

        if( chartType() == Artist )
        {
            tDebug() << "ChartsPlugin:" << "\tgot " << top_artists.size() << " artists";
            returnedData["artists"] = top_artists;
            returnedData["type"] = "artists";
        }

        Tomahawk::InfoSystem::InfoRequestData requestData = reply->property( "requestData" ).value< Tomahawk::InfoSystem::InfoRequestData >();

        emit info( requestData, returnedData );

        // update cache
        Tomahawk::InfoSystem::InfoStringHash criteria;
        Tomahawk::InfoSystem::InfoStringHash origData = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash >();
        criteria[ "chart_id" ] = origData[ "chart_id" ];
        criteria[ "chart_source" ] = origData[ "chart_source" ];
        emit updateCache( criteria, 86400000, requestData.type, returnedData );
    }
    else
        qDebug() << "Network error in fetching chart:" << reply->url().toString();
}


Q_EXPORT_PLUGIN2( Tomahawk::InfoSystem::InfoPlugin, Tomahawk::InfoSystem::SpotifyPlugin )
