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

#include "NewReleasesPlugin.h"

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

using namespace Tomahawk::InfoSystem;


bool
newReleaseSort( const InfoStringHash& left, const InfoStringHash& right )
{
    if ( !left.contains( "date" ) || !right.contains( "date" ) )
    {
        return true;
    }

    const QDate lDate = QDate::fromString( left[ "date" ], "yyyy-MM-dd" );
    const QDate rDate = QDate::fromString( right[ "date" ], "yyyy-MM-dd" );

    return lDate > rDate;
}


NewReleasesPlugin::NewReleasesPlugin()
    : InfoPlugin()
    , m_nrFetchJobs( 0 )
{
    m_nrVersion = "0.5";
    m_supportedGetTypes << InfoNewReleaseCapabilities << InfoNewRelease;
}


NewReleasesPlugin::~NewReleasesPlugin()
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO;
}

/**
 * @brief NewReleasesPlugin::init
 * Loops through cache expiration timestamps
 * Refetches source/items if invalid
 */
void
NewReleasesPlugin::init()
{
    QVariant data = TomahawkUtils::Cache::instance()->getData( "NewReleasesPlugin", "nr_sources" );
    if ( data.canConvert< QList< Tomahawk::InfoSystem::InfoStringHash > >() )
    {
         const QList< Tomahawk::InfoSystem::InfoStringHash > sourceList = data.value< QList< Tomahawk::InfoSystem::InfoStringHash > >();
         foreach ( const Tomahawk::InfoSystem::InfoStringHash &sourceHash, sourceList )
         {
             bool ok;
             qlonglong maxAge = getMaxAge( QString( sourceHash[ "nr_expires" ] ).toLongLong( &ok ) );
             if ( !ok || maxAge <= 0 )
             {
                 // This source has expired.
                 m_refetchSource << sourceHash[ "nr_source" ];
             }
             m_nrSources << sourceHash;
         }
    }
    else
    {
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Migrating";
        m_refetchSource << "ALL";
    }

    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "total sources" << m_nrSources.size() << m_nrSources;

    if( m_nrSources.size() == 0 || m_refetchSource.size() != 0 )
    {
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Will refetch on next request. Empty or Invalid CACHE" << m_nrSources.size() << m_refetchSource;
    }
}

void
NewReleasesPlugin::dataError( InfoRequestData requestData )
{
    emit info( requestData, QVariant() );
    return;
}


void
NewReleasesPlugin::getInfo( InfoRequestData requestData )
{

    InfoStringHash hash = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash >();
    bool foundSource = false;

    switch( requestData.type )
    {
    case InfoNewRelease:
        /// We need something to check if the request is actually ment to go to this plugin
        if ( !hash.contains( "nr_source" ) )
        {
            tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Hash did not contain required param!";
            dataError( requestData );
            break;
        }
        else
        {
            foreach ( const Tomahawk::InfoSystem::InfoStringHash &sourceHash, m_nrSources )
            {
                if ( sourceHash[ "nr_source" ] == hash[ "nr_source" ] )
                {
                    foundSource = true;
                }
            }

            if ( !foundSource )
            {
                tDebug ( LOGVERBOSE ) << Q_FUNC_INFO << "Hash did not contain source " << hash["nr_source"];
                dataError ( requestData );
                break;
            }

        }
        fetchNRFromCache( requestData );
        break;

    case InfoNewReleaseCapabilities:
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Requesting InfoNewReleaseCapabilities from cache";
        fetchNRCapabilitiesFromCache( requestData );
        break;

    default:
        dataError( requestData );
    }
}


void
NewReleasesPlugin::fetchNRFromCache( InfoRequestData requestData )
{
    if ( !requestData.input.canConvert< Tomahawk::InfoSystem::InfoStringHash >() )
    {
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Hash did not contain source " << requestData.input;
        dataError( requestData );
        return;
    }

    InfoStringHash hash = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash >();
    Tomahawk::InfoSystem::InfoStringHash criteria;

    /// Each request needs to contain both a id, source a expire header
    if ( !hash.contains( "nr_id" ) && !hash.contains( "nr_source" ) && !hash.contains( "nr_expires" ) )
    {
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Hash did not contain required params!";
        dataError( requestData );
        return;

    }
    /// Set the criterias for current chart
    criteria[ "nr_id" ] = hash[ "nr_id" ];
    criteria[ "nr_source" ] = hash[ "nr_source" ];
    criteria[ "nr_expires" ] = hash[ "nr_expires" ];

    bool ok;
    const qlonglong maxAge = getMaxAge( QString( hash[ "nr_expires" ] ).toLongLong( &ok ) );

    if ( !ok || maxAge <= 0 )
    {
        emit notInCacheSlot( criteria, requestData );
        return;
    }

    emit getCachedInfo( criteria, maxAge, requestData );
    return;
}


void
NewReleasesPlugin::fetchNRCapabilitiesFromCache( InfoRequestData requestData )
{
    if ( !requestData.input.canConvert< Tomahawk::InfoSystem::InfoStringHash >() )
    {
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Could not convert requestData to InfoStringHash!";
        dataError( requestData );
        return;
    }

    Tomahawk::InfoSystem::InfoStringHash criteria;
    criteria[ "InfoNewReleaseCapabilities" ] = "newreleasesplugin";
    criteria[ "InfoNewReleaseVersion" ] = m_nrVersion;

    /**
     * Someone requested capabilities, but init() told us someone was out of date
     * Next fetch will fetch those that are invalid
     */
    if ( m_refetchSource.size() != 0 )
    {
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Some sources need to refetch!" << m_refetchSource;
        emit notInCacheSlot( criteria, requestData );
        return;
    }

    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Fetching fetchNRCapabilitiesFromCache";
    emit getCachedInfo ( criteria, 172800000 /* 2 days */, requestData );
}


void
NewReleasesPlugin::notInCacheSlot( InfoStringHash criteria, InfoRequestData requestData )
{
    switch( requestData.type )
    {
        case InfoNewRelease:
        {
            tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "InfoNewRelease not in cache! Fetching...";
            fetchNR( requestData, criteria[ "nr_source" ], criteria[ "nr_id" ] );
            m_cachedRequests.append( requestData );
            return;
        }

        case InfoNewReleaseCapabilities:
        {
            tDebug ( LOGVERBOSE ) << Q_FUNC_INFO << "InfoChartCapabilities not in cache! Fetching..." << criteria << requestData.requestId;

            QUrl url = QUrl( QString ( CHART_URL "newreleases" ) );

            TomahawkUtils::urlAddQueryItem( url, "version", TomahawkUtils::appFriendlyVersion() );

            QNetworkReply* reply = TomahawkUtils::nam()->get( QNetworkRequest ( url ) );
            reply->setProperty( "only_source_list", true );

            tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "fetching:" << url;
            connect( reply, SIGNAL ( finished() ), SLOT ( nrSourcesList() ) );

            m_nrFetchJobs++;

            if ( m_nrFetchJobs > 0 )
            {
                qDebug() << Q_FUNC_INFO << "InfoChartCapabilities still fetching!";
                m_cachedRequests.append( requestData );
                return;
            }

            emit info( requestData, m_allNRsMap );
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
NewReleasesPlugin::nrSourcesList()
{
    tDebug ( LOGVERBOSE ) << Q_FUNC_INFO << "Got newreleases sources list";
    QNetworkReply* reply = qobject_cast<QNetworkReply*>( sender() );

    if ( reply->error() == QNetworkReply::NoError )
    {
        QJson::Parser p;
        bool ok;
        const QVariantMap res = p.parse ( reply, &ok ).toMap();
        const QVariantList sources = res.value ( "sources" ).toList();

        if ( !ok )
        {
            tDebug() << Q_FUNC_INFO  << "Failed to parse sources" << p.errorString() << "On line" << p.errorLine();
            return;
        }

        foreach ( const QVariant &rsource, sources )
        {

            /**
             * Get the maxAge for this source, update if invalid
             * We dont want to refetch all if not necessary
             */
            const QString source = rsource.toString();

            if ( !m_refetchSource.contains( source ) && !m_allNRsMap.empty() )
            {
                tDebug() << Q_FUNC_INFO  << "Skipping fetch of valid source" << source;
                continue;
            }

            if ( !m_nrSources.isEmpty() )
            {
                for ( int i = 0; i < m_nrSources.size(); i++ )
                {
                    const Tomahawk::InfoSystem::InfoStringHash &hash = m_nrSources.at( i );
                    if ( hash[ "nr_source" ] == source )
                    {
                        tDebug() << Q_FUNC_INFO  << "Removing invalid source" << source;
                        m_nrSources.removeAt( i );
                    }
                }
                reply->setProperty( "only_source_list", false );
            }

            /**
             * @brief Expiration
             * Each item has an expiration, on next request for cache, it will be checked
             */

            /// Twisted backend Uppers first header letter, and lowers the rest
            QString tmpSource = source + "expires";
            tmpSource[0] = tmpSource[0].toUpper();
            const QString headerExpiration = reply->rawHeader( QString( tmpSource ).toLocal8Bit() );

            const qlonglong maxAge = getMaxAge( headerExpiration.toLocal8Bit() );
            const qlonglong expires = headerExpiration.toLongLong(&ok);
            Tomahawk::InfoSystem::InfoStringHash source_expire;

            if ( ok )
            {
                source_expire[ "nr_source" ] = source;
                source_expire[ "nr_expires" ] = QString::number(expires);
                m_nrSources << source_expire;
            }

            if ( maxAge == 0 )
            {
                tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "MaxAge for " << source << " is  0. Fetching all";
                reply->setProperty( "only_source_list", false );
            }

        }

        /**
         * We can store the source list for how long as we want
         * In init, we check expiration for each source, and refetch if invalid
         * 2 days seems fair enough though
         */
        TomahawkUtils::Cache::instance()->putData( "NewReleasesPlugin", 172800000 /* 2 days */, "nr_sources", QVariant::fromValue< QList< Tomahawk::InfoSystem::InfoStringHash > > ( m_nrSources ) );
        m_nrFetchJobs--;

        if( !reply->property( "only_source_list" ).toBool() )
        {
            tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Fetching all sources!";
            fetchAllNRSources();
        }
    }
}

void
NewReleasesPlugin::fetchAllNRSources()
{
    if ( !m_nrSources.isEmpty() && m_allNRsMap.isEmpty() )
    {
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "InfoNewRelease fetching source data";
        foreach ( const Tomahawk::InfoSystem::InfoStringHash source, m_nrSources )
        {
            QUrl url = QUrl( QString( CHART_URL "newreleases/%1" ).arg( source[ "nr_source" ] ) );

            TomahawkUtils::urlAddQueryItem( url, "version", TomahawkUtils::appFriendlyVersion() );

            QNetworkReply* reply = TomahawkUtils::nam()->get( QNetworkRequest( url ) );
            reply->setProperty( "nr_source", source[ "nr_source" ] );

            tDebug() << Q_FUNC_INFO << "fetching:" << url;
            connect( reply, SIGNAL( finished() ), SLOT( nrList() ) );

            m_nrFetchJobs++;
        }
    }
}


void
NewReleasesPlugin::fetchNR( InfoRequestData requestData, const QString& source, const QString& nr_id )
{
    /// Fetch the chart, we need source and id
    QUrl url = QUrl ( QString ( CHART_URL "newreleases/%1/%2" ).arg( source ).arg( nr_id ) );

    TomahawkUtils::urlAddQueryItem( url, "version", TomahawkUtils::appFriendlyVersion() );

    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "fetching: " << url;

    QNetworkReply* reply = TomahawkUtils::nam()->get( QNetworkRequest( url ) );
    reply->setProperty( "requestData", QVariant::fromValue< Tomahawk::InfoSystem::InfoRequestData >( requestData ) );

    connect ( reply, SIGNAL( finished() ), SLOT( nrReturned() ) );
}


void
NewReleasesPlugin::nrList()
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Got newreleases list result";
    QNetworkReply* reply = qobject_cast<QNetworkReply*>( sender() );

    if ( reply->error() == QNetworkReply::NoError )
    {
        QJson::Parser p;
        bool ok;
        const QVariantMap res = p.parse( reply, &ok ).toMap();

        if ( !ok )
        {
            tLog() << "Failed to parse resources" << p.errorString() << "On line" << p.errorLine();
            return;
        }

        /// Got types, append!
        const QString source = reply->property( "nr_source" ).toString();

        /// Twisted backend Uppers first header letter, and lowers the rest
        QString tmpSource = source + "expires";
        tmpSource[0] = tmpSource[0].toUpper();
        const qlonglong expires = QString( reply->rawHeader( QString( tmpSource ).toLocal8Bit() ) ).toLongLong( &ok );

        // We'll populate newreleases with the data from the server
        QVariantMap newreleases;
        QString nrName;
        QStringList defaultChain;

        QList< InfoStringHash > albumNRs;
        QHash< QString, QVariantMap > extraType;

        if( source == "itunes" )
        {
            // Itunes has geographic-area based releases. So we build a breadcrumb of
            // iTunes - Country - Featured/Just Released/New Releases - Genre
            foreach ( const QVariant &nrObj, res.values() )
            {
                if ( !nrObj.toMap().isEmpty() )
                {
                    const QVariantMap nrMap = nrObj.toMap();
                    const QString id = nrMap.value( "id" ).toString();
                    const QString geo = nrMap.value( "geo" ).toString();
                    const QString name = nrMap.value( "genre" ).toString();
                    const QString type = QString( nrMap.value( "type" ).toString() + "s" );
                    const QString nrExtraType = nrMap.value( "extra" ).toString();
                    const bool isDefault = ( nrMap.contains( "default" ) && nrMap[ "default" ].toInt() == 1 );

                    // We only have albums in newReleases
                    if ( type != "Albums" || name.isEmpty() )
                        continue;

                    QString extra;
                    if ( !geo.isEmpty() )
                    {
                        if ( !m_cachedCountries.contains( geo ) )
                        {
                            extra = Tomahawk::CountryUtils::fullCountryFromCode( geo );
                            if ( extra.isEmpty() || extra.isNull() ){
                                qWarning() << "Geo string seems to be off!" << geo;
                                continue;
                            }
                            for ( int i = 1; i < extra.size(); i++ )
                            {
                                if ( extra.at( i ).isUpper() )
                                {
                                    extra.insert( i, " " );
                                    i++;
                                }
                            }
                            m_cachedCountries[ geo ] = extra;
                        }
                        else
                        {
                            extra = m_cachedCountries[ geo ];
                        }
                    }
                    else
                    {
                        // No geo? Extra is the type, eg. Album
                        extra = type;
                    }

                    InfoStringHash nr;
                    nr[ "id" ] = id;
                    nr[ "label" ] = name;
                    nr[ "type" ] = "album";
                    /**
                     * If this item has expired, set it to 0.
                     */
                    nr[ "expires" ] = ( ok ? QString::number (expires ) : QString::number( 0 ) );

                    if ( isDefault )
                        nr[ "default" ] = "true";


                    QList< Tomahawk::InfoSystem::InfoStringHash > extraTypeData = extraType[ nrExtraType ][ extra ].value< QList< Tomahawk::InfoSystem::InfoStringHash > >();
                    extraTypeData.append( nr );
                    extraType[ nrExtraType ][ extra ] = QVariant::fromValue< QList< Tomahawk::InfoSystem::InfoStringHash > >( extraTypeData );

                    if ( isDefault )
                    {
                        defaultChain.clear();
                        defaultChain.append( nrExtraType );
                        defaultChain.append( type );
                        defaultChain.append( name );
                    }
                }
            }

            foreach ( const QString& nr, extraType.keys() )
            {
                newreleases[ nr ] = extraType[ nr ];
            }

            if ( source == "itunes" )
            {
                nrName = "iTunes";
            }

        }
        else
        {
            foreach ( const QVariant &nrObj, res.values() )
            {
                if ( !nrObj.toMap().isEmpty() )
                {
                    const QVariantMap nrMap = nrObj.toMap();
                    const QString type = nrMap.value ( "type" ).toString();
                    const QString extra = nrMap.value( "extra" ).toString();

                    InfoStringHash nr;
                    nr[ "id" ] = nrMap.value( "id" ).toString();
                    nr[ "label" ] = nrMap.value( "name" ).toString();
                    nr[ "date" ] = nrMap.value( "date" ).toString();
                    nr[ "expires" ] = ( ok ? QString::number( expires ) : QString::number( 0 ) );

                    if ( type == "Album" )
                    {
                        nr[ "type" ] = "album";

                        if ( !extra.isEmpty() )
                        {
                            QList< Tomahawk::InfoSystem::InfoStringHash > extraTypeData = extraType[ extra ][ type ].value< QList< Tomahawk::InfoSystem::InfoStringHash > >();
                            extraTypeData.append( nr );
                            extraType[ extra ][ type ] = QVariant::fromValue< QList< Tomahawk::InfoSystem::InfoStringHash > >( extraTypeData );
                        }
                        else
                            albumNRs.append( nr );
                    }
                    else
                    {
                        tLog() << "Unknown newrelease type " << type;
                        continue;
                    }

                }

                foreach ( const QString& c, extraType.keys() )
                {
                    newreleases[ c ] = extraType[ c ];
                }
            }

            if ( !albumNRs.isEmpty() )
                newreleases.insert ( tr ( "Albums" ), QVariant::fromValue< QList< Tomahawk::InfoSystem::InfoStringHash > >( albumNRs ) );

            /// @note For displaying purposes, upper the first letter
            /// @note Remeber to lower it when fetching this!
            nrName = source;
            nrName[0] = nrName[0].toUpper();
        }

        /// Add the possible charts and its types to breadcrumb
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "ADDING newrelease TO NRS:" << nrName << newreleases;
        m_allNRsMap.insert( nrName, QVariant::fromValue< QVariantMap >( newreleases ) );
    }
    else
    {
        tLog() << "Error fetching charts:" << reply->errorString();
    }

    m_nrFetchJobs--;

    if ( !m_cachedRequests.isEmpty() && m_nrFetchJobs == 0 )
    {
        foreach ( InfoRequestData request, m_cachedRequests )
        {
            emit info( request, m_allNRsMap );
            // update cache
            Tomahawk::InfoSystem::InfoStringHash criteria;
            criteria[ "InfoNewReleaseCapabilities" ] = "newreleasesplugin";
            criteria[ "InfoNewReleaseVersion" ] = m_nrVersion;
            /**
             * We can cache it the lot for 2 days, it will be checked on next request
             */
            emit updateCache( criteria, 172800000 /* 2 days */, request.type, m_allNRsMap );
        }
        m_cachedRequests.clear();
    }
}

qlonglong
NewReleasesPlugin::getMaxAge( const QByteArray &rawHeader ) const
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
NewReleasesPlugin::getMaxAge( const qlonglong expires ) const
{
    qlonglong currentEpoch = QDateTime::currentMSecsSinceEpoch()/1000;
    qlonglong expiresInSeconds = expires-currentEpoch;

    if ( expiresInSeconds > 0 )
    {
        return ( qlonglong )expiresInSeconds*1000;
    }
    return 0;
}

void
NewReleasesPlugin::nrReturned()
{
    /// Chart request returned something! Woho
    QNetworkReply* reply = qobject_cast<QNetworkReply*>( sender() );
    QVariantMap returnedData;

    if ( reply->error() == QNetworkReply::NoError )
    {
        QJson::Parser p;
        bool ok;
        QVariantMap res = p.parse ( reply, &ok ).toMap();

        if ( !ok )
        {
            tLog() << "Failed to parse json from chart lookup:" << p.errorString() << "On line" << p.errorLine();
            return;
        }

        const qlonglong maxAge = getMaxAge( reply->rawHeader( QString( "Expires" ).toLocal8Bit() ) );
        const qlonglong expires = QString( reply->rawHeader( QString( "Expires" ).toLocal8Bit() ) ).toLongLong( &ok );

        /// SO we have a result, parse it!
        QVariantList albumList = res.value( "list" ).toList();
        QList< Tomahawk::InfoSystem::InfoStringHash >newreleases;

        foreach( const QVariant & albumObj, albumList )
        {
            QVariantMap albumMap = albumObj.toMap();
            if( !albumMap.isEmpty() )
            {
                const QString album = albumMap.value( "album" ).toString();
                const QString artist = albumMap.value( "artist" ).toString();
                const QString date = albumMap.value( "date" ).toString();

                Tomahawk::InfoSystem::InfoStringHash pair;
                pair[ "artist" ] = artist;
                pair[ "album" ] = album;
                pair[ "date" ] = date;
                newreleases.append( pair );
            }
        }

        qSort( newreleases.begin(), newreleases.end(), newReleaseSort );

        returnedData[ "albums" ] = QVariant::fromValue< QList< Tomahawk::InfoSystem::InfoStringHash > >( newreleases );
        returnedData[ "type" ] = "albums";
        Tomahawk::InfoSystem::InfoRequestData requestData = reply->property( "requestData" ).value< Tomahawk::InfoSystem::InfoRequestData >();

        emit info( requestData, returnedData );

        // update cache
        Tomahawk::InfoSystem::InfoStringHash criteria;
        Tomahawk::InfoSystem::InfoStringHash origData = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash >();
        criteria[ "nr_id" ] = origData[ "nr_id" ];
        criteria[ "nr_source" ] = origData[ "nr_source" ];
        criteria[ "nr_expires" ] = ( ok ? QString::number( expires ) : QString::number( 0 ) );

        /**
         * If the item has expired, cache it for one hour and try and refetch later
         */
        emit updateCache( criteria, (maxAge == 0 ? 3600000 /* One hour */ : maxAge), requestData.type, returnedData );
    }
    else
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Network error in fetching newrelease:" << reply->url().toString();
}

Q_EXPORT_PLUGIN2( Tomahawk::InfoSystem::InfoPlugin, Tomahawk::InfoSystem::NewReleasesPlugin )
