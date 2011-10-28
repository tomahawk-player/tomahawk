/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Hugo Lindström <hugolm84@gmail.com>
 *   Copyright 2011, Leo Franchi <lfranchi@kde.org>
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

#include "chartsplugin.h"

#include <QDir>
#include <QSettings>
#include <QCryptographicHash>
#include <QNetworkConfiguration>
#include <QNetworkReply>
#include <QDomElement>

#include "album.h"
#include "chartsplugin_data_p.h"
#include "typedefs.h"
#include "audio/audioengine.h"
#include "tomahawksettings.h"
#include "utils/tomahawkutils.h"
#include "utils/logger.h"

#define CHART_URL "http://charts.tomahawk-player.org:10080/"
//#define CHART_URL "http://localhost:8080/"
#include <qjson/parser.h>
#include <qjson/serializer.h>

using namespace Tomahawk::InfoSystem;


ChartsPlugin::ChartsPlugin()
    : InfoPlugin()
    , m_chartsFetchJobs( 0 )
{


    /// Add resources here
    m_chartResources << "billboard" << "itunes";
    m_supportedGetTypes <<  InfoChart << InfoChartCapabilities;

}


ChartsPlugin::~ChartsPlugin()
{
    qDebug() << Q_FUNC_INFO;
}


void
ChartsPlugin::namChangedSlot( QNetworkAccessManager *nam )
{
    tDebug() << "ChartsPlugin: namChangedSLot";

    qDebug() << Q_FUNC_INFO;
    if( !nam )
        return;

    m_nam = QWeakPointer< QNetworkAccessManager >( nam );

    /// Then get each chart from resource
    /// We need to fetch them before they are asked for

    if( !m_chartResources.isEmpty() && m_nam ){

        tDebug() << "ChartsPlugin: InfoChart fetching possible resources";
        foreach ( QVariant resource, m_chartResources )
        {
            QUrl url = QUrl( QString( CHART_URL "source/%1" ).arg(resource.toString() ) );
            QNetworkReply* reply = m_nam.data()->get( QNetworkRequest( url ) );
            tDebug() << "fetching:" << url;
            connect( reply, SIGNAL( finished() ), SLOT( chartTypes() ) );

            m_chartsFetchJobs++;
        }

    }
}


void
ChartsPlugin::dataError( Tomahawk::InfoSystem::InfoRequestData requestData )
{
    emit info( requestData, QVariant() );
    return;
}


void
ChartsPlugin::getInfo( Tomahawk::InfoSystem::InfoRequestData requestData )
{
    qDebug() << Q_FUNC_INFO << requestData.caller;
    qDebug() << Q_FUNC_INFO << requestData.customData;

    InfoStringHash hash = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash >();
    bool foundSource = false;

    switch ( requestData.type )
    {

        case InfoChart:
            /// We need something to check if the request is actually ment to go to this plugin
            if ( !hash.contains( "chart_source" ) )
            {
                dataError( requestData );
                break;
            }
            else
            {
                foreach( QVariant resource, m_chartResources )
                {
                    if( resource.toString() == hash["chart_source"] )
                    {
                        foundSource = true;
                    }
                }

                if( !foundSource )
                {
                    dataError( requestData );
                    break;
                }

            }
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
ChartsPlugin::pushInfo( const QString caller, const Tomahawk::InfoSystem::InfoType type, const QVariant input )
{
    Q_UNUSED( caller )
    Q_UNUSED( type)
    Q_UNUSED( input )
}


void
ChartsPlugin::fetchChart( Tomahawk::InfoSystem::InfoRequestData requestData )
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
        dataError( requestData );
        return;

    }
    /// Set the criterias for current chart
    criteria["chart_id"] = hash["chart_id"];
    criteria["chart_source"] = hash["chart_source"];

    emit getCachedInfo( criteria, 0, requestData );
}

void
ChartsPlugin::fetchChartCapabilities( Tomahawk::InfoSystem::InfoRequestData requestData )
{
    if ( !requestData.input.canConvert< Tomahawk::InfoSystem::InfoStringHash >() )
    {
        dataError( requestData );
        return;
    }

    Tomahawk::InfoSystem::InfoStringHash criteria;
    emit getCachedInfo( criteria, 0, requestData );
}

void
ChartsPlugin::notInCacheSlot( QHash<QString, QString> criteria, Tomahawk::InfoSystem::InfoRequestData requestData )
{
    if ( !m_nam.data() )
    {
        tLog() << "Have a null QNAM, uh oh";
        emit info( requestData, QVariant() );
        return;
    }


    switch ( requestData.type )
    {
        case InfoChart:
        {
            /// Fetch the chart, we need source and id
            QUrl url = QUrl( QString( CHART_URL "source/%1/chart/%2" ).arg( criteria["chart_source"] ).arg( criteria["chart_id"] ) );
            qDebug() << Q_FUNC_INFO << "Getting chart url" << url;

            QNetworkReply* reply = m_nam.data()->get( QNetworkRequest( url ) );
            reply->setProperty( "requestData", QVariant::fromValue< Tomahawk::InfoSystem::InfoRequestData >( requestData ) );

            connect( reply, SIGNAL( finished() ), SLOT( chartReturned() ) );
            return;

        }

        case InfoChartCapabilities:
        {
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
ChartsPlugin::chartTypes()
{
    /// Get possible chart type for specificChartsPlugin: InfoChart types returned chart source
    tDebug() << "Got chart type result";
    QNetworkReply* reply = qobject_cast<QNetworkReply*>( sender() );

    if ( reply->error() == QNetworkReply::NoError )
    {
        QJson::Parser p;
        bool ok;
        const QVariantMap res = p.parse( reply, &ok ).toMap();
        const QVariantMap chartObjs = res.value( "charts" ).toMap();

        if ( !ok )
        {
            tLog() << "Failed to parse resources" << p.errorString() << "On line" << p.errorLine();

            return;
        }

        /// Got types, append!
        const QString source = res.value( "source" ).toString();

        // We'll populate charts with the data from the server
        QVariantMap charts;
        QString chartName;
        QStringList defaultChain;
        if ( source == "itunes" )
        {
            // Itunes has geographic-area based charts. So we build a breadcrumb of
            // ITunes - Country - Albums - Top Chart Type
            //                  - Tracks - Top Chart Type
            QHash< QString, QVariantMap > countries;
            foreach( const QVariant& chartObj, chartObjs.values() )
            {
                const QVariantMap chart = chartObj.toMap();
                const QString id = chart.value( "id" ).toString();
                const QString geo = chart.value( "geo" ).toString();
                QString name = chart.value( "name" ).toString();
                const QString type = chart.value( "type" ).toString();
                const bool isDefault = ( chart.contains( "default" ) && chart[ "default" ].toInt() == 1 );

                QString country;
                if ( !m_cachedCountries.contains( geo ) )
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
                    m_cachedCountries[ geo ] = country;
                }
                else
                {
                    country = m_cachedCountries[ geo ];
                }

                if ( name.startsWith( "iTunes Store:" ) ) // truncate
                    name = name.mid( 13 );

                InfoStringHash c;
                c[ "id" ] = id;
                c[ "label" ] = name;
                c[ "type" ] = "album";
                if ( isDefault )
                    c[ "default" ] = "true";

                QList<InfoStringHash> countryTypeData = countries[ country ][ type ].value< QList< InfoStringHash > >();
                countryTypeData.append( c );

                countries[ country ].insert( type, QVariant::fromValue< QList< InfoStringHash > >( countryTypeData ) );
                if ( isDefault )
                {
                    defaultChain.clear();
                    defaultChain.append( country );
                    defaultChain.append( type );
                    defaultChain.append( name );
                }
            }

            foreach( const QString& c, countries.keys() )
            {
                charts[ c ] = countries[ c ];
//                 qDebug() << "Country has types:" << countries[ c ];
            }
            chartName = "iTunes";
        } else
        {
            // We'll just build:
            // [Source] - Album - Chart Type
            // [Source] - Track - Chart Type
            QList< InfoStringHash > albumCharts;
            QList< InfoStringHash > trackCharts;
            foreach( const QVariant& chartObj, chartObjs.values() )
            {
                const QVariantMap chart = chartObj.toMap();
                const QString type = chart.value( "type" ).toString();
                const bool isDefault = ( chart.contains( "default" ) && chart[ "default" ].toInt() == 1 );

                InfoStringHash c;
                c[ "id" ] = chart.value( "id" ).toString();
                c[ "label" ] = chart.value( "name" ).toString();
                if ( isDefault )
                    c[ "default" ] = "true";

                if ( type == "Album" )
                {
                    c[ "type" ] = "album";
                    albumCharts.append( c );
                }
                else if ( type == "Track" )
                {
                    c[ "type" ] = "tracks";
                    trackCharts.append( c );
                }

                if ( isDefault )
                {
                    defaultChain.clear();
                    defaultChain.append( type + "s" ); //UGLY but it's plural to the user, see below
                    defaultChain.append( c[ "label" ] );
                }
            }
            charts.insert( tr( "Albums" ), QVariant::fromValue< QList< InfoStringHash > >( albumCharts ) );
            charts.insert( tr( "Tracks" ), QVariant::fromValue< QList< InfoStringHash > >( trackCharts ) );

            /// @note For displaying purposes, upper the first letter
            /// @note Remeber to lower it when fetching this!
            chartName = source;
            chartName[0] = chartName[0].toUpper();
        }

        /// Add the possible charts and its types to breadcrumb
//         qDebug() << "ADDING CHART TYPE TO CHARTS:" << chartName;
        QVariantMap defaultMap = m_allChartsMap.value( "defaults" ).value< QVariantMap >();
        defaultMap[ source ] = defaultChain;
        m_allChartsMap[ "defaults" ] = defaultMap;
        m_allChartsMap[ "defaultSource" ] = "itunes";
        m_allChartsMap.insert( chartName , QVariant::fromValue< QVariantMap >( charts ) );

    }
    else
    {
        tLog() << "Error fetching charts:" << reply->errorString();
    }

    m_chartsFetchJobs--;
    if ( !m_cachedRequests.isEmpty() && m_chartsFetchJobs == 0 )
    {
        foreach ( InfoRequestData request, m_cachedRequests )
        {
            emit info( request, m_allChartsMap );
        }
        m_cachedRequests.clear();
    }

}

void
ChartsPlugin::chartReturned()
{

    /// Chart request returned something! Woho
    QNetworkReply* reply = qobject_cast<QNetworkReply*>( sender() );
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
        QVariantList chartResponse = res.value( "list" ).toList();
        QList< InfoStringHash > top_tracks;
        QList< InfoStringHash > top_albums;

        /// Deside what type, we need to handle it differently
        /// @todo: We allready know the type, append it to breadcrumb hash

        if( res.value( "type" ).toString() == "Album" )
            setChartType( Album );
        else if( res.value( "type" ).toString() == "Track" )
            setChartType( Track );
        else
            setChartType( None );


//         qDebug() << "Got chart returned!" << res;
        foreach ( QVariant chartR, chartResponse )
        {
            QString title, artist, album;
            QVariantMap chartMap = chartR.toMap();

            if ( !chartMap.isEmpty() )
            {

                title = chartMap.value( "track" ).toString();
                album = chartMap.value( "album" ).toString();
                artist = chartMap.value( "artist" ).toString();
                /// Maybe we can use rank later on, to display something nice
                /// rank = chartMap.value( "rank" ).toString();

                if ( chartType() == Album )
                {

                    if ( album.isEmpty() && artist.isEmpty() ) // don't have enough...
                    {
                        tLog() << "Didn't get an artist and album name from chart, not enough to build a query on. Aborting" << title << album << artist;

                    }
                    else
                    {
                        qDebug() << Q_FUNC_INFO << album << artist;
                        InfoStringHash pair;
                        pair["artist"] = artist;
                        pair["album"] = album;
                        top_albums << pair;

                    }
                }

                else if ( chartType() == Track )
                {

                    if ( title.isEmpty() && artist.isEmpty() ) // don't have enough...
                    {
                        tLog() << "Didn't get an artist and track name from charts, not enough to build a query on. Aborting" << title << artist << album;

                    }
                    else
                    {

                        InfoStringHash pair;
                        pair["artist"] = artist;
                        pair["track"] = title;
                        top_tracks << pair;

                    }
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

        Tomahawk::InfoSystem::InfoRequestData requestData = reply->property( "requestData" ).value< Tomahawk::InfoSystem::InfoRequestData >();


        emit info( requestData, returnedData );
        // TODO update cache
    }
    else
        qDebug() << "Network error in fetching chart:" << reply->url().toString();

}
