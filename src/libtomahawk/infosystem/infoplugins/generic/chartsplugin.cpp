/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Hugo Lindström <hugolm84@gmail.com>
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
#include <QDomElement>

#include "album.h"
#include "typedefs.h"
#include "audio/audioengine.h"
#include "tomahawksettings.h"
#include "utils/tomahawkutils.h"
#include "utils/logger.h"

#include <lastfm/ws.h>

#define CHART_URL "http://charts.tomahawk-player.org:10080/"
#include <qjson/parser.h>

using namespace Tomahawk::InfoSystem;

static QString
md5( const QByteArray& src )
{
    QByteArray const digest = QCryptographicHash::hash( src, QCryptographicHash::Md5 );
    return QString::fromLatin1( digest.toHex() ).rightJustified( 32, '0' );
}


ChartsPlugin::ChartsPlugin()
    : InfoPlugin()
{

    tDebug() << "ChartsPlugin: InfoChart fetching possible resources";

    QUrl url = QUrl(CHART_URL);
    QNetworkReply* reply = lastfm::nam()->get( QNetworkRequest( url ) );
    connect( reply, SIGNAL( finished() ), SLOT( chartResources() ) );

    m_supportedGetTypes <<  InfoChart << InfoChartCapabilities;

}


ChartsPlugin::~ChartsPlugin()
{
    qDebug() << Q_FUNC_INFO;
}


void
ChartsPlugin::namChangedSlot( QNetworkAccessManager *nam )
{
    qDebug() << Q_FUNC_INFO;
    if( !nam )
        return;
}


void
ChartsPlugin::dataError( uint requestId, Tomahawk::InfoSystem::InfoRequestData requestData )
{
    emit info( requestId, requestData, QVariant() );
    return;
}


void
ChartsPlugin::getInfo( uint requestId, Tomahawk::InfoSystem::InfoRequestData requestData )
{
    switch ( requestData.type )
    {

        case InfoChart:
            fetchChart( requestId, requestData );
            break;

        case InfoChartCapabilities:
            fetchChartCapabilities( requestId, requestData );
            break;
        default:
            dataError( requestId, requestData );
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
ChartsPlugin::fetchChart( uint requestId, Tomahawk::InfoSystem::InfoRequestData requestData )
{
    if ( !requestData.input.canConvert< Tomahawk::InfoSystem::InfoCriteriaHash >() )
    {
        dataError( requestId, requestData );
        return;
    }
    InfoCriteriaHash hash = requestData.input.value< Tomahawk::InfoSystem::InfoCriteriaHash >();
    Tomahawk::InfoSystem::InfoCriteriaHash criteria;
    if ( !hash.contains( "chart_id" ) )
    {
        dataError( requestId, requestData );
        return;
    } else {
        criteria["chart_id"] = hash["chart_id"];
    }
    if ( hash.contains( "chart_source" ) )
    {
        criteria["chart_source"] = hash["chart_source"];
    }

    emit getCachedInfo( requestId, criteria, 0, requestData );
}

void
ChartsPlugin::fetchChartCapabilities( uint requestId, Tomahawk::InfoSystem::InfoRequestData requestData )
{
    if ( !requestData.input.canConvert< Tomahawk::InfoSystem::InfoCriteriaHash >() )
    {
        dataError( requestId, requestData );
        return;
    }
    InfoCriteriaHash hash = requestData.input.value< Tomahawk::InfoSystem::InfoCriteriaHash >();
    Tomahawk::InfoSystem::InfoCriteriaHash criteria;

    emit getCachedInfo( requestId, criteria, 0, requestData );
}

void
ChartsPlugin::notInCacheSlot( uint requestId, QHash<QString, QString> criteria, Tomahawk::InfoSystem::InfoRequestData requestData )
{
    if ( !lastfm::nam() )
    {
        tLog() << "Have a null QNAM, uh oh";
        emit info( requestId, requestData, QVariant() );
        return;
    }

    switch ( requestData.type )
    {
        case InfoChart:
        {

            QUrl url = QUrl( QString( CHART_URL "/source/%1/chart/%2" ).arg( criteria["chart_source"] ).arg( criteria["chart_id"] ) );
            qDebug() << Q_FUNC_INFO << "Getting chart url" << url;

            QNetworkReply* reply = lastfm::nam()->get( QNetworkRequest( url ) );
            reply->setProperty( "requestId", requestId );
            reply->setProperty( "requestData", QVariant::fromValue< Tomahawk::InfoSystem::InfoRequestData >( requestData ) );

            connect( reply, SIGNAL( finished() ), SLOT( chartReturned() ) );
            return;
        }

        case InfoChartCapabilities:
        {

            QVariantMap result;
            foreach( QVariant chartResource, m_chartResources )
            {

                QList<Chart> album_charts;
                QList<Chart> track_charts;
                QVariantMap charts;

                if( !m_chartTypes.isEmpty() )
                    foreach( QVariant type, m_chartTypes )
                    {
                        // Itunes supplys charts based on geo, for now, only take US charts
                        if( type.toMap().value( "source" ).toString() == chartResource.toString()
                            && type.toMap().value( "geo" ).isValid()
                            && type.toMap().value( "geo" ).toString() != "us" )
                                continue;

                        if( type.toMap().value( "source" ).toString() == chartResource.toString() )
                        {
                            if( type.toMap().value( "type" ).toString() == "Album" )
                            {
                                album_charts.append( Chart(  type.toMap().value("id").toString(), type.toMap().value("name").toString(), "album" ) );
                                charts.insert( "Albums", QVariant::fromValue<QList<Chart> >( album_charts ) );
                            }
                            if( type.toMap().value( "type" ).toString() == "Track" )
                            {
                                track_charts.append( Chart( type.toMap().value("id").toString(), type.toMap().value("name").toString(), "tracks" ) );
                                charts.insert( "Tracks", QVariant::fromValue<QList<Chart> >( track_charts ) );
                            }
                        }
                    }

                result.insert( chartResource.toString() , QVariant::fromValue<QVariantMap>( charts ) );
            }
            emit info(
                requestId,
                requestData,
                result
            );
            return;
        }



        default:
        {
            tLog() << "Couldn't figure out what to do with this type of request after cache miss";
            emit info( requestId, requestData, QVariant() );
            return;
        }
    }
}


void
ChartsPlugin::chartResources()
{

    tDebug() << "ChartsPlugin: InfoChart resources returned!";
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

        m_chartResources = res.value( "chart_sources" ).toList();
        qDebug() << "Resources" << m_chartResources;

        if(!m_chartResources.isEmpty()){

            foreach(QVariant resource, m_chartResources){
                tDebug() << "ChartsPlugin: InfoChart fetching possible types for "<< resource.toString();

                QUrl url = QUrl( QString( CHART_URL "source/%1" ).arg(resource.toString() ) );
                qDebug() << "Getting types from " << url;

                QNetworkReply* reply = lastfm::nam()->get( QNetworkRequest( url ) );

                connect( reply, SIGNAL( finished() ), SLOT( chartTypes() ) );
            }
        }
    }

}

void
ChartsPlugin::chartTypes()
{

    tDebug() << "ChartsPlugin: InfoChart types returned!";
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

        foreach(QVariant chart, res.value( "charts" ).toMap() ){
            m_chartTypes.append(chart);
            qDebug() << "Chart types" << chart;
        }

    }

}

void
ChartsPlugin::chartReturned()
{

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
        }else qDebug() << res;


        QVariantList chartResponse = res.value( "list" ).toList();
        QList<ArtistTrackPair> top_tracks;
        QList<ArtistAlbumPair> top_albums;


        if( res.value( "type" ).toString() == "Album" )
            setChartType( Album );
        else if( res.value( "type" ).toString() == "Track" )
            setChartType( Track );
        else
            setChartType( None );


        foreach ( QVariant chartR, chartResponse )
        {
            QString title, artist, album;
            QVariantMap chartMap = chartR.toMap();

            if ( chartMap.contains( "track" ) )
            {

                title = chartMap.value( "track" ).toString();
                artist = chartMap.value( "artist" ).toString();

                if ( title.isEmpty() && artist.isEmpty() ) // don't have enough...
                {
                    tLog() << "Didn't get an artist and track name from itunes, not enough to build a query on. Aborting" << title << artist << album;

                }
                else{

                    if( chartType() == Album ){

                        ArtistAlbumPair pair;
                        pair.artist = artist;
                        pair.album = title;
                        top_albums << pair;

                    }else if( chartType() == Track ){

                        ArtistTrackPair pair;
                        pair.artist = artist;
                        pair.track = title;
                        top_tracks << pair;
                    }

                }
            }
        }
            if( chartType() == Track ){
                tDebug() << "ChartsPlugin:" << "\tgot " << top_tracks.size() << " tracks";
                returnedData["tracks"] = QVariant::fromValue( top_tracks );
                returnedData["type"] = "tracks";
            }

            if( chartType() == Album ){
                tDebug() << "ChartsPlugin:" << "\tgot " << top_albums.size() << " albums";
                returnedData["albums"] = QVariant::fromValue( top_albums );
                returnedData["type"] = "albums";
            }

        Tomahawk::InfoSystem::InfoRequestData requestData = reply->property( "requestData" ).value< Tomahawk::InfoSystem::InfoRequestData >();

        emit info(
            reply->property( "requestId" ).toUInt(),
            requestData,
            returnedData
        );
        // TODO update cache

    }else qDebug() << "Network error";

}

