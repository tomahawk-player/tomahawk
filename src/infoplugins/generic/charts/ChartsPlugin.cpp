/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012, Casey Link <unnamedrambler@gmail.com>
 *   Copyright 2011-2012, Hugo Lindström <hugolm84@gmail.com>
 *   Copyright 2011, Leo Franchi <lfranchi@kde.org>
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

#include "ChartsPlugin.h"

#include "Album.h"
#include "CountryUtils.h"
#include "Typedefs.h"
#include "audio/AudioEngine.h"
#include "TomahawkSettings.h"
#include "utils/TomahawkUtils.h"
#include "utils/Logger.h"
#include "utils/TomahawkCache.h"
#include "Source.h"

#include <qjson/parser.h>
#include <qjson/serializer.h>

#include <QDir>
#include <QSettings>
#include <QNetworkConfiguration>
#include <QNetworkReply>

#define CHART_URL "http://charts.tomahawk-player.org/"
//#define CHART_URL "http://localhost:8080/"

namespace Tomahawk
{

namespace InfoSystem
{

ChartsPlugin::ChartsPlugin()
    : InfoPlugin()
    , m_chartsFetchJobs( 0 )
    , m_fetchAll( true )
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << QThread::currentThread();

    m_chartVersion = "2.6.5";
    m_supportedGetTypes <<  InfoChart << InfoChartCapabilities;
    m_cacheIdentifier = TomahawkUtils::md5( QString("ChartsPlugin" + m_chartVersion ).toUtf8() );
}


ChartsPlugin::~ChartsPlugin()
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << QThread::currentThread();
}


void
ChartsPlugin::init()
{
    QVariant data = TomahawkUtils::Cache::instance()->getData( m_cacheIdentifier, "chart_sources" );
    if ( data.canConvert< QList< Tomahawk::InfoSystem::InfoStringHash > >() )
    {
         const QList< Tomahawk::InfoSystem::InfoStringHash > sourceList = data.value< QList< Tomahawk::InfoSystem::InfoStringHash > >();
         foreach ( const Tomahawk::InfoSystem::InfoStringHash &sourceHash, sourceList )
         {
             bool ok;
             qlonglong maxAge = getMaxAge( QString( sourceHash[ "chart_expires" ] ).toLongLong( &ok ) );
             if ( !ok || maxAge <= 0 )
             {
                 // This source has expired.
                 m_refetchSource << sourceHash[ "chart_source" ];
             }
             m_chartResources << sourceHash;
         }

         data = TomahawkUtils::Cache::instance()->getData( m_cacheIdentifier, "allCharts" );

         if ( data.canConvert< QVariantMap >() )
         {
             m_allChartsMap = data.toMap();
             if ( !m_allChartsMap.empty() )
                 m_fetchAll = false;
         }
    }
    else
    {
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Migrating";
    }

    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "total sources" << m_chartResources.size() << m_chartResources;

    if( ( m_chartResources.size() == 0 || m_refetchSource.size() != 0 ) || m_fetchAll )
    {
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Will refetch on next request. Empty or Invalid CACHE" << m_chartResources.size() << m_refetchSource << "fetchAll?" << m_fetchAll;
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
    InfoStringHash hash = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash >();
    bool foundSource = false;

    switch ( requestData.type )
    {
        case InfoChart:
            /// We need something to check if the request is actually ment to go to this plugin
            if ( !hash.contains( "chart_source" ) )
            {
                tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Hash did not contain required param!";
                dataError( requestData );
                break;
            }
            else
            {
                foreach ( const Tomahawk::InfoSystem::InfoStringHash &sourceHash, m_chartResources )
                {
                    if ( sourceHash[ "chart_source" ] == hash[ "chart_source" ] )
                    {
                        foundSource = true;
                    }
                }

                if ( !foundSource )
                {
                    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "no such source" << hash["chart_source"] << "(" << m_chartResources.size() << " total sources)";
                    dataError( requestData );
                    break;
                }

            }
            fetchChartFromCache( requestData );
            break;

        case InfoChartCapabilities:
            fetchChartCapabilitiesFromCache( requestData );
            break;
        default:
            dataError( requestData );
    }
}


void
ChartsPlugin::fetchChartFromCache( Tomahawk::InfoSystem::InfoRequestData requestData )
{
    if ( !requestData.input.canConvert< Tomahawk::InfoSystem::InfoStringHash >() )
    {
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Could not convert requestData to InfoStringHash!";
        dataError( requestData );
        return;
    }

    InfoStringHash hash = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash >();
    Tomahawk::InfoSystem::InfoStringHash criteria;

    /// Each request needs to contain both a id, source and expires param
    if ( !hash.contains( "chart_id" ) && !hash.contains( "chart_source" ) && !hash.contains( "chart_expires" ) )
    {
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Hash did not contain required params!";
        dataError( requestData );
        return;

    }
    /// Set the criterias for current chart
    criteria[ "chart_id" ] = hash[ "chart_id" ];
    criteria[ "chart_source" ] = hash[ "chart_source" ];
    criteria[ "chart_expires" ] = hash[ "chart_expires" ];
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Checking cache for " << hash["chart_id"] << " from " << hash["chart_source"];

    bool ok;
    const qlonglong maxAge = getMaxAge( QString( hash[ "chart_expires" ] ).toLongLong( &ok ) );

    if ( !ok || maxAge <= 0 )
    {
        emit notInCacheSlot( criteria, requestData );
        return;
    }

    emit getCachedInfo( criteria, maxAge, requestData );
    return;
}


void
ChartsPlugin::fetchChartCapabilitiesFromCache( Tomahawk::InfoSystem::InfoRequestData requestData )
{
    if ( !requestData.input.canConvert< Tomahawk::InfoSystem::InfoStringHash >() )
    {
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Could not convert requestData to InfoStringHash!";
        dataError( requestData );
        return;
    }

    Tomahawk::InfoSystem::InfoStringHash criteria;
    criteria[ "InfoChartCapabilities" ] = "chartsplugin";
    criteria[ "InfoChartVersion" ] = m_chartVersion;

    Tomahawk::InfoSystem::InfoStringHash inputData = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash >();

    /// @todo: Only fetch this source, and update charts map
    if( inputData.contains( "chart_refetch" ) )
    {
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Chart source needs to be refetched!" << inputData[ "chart_refetch" ];
        m_refetchSource << inputData[ "chart_refetch" ];
    }

    ///  Someone requested capabilities, but init() told us someone was out of date
    ///  Next fetch will fetch those that are invalid

    if ( m_refetchSource.size() != 0 )
    {
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Some sources need to refetch!" << m_refetchSource;
        emit notInCacheSlot( criteria, requestData );
        return;
    }

    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Checking cache for " << "InfoChartCapabilities" << m_chartVersion;
    emit getCachedInfo( criteria, 172800000 /* 2 days */, requestData );
}


void
ChartsPlugin::notInCacheSlot( QHash<QString, QString> criteria, Tomahawk::InfoSystem::InfoRequestData requestData )
{
    switch ( requestData.type )
    {
        case InfoChart:
        {
            tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "InfoChart not in cache! Fetching...";
            fetchChart( requestData, criteria["chart_source"], criteria["chart_id"] );
            return;
        }

        case InfoChartCapabilities:
        {
            tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "InfoChartCapabilities not in cache! Fetching...";
            fetchChartSourcesList( false );
            m_cachedRequests.append( requestData );
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
ChartsPlugin::fetchChartSourcesList( bool fetchOnlySourceList )
{
    QUrl url = QUrl( QString ( CHART_URL "charts" ) );

    TomahawkUtils::urlAddQueryItem( url, "version", TomahawkUtils::appFriendlyVersion() );

    QNetworkReply* reply = TomahawkUtils::nam()->get( QNetworkRequest( url ) );
    reply->setProperty( "only_source_list", fetchOnlySourceList );

    connect( reply, SIGNAL( finished() ), SLOT( chartSourcesList() ) );
}


void
ChartsPlugin::chartSourcesList()
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Got chart sources list";
    QNetworkReply* reply = qobject_cast<QNetworkReply*>( sender() );

    if ( reply->error() == QNetworkReply::NoError )
    {
        QJson::Parser p;
        bool ok;
        const QVariantMap res = p.parse( reply, &ok ).toMap();
        const QVariantList sources = res.value( "sources" ).toList();

        if ( !ok )
        {
            tLog() << Q_FUNC_INFO << "Failed to parse sources" << p.errorString() << "On line" << p.errorLine();
            return;
        }

        m_chartResources.clear();

        foreach ( const QVariant &rsource, sources )
        {
            /// Each item has an expiration, on next request for cache, it will be checked */
            const QString source = rsource.toString();

            /// Twisted backend Uppers first header letter, and lowers the rest
            QString tmpSource = source + "expires";
            tmpSource[0] = tmpSource[0].toUpper();

            const QString headerExpiration = reply->rawHeader( QString( tmpSource ).toLocal8Bit() );
            const qlonglong maxAge = getMaxAge( headerExpiration.toLocal8Bit() );
            const qlonglong expires = headerExpiration.toLongLong(&ok);
            Tomahawk::InfoSystem::InfoStringHash source_expire;

            if ( ok )
            {
                source_expire[ "chart_source" ] = source;
                source_expire[ "chart_expires" ] = QString::number(expires);
                m_chartResources << source_expire;

                if( !m_fetchAll )
                {
                    if ( maxAge == 0 )
                    {
                        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "MaxAge for " << source << " is  0. reFetching";
                        reply->setProperty( "only_source_list", false );
                    }
                }
                else
                {
                    m_refetchSource << source;
                }
            }
        }

        /// We can store the source list for how long as we want
        /// In init, we check expiration for each source, and refetch if invalid
        /// 2 days seems fair enough though
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "storing sources in cache" << m_chartResources;
        TomahawkUtils::Cache::instance()->putData( m_cacheIdentifier, 172800000 /* 2 days */, "chart_sources", QVariant::fromValue< QList< Tomahawk::InfoSystem::InfoStringHash > > ( m_chartResources ) );

        if( !reply->property( "only_source_list" ).toBool() )
        {

            if ( !m_fetchAll )
            {
                tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "reFetching" << m_refetchSource;
                fetchExpiredSources();
            }
            else
            {
                tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Fetching all sources!" << m_refetchSource;
                fetchAllChartSources();
                m_fetchAll = false;
            }
        }
    }
    else
        tDebug() << Q_FUNC_INFO << "Encountered error fetching chart sources list";
}


void
ChartsPlugin::fetchSource(const QString& source)
{
    QUrl url = QUrl( QString( CHART_URL "charts/%1" ).arg( source ) );
    TomahawkUtils::urlAddQueryItem( url, "version", TomahawkUtils::appFriendlyVersion() );

    QNetworkReply* reply = TomahawkUtils::nam()->get( QNetworkRequest( url ) );
    reply->setProperty( "chart_source", source );

    tDebug() << Q_FUNC_INFO << "fetching:" << url;
    connect( reply, SIGNAL( finished() ), SLOT( chartsList() ) );

    m_chartsFetchJobs++;
}


void
ChartsPlugin::fetchExpiredSources()
{
    if ( !m_refetchSource.isEmpty() )
    {
        foreach ( const QString& source, m_refetchSource )
        {
            fetchSource(source);
        }
    }
}


void
ChartsPlugin::fetchAllChartSources()
{
    if ( !m_chartResources.isEmpty() && m_allChartsMap.isEmpty() )
    {
        foreach ( const Tomahawk::InfoSystem::InfoStringHash source, m_chartResources )
        {
            fetchSource(source[ "chart_source" ]);
        }
    }
}


void
ChartsPlugin::fetchChart( Tomahawk::InfoSystem::InfoRequestData requestData, const QString& source, const QString& chart_id )
{
    /// Fetch the chart, we need source and id
    QUrl url = QUrl ( QString ( CHART_URL "charts/%1/%2" ).arg( source ).arg( chart_id ) );

    TomahawkUtils::urlAddQueryItem( url, "version", TomahawkUtils::appFriendlyVersion() );

    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "fetching: " << url;

    QNetworkReply* reply = TomahawkUtils::nam()->get( QNetworkRequest( url ) );
    reply->setProperty( "requestData", QVariant::fromValue< Tomahawk::InfoSystem::InfoRequestData >( requestData ) );

    connect( reply, SIGNAL( finished() ), SLOT( chartReturned() ) );
}


void
ChartsPlugin::chartsList()
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Got chart list result";
    QNetworkReply* reply = qobject_cast<QNetworkReply*>( sender() );

    if ( reply->error() == QNetworkReply::NoError )
    {
        QJson::Parser p;
        bool ok;
        QVariantMap res = p.parse( reply, &ok ).toMap();

        if ( !ok )
        {
            tLog() << "Failed to parse resources" << p.errorString() << "On line" << p.errorLine();

            return;
        }

        const QVariantMap details = res.take( "details" ).toMap();
        const QVariantMap response = res;
        const bool haveExtra = details.value( "have_extra", false ).toBool();
        // This is what we use as ID
        const QString source = reply->property( "chart_source" ).toString();
        const QString prettyName = details.value( "name", source ).toString();
        const qlonglong expires = QString( reply->rawHeader( QString( "Expires" ).toLocal8Bit() ) ).toLongLong( &ok );

        if ( !ok )
        {
            tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Failed to parse expire headers!";
            tDebug( LOGVERBOSE ) << Q_FUNC_INFO << reply->url();
            tDebug( LOGVERBOSE ) << Q_FUNC_INFO << reply->rawHeaderPairs();
        }

        // We'll populate charts with the data from the server
        QVariantMap charts;
        QStringList defaultChain;

        if ( haveExtra )
        {
            // Some charts can have an extra param, itunes has geo, HNHH has upcomming/all/mainstream
            // Itunes has geographic-area based charts. So we build a breadcrumb of
            // ITunes - Country - Albums - Top Chart Type
            //                  - Tracks - Top Chart Type
            // NameID - Extra - Type -> Chart Name
            QHash< QString, QVariantMap > extraType;
            QStringList processed;

            foreach ( const QVariant& chartObj, response.values() )
            {
                if ( !chartObj.toMap().isEmpty() )
                {
                    const QVariantMap chart = chartObj.toMap();
                    const QString id = chart.value( "id" ).toString();
                    const QString geo = chart.value( "geo" ).toString();
                    const QString name = chart.value( "display_name" ).toString();
                    const QString type = QString( chart.value( "type" ).toString() );
                    const bool isDefault = ( chart.contains( "default" ) && chart[ "default" ].toInt() == 1 );
                    QString extra;

                    // Hack!
                    // Japan charts contains multiple duplicates, all which are linked
                    // back to ONE specific id. So we only parse the first
                    // Should/Could be fixed in the chartserver when its less fragile
                    if ( geo == "jp" && type == "Tracks" )
                    {
                        if ( processed.contains( name ) )
                            continue;
                        processed << name;
                    }

                    if ( !geo.isEmpty() )
                        extra = countryName( geo );
                    else
                        extra = chart.value( "extra" ).toString();

                    InfoStringHash c;
                    c[ "id" ] = id;
                    c[ "label" ] = name;
                    c[ "type" ] = type;

                    if ( isDefault )
                    {
                        c[ "default" ] = "true";
                        defaultChain.clear();
                        defaultChain.append( extra );
                        defaultChain.append( type );
                        defaultChain.append( name );
                    }

                    // If this item has expired, set it to 0.
                    c[ "expires" ] = ( ok ? QString::number (expires ) : QString::number( 0 ) );

                    QList< Tomahawk::InfoSystem::InfoStringHash > extraTypeData = extraType[ extra ][ type ].value< QList< Tomahawk::InfoSystem::InfoStringHash > >();
                    extraTypeData.append( c );
                    extraType[ extra ][ type ] = QVariant::fromValue< QList< Tomahawk::InfoSystem::InfoStringHash > >( extraTypeData );

                }
                foreach ( const QString& c, extraType.keys() )
                {
                    charts[ c ] = extraType[ c ];
                }
            }
        }
        else
        {
            // We'll just build:
            // [Source] - Album - Chart Type
            // [Source] - Track - Chart Type
            QList< InfoStringHash > albumCharts;
            QList< InfoStringHash > trackCharts;
            QList< InfoStringHash > artistCharts;

            foreach ( const QVariant& chartObj, response.values() )
            {
                if ( !chartObj.toMap().isEmpty() )
                {
                    const QVariantMap chart = chartObj.toMap();
                    const QString type = chart.value( "type" ).toString();
                    const QString name = chart.value( "display_name" ).toString();
                    const bool isDefault = ( chart.contains( "default" ) && chart[ "default" ].toInt() == 1 );

                    InfoStringHash c;
                    c[ "id" ] = chart.value( "id" ).toString();
                    c[ "label" ] = name;
                    c[ "expires" ] = ( ok ? QString::number( expires ) : QString::number( 0 ) );
                    c[ "type" ] = type.toLower();

                    if ( isDefault )
                    {
                        c[ "default" ] = "true";
                        defaultChain.clear();
                        defaultChain.append( type );
                        defaultChain.append( c[ "label" ] );
                    }

                    if ( type == "Albums" )
                        albumCharts.append( c );
                    else if ( type == "Tracks" )
                        trackCharts.append( c );
                    else if ( type == "Artists" )
                        artistCharts.append( c );

                }

                if ( !artistCharts.isEmpty() )
                    charts.insert( tr( "Artists" ), QVariant::fromValue< QList< Tomahawk::InfoSystem::InfoStringHash > >( artistCharts ) );
                if ( !albumCharts.isEmpty() )
                    charts.insert( tr( "Albums" ), QVariant::fromValue< QList< Tomahawk::InfoSystem::InfoStringHash > >( albumCharts ) );
                if ( !trackCharts.isEmpty() )
                    charts.insert( tr( "Tracks" ), QVariant::fromValue< QList< Tomahawk::InfoSystem::InfoStringHash > >( trackCharts ) );

            }
        }

        // Add the possible charts and its types to breadcrumb
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Adding to charts:" << prettyName;
        QVariantMap defaultMap = m_allChartsMap.value( "defaults" ).value< QVariantMap >();
        defaultMap[ source ] = defaultChain;
        m_allChartsMap[ "defaults" ] = defaultMap;
        m_allChartsMap[ "defaultSource" ] = "itunes";
        m_allChartsMap.insert( prettyName , QVariant::fromValue< QVariantMap >( charts ) );
        m_refetchSource.removeOne( source );
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Fetched " << source << " have " << m_refetchSource << "left";
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

            tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Updating cache with" << m_allChartsMap.size() << "charts";
            Tomahawk::InfoSystem::InfoStringHash criteria;
            criteria[ "InfoChartCapabilities" ] = "chartsplugin";
            criteria[ "InfoChartVersion" ] = m_chartVersion;

            /// We can cache it the lot for 2 days, it will be checked on next request
            emit updateCache( criteria, 172800000 /* 2 days */, request.type, m_allChartsMap );
        }

        TomahawkUtils::Cache::instance()->putData( m_cacheIdentifier, 172800000 /* 2 days */, "allCharts", m_allChartsMap );
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

        const qlonglong maxAge = getMaxAge( reply->rawHeader( QString( "Expires" ).toLocal8Bit() ) );
        const qlonglong expires = QString( reply->rawHeader( QString( "Expires" ).toLocal8Bit() ) ).toLongLong( &ok );

        /// SO we have a result, parse it!
        QVariantList chartResponse = res.value( "list" ).toList();
        QList< Tomahawk::InfoSystem::InfoStringHash > top_tracks;
        QList< Tomahawk::InfoSystem::InfoStringHash > top_albums;
        QStringList top_artists;

        /// Deside what type, we need to handle it differently
        /// @todo: We allready know the type, append it to breadcrumb hash

        if ( res.value( "type" ).toString() == "Album" )
            setChartType( Album );
        else if ( res.value( "type" ).toString() == "Track" )
            setChartType( Track );
        else if ( res.value( "type" ).toString() == "Artist" )
            setChartType( Artist );
        else
            setChartType( None );

        foreach ( const QVariant& chartR, chartResponse )
        {
            QString title, artist, album, streamUrl;
            QVariantMap chartMap = chartR.toMap();

            if ( !chartMap.isEmpty() )
            {
                title = chartMap.value( "track" ).toString();
                album = chartMap.value( "album" ).toString();
                artist = chartMap.value( "artist" ).toString();
                streamUrl = chartMap.value( "stream_url" ).toString();
                /// Maybe we can use rank later on, to display something nice
                /// rank = chartMap.value( "rank" ).toString();

                if ( chartType() == Album )
                {
                    if ( album.isEmpty() && artist.isEmpty() ) // don't have enough...
                    {
                        tDebug( LOGVERBOSE ) << "Didn't get an artist and album name from chart, not enough to build a query on. Aborting" << title << album << artist;
                    }
                    else
                    {
                        Tomahawk::InfoSystem::InfoStringHash pair;
                        pair["artist"] = artist;
                        pair["album"] = album;
                        top_albums.append( pair );
                    }
                }
                else if ( chartType() == Track )
                {
                    if ( title.isEmpty() && artist.isEmpty() ) // don't have enough...
                    {
                        tDebug( LOGVERBOSE ) << "Didn't get an artist and track name from charts, not enough to build a query on. Aborting" << title << artist << album;
                    }
                    else
                    {
                        Tomahawk::InfoSystem::InfoStringHash pair;
                        pair["artist"] = artist;
                        pair["track"] = title;
                        pair["streamUrl"] = streamUrl;
                        top_tracks.append( pair );
                    }
                }
                else if ( chartType() == Artist )
                {
                    if ( artist.isEmpty() ) // don't have enough...
                    {
                        tDebug( LOGVERBOSE ) << "Didn't get an artist from charts, not enough to build a query on. Aborting" << artist;
                    }
                    else
                    {
                        top_artists.append( artist );
                    }
                }
            }
        }

        if ( chartType() == Artist )
        {
            tDebug( LOGVERBOSE ) << "ChartsPlugin:" << "got" << top_artists.size() << "artists";
            returnedData[ "artists" ] = QVariant::fromValue< QStringList >( top_artists );
            returnedData[ "type" ] = "artists";
        }
        else if ( chartType() == Track )
        {
            tDebug( LOGVERBOSE ) << "ChartsPlugin:" << "got" << top_tracks.size() << "tracks";
            returnedData[ "tracks" ] = QVariant::fromValue< QList< Tomahawk::InfoSystem::InfoStringHash > >( top_tracks );
            returnedData[ "type" ] = "tracks";
        }
        else if ( chartType() == Album )
        {
            tDebug( LOGVERBOSE ) << "ChartsPlugin:" << "got" << top_albums.size() << "albums";
            returnedData[ "albums" ] = QVariant::fromValue< QList< Tomahawk::InfoSystem::InfoStringHash > >( top_albums );
            returnedData[ "type" ] = "albums";
        }

        Tomahawk::InfoSystem::InfoRequestData requestData = reply->property( "requestData" ).value< Tomahawk::InfoSystem::InfoRequestData >();

        emit info( requestData, returnedData );
        // update cache
        Tomahawk::InfoSystem::InfoStringHash criteria;
        Tomahawk::InfoSystem::InfoStringHash origData = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash >();
        criteria[ "chart_id" ] = origData[ "chart_id" ];
        criteria[ "chart_source" ] = origData[ "chart_source" ];
        criteria[ "chart_expires" ] = ( ok ? QString::number( expires ) : QString::number( 0 ) );

        /// If the item has expired, cache it for one hour and try and refetch later
        emit updateCache( criteria, (maxAge == 0 ? 3600000 /* One hour */ : maxAge), requestData.type, returnedData );
    }
    else
    {
        tDebug() << Q_FUNC_INFO << "Network error in fetching chart:" << reply->url().toString();
        Tomahawk::InfoSystem::InfoRequestData requestData = reply->property( "requestData" ).value< Tomahawk::InfoSystem::InfoRequestData >();
        Tomahawk::InfoSystem::InfoStringHash origData = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash >();
        returnedData[ "chart_error" ] = "Invalid ID";
        returnedData[ "chart_source" ] = origData[ "chart_source" ];
        returnedData[ "chart_id" ] = origData[ "chart_id" ];
        emit info( requestData, returnedData );
    }
}


QString
ChartsPlugin::countryName( const QString& cc )
{
    if ( m_cachedCountries.contains( cc ) )
        return m_cachedCountries[ cc ];

    QString name = Tomahawk::CountryUtils::fullCountryFromCode( cc );
    for ( int i = 1; i < name.size(); i++ )
    {
        if ( name.at( i ).isUpper() )
        {
            name.insert( i, " " );
            i++;
        }
    }
    m_cachedCountries[ cc ] = name;
    return name;
}


qlonglong
ChartsPlugin::getMaxAge( const QByteArray &rawHeader ) const
{
    bool ok;
    qlonglong expires = QString( rawHeader ).toLongLong( &ok );
    if ( ok )
    {
        return getMaxAge( expires );
    }
    return 0;
}

qlonglong
ChartsPlugin::getMaxAge( const qlonglong expires ) const
{
    qlonglong currentEpoch = QDateTime::currentMSecsSinceEpoch()/1000;
    qlonglong expiresInSeconds = expires-currentEpoch;

    if ( expiresInSeconds > 0 )
    {
        return ( qlonglong )expiresInSeconds*1000;
    }
    return 0;
}

}

}

Q_EXPORT_PLUGIN2( Tomahawk::InfoSystem::InfoPlugin, Tomahawk::InfoSystem::ChartsPlugin )
