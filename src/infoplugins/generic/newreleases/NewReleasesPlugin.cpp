#include "NewReleasesPlugin.h"

#include <QtCore/QDir>
#include <QtCore/QSettings>
#include <QtNetwork/QNetworkConfiguration>
#include <QtNetwork/QNetworkReply>
#include <QtPlugin>

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

#define CHART_URL "http://charts.tomahawk-player.org/"
//#define CHART_URL "http://localhost:8080/"

using namespace Tomahawk::InfoSystem;

bool newReleaseSort( const InfoStringHash& left, const InfoStringHash& right )
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
    , m_nrFetchJobs ( 0 )
{
    m_nrVersion = "0.4";
    m_supportedGetTypes << InfoNewReleaseCapabilities << InfoNewRelease;
}

NewReleasesPlugin::~NewReleasesPlugin()
{
    tDebug ( LOGVERBOSE ) << Q_FUNC_INFO;
}

void
NewReleasesPlugin::init()
{

    QVariantList source_qvarlist = TomahawkUtils::Cache::instance()->getData( "NewReleasesPlugin", "nr_sources" ).toList();
    foreach( const QVariant & source, source_qvarlist )
    {
        m_nrSources.append( source.toString() );
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "fetched source from cache" << source.toString();

    }
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "total sources" << m_nrSources.size() << source_qvarlist.size();
    if( m_nrSources.size() == 0 )
        fetchNRSourcesList( true );
}

void NewReleasesPlugin::dataError ( InfoRequestData requestData )
{
    emit info ( requestData, QVariant() );
    return;
}

void NewReleasesPlugin::getInfo ( InfoRequestData requestData )
{
//qDebug() << Q_FUNC_INFO << requestData.caller;
    //qDebug() << Q_FUNC_INFO << requestData.customData;

    InfoStringHash hash = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash >();
    bool foundSource = false;

    switch ( requestData.type )
    {
    case InfoNewRelease:
        /// We need something to check if the request is actually ment to go to this plugin
        if ( !hash.contains ( "nr_source" ) )
        {
            tDebug ( LOGVERBOSE ) << Q_FUNC_INFO << "Hash did not contain required param!";
            dataError ( requestData );
            break;
        }
        else
        {
            foreach ( QString resource, m_nrSources )
            {
                if ( resource == hash["nr_source"] )
                {
                    foundSource = true;
                }
            }

            if ( !foundSource )
            {
                dataError ( requestData );
                break;
            }

        }
        fetchNRFromCache ( requestData );
        break;

    case InfoNewReleaseCapabilities:
        fetchNRCapabilitiesFromCache ( requestData );
        break;
    default:
        dataError ( requestData );
    }
}

void NewReleasesPlugin::fetchNRFromCache ( InfoRequestData requestData )
{
    if ( !requestData.input.canConvert< Tomahawk::InfoSystem::InfoStringHash >() )
    {
        dataError ( requestData );
        return;
    }

    InfoStringHash hash = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash >();
    Tomahawk::InfoSystem::InfoStringHash criteria;

    /// Each request needs to contain both a id and source
    if ( !hash.contains ( "nr_id" ) && !hash.contains ( "nr_source" ) )
    {
        tDebug ( LOGVERBOSE ) << Q_FUNC_INFO << "Hash did not contain required params!";
        dataError ( requestData );
        return;

    }
    /// Set the criterias for current chart
    criteria["nr_id"] = hash["nr_id"];
    criteria["nr_source"] = hash["nr_source"];

    emit getCachedInfo ( criteria, 86400000, requestData );
}

void NewReleasesPlugin::fetchNRCapabilitiesFromCache ( InfoRequestData requestData )
{
    if ( !requestData.input.canConvert< Tomahawk::InfoSystem::InfoStringHash >() )
    {
        tDebug ( LOGVERBOSE ) << Q_FUNC_INFO << "Could not convert requestData to InfoStringHash!";
        dataError ( requestData );
        return;
    }

    Tomahawk::InfoSystem::InfoStringHash criteria;
    criteria[ "InfoNewReleaseCapabilities" ] = "newreleasesplugin";
    criteria[ "InfoNewReleaseVersion" ] = m_nrVersion;
    emit getCachedInfo ( criteria, 864000000, requestData );
}

void NewReleasesPlugin::notInCacheSlot ( InfoStringHash criteria, InfoRequestData requestData )
{
    switch ( requestData.type )
    {
    case InfoNewRelease:
    {
        tDebug ( LOGVERBOSE ) << Q_FUNC_INFO << "InfoNewRelease not in cache! Fetching...";
        fetchNR ( requestData, criteria["nr_source"], criteria["nr_id"] );
        return;

    }

    case InfoNewReleaseCapabilities:
    {
        tDebug ( LOGVERBOSE ) << Q_FUNC_INFO << "InfoChartCapabilities not in cache! Fetching...";
        fetchNRSourcesList( false );
        m_cachedRequests.append ( requestData );

        return;
    }

    default:
    {
        tLog() << Q_FUNC_INFO << "Couldn't figure out what to do with this type of request after cache miss";
        emit info ( requestData, QVariant() );
        return;
    }
    }
}

void NewReleasesPlugin::fetchNRSourcesList( bool fetchOnlySourcesList )
{

    QUrl url = QUrl ( QString ( CHART_URL "newreleases" ) );
    QNetworkReply* reply = TomahawkUtils::nam()->get ( QNetworkRequest ( url ) );
    reply->setProperty( "only_source_list", fetchOnlySourcesList );


    tDebug() << "fetching:" << url;
    connect ( reply, SIGNAL ( finished() ), SLOT ( nrSourcesList() ) );

}

void NewReleasesPlugin::nrSourcesList()
{
    tDebug ( LOGVERBOSE )  << "Got newreleases sources list";
    QNetworkReply* reply = qobject_cast<QNetworkReply*> ( sender() );

    if ( reply->error() == QNetworkReply::NoError )
    {
        QJson::Parser p;
        bool ok;
        const QVariantMap res = p.parse ( reply, &ok ).toMap();
        const QVariantList sources = res.value ( "sources" ).toList();

        if ( !ok )
        {
            tLog() << "Failed to parse sources" << p.errorString() << "On line" << p.errorLine();
            return;
        }

        m_nrSources.clear();
        foreach ( const QVariant &source, sources )
        {
            m_nrSources << source.toString();
        }
        TomahawkUtils::Cache::instance()->putData( "NewReleasesPlugin", 172800000 /* 2 days */, "nr_sources", m_nrSources );
        if( !reply->property( "only_source_list" ).toBool() )
            fetchAllNRSources();
    }
}

void NewReleasesPlugin::fetchAllNRSources()
{
    if ( !m_nrSources.isEmpty() && m_allNRsMap.isEmpty() )
    {
        tDebug ( LOGVERBOSE ) << Q_FUNC_INFO << "InfoNewRelease fetching source data";
        foreach ( QString source, m_nrSources )
        {
            QUrl url = QUrl ( QString ( CHART_URL "newreleases/%1?version=%2" ).arg ( source ).arg( TomahawkUtils::appFriendlyVersion() ) );
            QNetworkReply* reply = TomahawkUtils::nam()->get ( QNetworkRequest ( url ) );
            reply->setProperty ( "nr_source", source );

            tDebug() << "fetching:" << url;
            connect ( reply, SIGNAL ( finished() ), SLOT ( nrList() ) );

            m_nrFetchJobs++;
        }
    }
}

void NewReleasesPlugin::fetchNR ( InfoRequestData requestData, const QString& source, const QString& nr_id )
{
    /// Fetch the chart, we need source and id
    QUrl url = QUrl ( QString ( CHART_URL "newreleases/%1/%2" ).arg ( source ).arg ( nr_id ) );
    tDebug ( LOGVERBOSE ) << Q_FUNC_INFO << "fetching: " << url;

    QNetworkReply* reply = TomahawkUtils::nam()->get ( QNetworkRequest ( url ) );
    reply->setProperty ( "requestData", QVariant::fromValue< Tomahawk::InfoSystem::InfoRequestData > ( requestData ) );

    connect ( reply, SIGNAL ( finished() ), SLOT ( nrReturned() ) );
}

void NewReleasesPlugin::nrList()
{
    tDebug ( LOGVERBOSE )  << "Got newreleases list result";
    QNetworkReply* reply = qobject_cast<QNetworkReply*> ( sender() );

    if ( reply->error() == QNetworkReply::NoError )
    {
        QJson::Parser p;
        bool ok;
        const QVariantMap res = p.parse ( reply, &ok ).toMap();

        if ( !ok )
        {
            tLog() << "Failed to parse resources" << p.errorString() << "On line" << p.errorLine();

            return;
        }

        /// Got types, append!
        const QString source = reply->property ( "nr_source" ).toString();

        // We'll populate newreleases with the data from the server
        QVariantMap newreleases;
        QString nrName;

        // Building:
        // [Source] - New Release
        QList< InfoStringHash > albumNRs;
        QHash< QString, QVariantMap > extraType;
        foreach ( const QVariant &nrObj, res.values() )
        {
            if ( !nrObj.toMap().isEmpty() )
            {
                const QVariantMap nrMap = nrObj.toMap();
                const QString type = nrMap.value ( "type" ).toString();
                const QString extra = nrMap.value( "extra" ).toString();

                InfoStringHash nr;
                nr["id"] = nrMap.value ( "id" ).toString();
                nr["label"] = nrMap.value ( "name" ).toString();
                nr["date"] = nrMap.value ( "date" ).toString();

                if ( type == "Album" )
                {
                    nr[ "type" ] = "album";

                    if( !extra.isEmpty() )
                    {
                        qDebug() << "FOUND EXTRA!! " << extra;
                        QList< Tomahawk::InfoSystem::InfoStringHash > extraTypeData = extraType[ extra ][ type ].value< QList< Tomahawk::InfoSystem::InfoStringHash > >();
                        extraTypeData.append( nr );
                        extraType[ extra ][ type ] = QVariant::fromValue< QList< Tomahawk::InfoSystem::InfoStringHash > >( extraTypeData );
                    }
                    else
                        albumNRs.append ( nr );
                }
                else
                {
                    tLog() << "Unknown newrelease type " << type;
                    continue;
                }

            }

            foreach( const QString& c, extraType.keys() )
            {
                newreleases[ c ] = extraType[ c ];
                tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "extraType has types:" << c;
            }
        }

        if ( !albumNRs.isEmpty() )
            newreleases.insert ( tr ( "Albums" ), QVariant::fromValue< QList< Tomahawk::InfoSystem::InfoStringHash > > ( albumNRs ) );


        /// @note For displaying purposes, upper the first letter
        /// @note Remeber to lower it when fetching this!
        nrName = source;
        nrName[0] = nrName[0].toUpper();

        tDebug ( LOGVERBOSE ) << Q_FUNC_INFO << "ADDING newrelease TO NRS:" << nrName;
        QVariantMap defaultMap = m_allNRsMap.value ( "defaults" ).value< QVariantMap >();
        m_allNRsMap.insert ( nrName, QVariant::fromValue< QVariantMap > ( newreleases ) );

    } else {
        tLog() << "Error fetching charts:" << reply->errorString();
    }

    m_nrFetchJobs--;
    if ( !m_cachedRequests.isEmpty() && m_nrFetchJobs == 0 )
    {
        foreach ( InfoRequestData request, m_cachedRequests )
        {
            emit info ( request, m_allNRsMap );
            // update cache
            Tomahawk::InfoSystem::InfoStringHash criteria;
            criteria[ "InfoNewReleaseCapabilities" ] = "newreleasesplugin";
            criteria[ "InfoNewReleaseVersion" ] = m_nrVersion;
            emit updateCache ( criteria, 864000000, request.type, m_allNRsMap );
        }
        m_cachedRequests.clear();
    }
}

void NewReleasesPlugin::nrReturned()
{
    /// Chart request returned something! Woho
    QNetworkReply* reply = qobject_cast<QNetworkReply*> ( sender() );
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

        /// SO we have a result, parse it!
        QVariantList albumList = res.value ( "list" ).toList();
        QList< Tomahawk::InfoSystem::InfoStringHash > newreleases;

        foreach( const QVariant & albumObj, albumList )
        {
            QVariantMap albumMap = albumObj.toMap();
            if( !albumMap.isEmpty() )
            {
                const QString album = albumMap.value("album").toString();
                const QString artist = albumMap.value("artist").toString();
                const QString date = albumMap.value("date").toString();

                Tomahawk::InfoSystem::InfoStringHash pair;
                pair["artist"] = artist;
                pair["album"] = album;
                pair["date"] = date;
                newreleases.append( pair );
            }
        }

        qSort( newreleases.begin(), newreleases.end(), newReleaseSort );

//        tDebug() << "NewReleasesPlugin:" << "\tgot " << newreleases.size() << " albums";
        returnedData[ "albums" ] = QVariant::fromValue< QList< Tomahawk::InfoSystem::InfoStringHash > >( newreleases );
        returnedData[ "type" ] = "albums";
        Tomahawk::InfoSystem::InfoRequestData requestData = reply->property( "requestData" ).value< Tomahawk::InfoSystem::InfoRequestData >();

        emit info( requestData, returnedData );
        // update cache
        Tomahawk::InfoSystem::InfoStringHash criteria;
        Tomahawk::InfoSystem::InfoStringHash origData = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash >();
        criteria[ "nr_id" ] = origData[ "nr_id" ];
        criteria[ "nr_source" ] = origData[ "nr_source" ];
        emit updateCache( criteria, 86400000, requestData.type, returnedData );
    } else
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Network error in fetching newrelease:" << reply->url().toString();
}


Q_EXPORT_PLUGIN2( Tomahawk::InfoSystem::InfoPlugin, Tomahawk::InfoSystem::NewReleasesPlugin )
