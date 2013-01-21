/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2012, Leo Franchi            <lfranchi@kde.org>
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

#include "LastFmInfoPlugin.h"

#include <QDir>
#include <QSettings>
#include <QNetworkConfiguration>
#include <QDomElement>

#include "Album.h"
#include "Typedefs.h"
#include "audio/AudioEngine.h"
#include "utils/TomahawkUtils.h"
#include "utils/Logger.h"
#include "accounts/lastfm/LastFmAccount.h"
#include "Source.h"
#include "TomahawkSettings.h"

#include <lastfm/ws.h>
#include <lastfm/XmlQuery.h>

#include <qjson/parser.h>

using namespace Tomahawk::Accounts;
using namespace Tomahawk::InfoSystem;


LastFmInfoPlugin::LastFmInfoPlugin( LastFmAccount* account )
    : InfoPlugin()
    , m_account( account )
    , m_scrobbler( 0 )
{
    m_supportedGetTypes << InfoAlbumCoverArt << InfoArtistImages << InfoArtistSimilars << InfoArtistSongs << InfoArtistBiography << InfoChart << InfoChartCapabilities << InfoTrackSimilars;
    m_supportedPushTypes << InfoSubmitScrobble << InfoSubmitNowPlaying << InfoLove << InfoUnLove;
}


void
LastFmInfoPlugin::init()
{
    if ( Tomahawk::InfoSystem::InfoSystem::instance()->workerThread() && thread() != Tomahawk::InfoSystem::InfoSystem::instance()->workerThread().data() )
    {
        tDebug() << "Failure: move to the worker thread before running init";
        return;
    }

    lastfm::ws::ApiKey = "7194b85b6d1f424fe1668173a78c0c4a";
    lastfm::ws::SharedSecret = "ba80f1df6d27ae63e9cb1d33ccf2052f";
    lastfm::ws::Username = m_account.data()->username();
    lastfm::setNetworkAccessManager( TomahawkUtils::nam() );

    m_pw = m_account.data()->password();

    //HACK work around a bug in liblastfm---it doesn't create its config dir, so when it
    // tries to write the track cache, it fails silently. until we have a fixed version, do this
    // code taken from Amarok (src/services/lastfm/ScrobblerAdapter.cpp)
#ifdef Q_WS_X11
    QString lpath = QDir::home().filePath( ".local/share/Last.fm" );
    QDir ldir = QDir( lpath );
    if( !ldir.exists() )
    {
        ldir.mkpath( lpath );
    }
#endif

    m_badUrls << QUrl( "http://cdn.last.fm/flatness/catalogue/noimage" );

    QTimer::singleShot( 0, this, SLOT( settingsChanged() ) );
}


LastFmInfoPlugin::~LastFmInfoPlugin()
{
    qDebug() << Q_FUNC_INFO;
    delete m_scrobbler;
    m_scrobbler = 0;
}


void
LastFmInfoPlugin::dataError( Tomahawk::InfoSystem::InfoRequestData requestData )
{
    emit info( requestData, QVariant() );
    return;
}


void
LastFmInfoPlugin::getInfo( Tomahawk::InfoSystem::InfoRequestData requestData )
{
    switch ( requestData.type )
    {
        case InfoAlbumCoverArt:
            fetchAlbumInfo( requestData );
            break;

        case InfoArtistSimilars:
            fetchSimilarArtists( requestData );
            break;

        case InfoArtistSongs:
            fetchTopTracks( requestData );
            break;

        case InfoArtistImages:
        case InfoArtistBiography:
            fetchArtistInfo( requestData );
            break;

        case InfoChart:
            fetchChart( requestData );
            break;

        case InfoChartCapabilities:
            fetchChartCapabilities( requestData );
            break;

        case InfoTrackSimilars:
            fetchSimilarTracks( requestData );
            break;

        default:
            dataError( requestData );
    }
}


void
LastFmInfoPlugin::pushInfo( Tomahawk::InfoSystem::InfoPushData pushData )
{
    switch ( pushData.type )
    {
        case InfoSubmitNowPlaying:
            nowPlaying( pushData.infoPair.second );
            break;

        case InfoSubmitScrobble:
            scrobble();
            break;

        case InfoLove:
        case InfoUnLove:
            sendLoveSong( pushData.type, pushData.infoPair.second );
            break;

        default:
            return;
    }
}


void
LastFmInfoPlugin::nowPlaying( const QVariant &input )
{
    m_track = lastfm::MutableTrack();
    if ( !input.canConvert< QVariantMap >() )
    {
        tDebug() << Q_FUNC_INFO << "Failed to convert data to a QVariantMap";
        return;
    }

    QVariantMap map = input.toMap();
    if ( map.contains( "private" ) && map[ "private" ] == TomahawkSettings::FullyPrivate )
        return;

    if ( !map.contains( "trackinfo" ) || !map[ "trackinfo" ].canConvert< Tomahawk::InfoSystem::InfoStringHash >() || !m_scrobbler )
    {
        tLog() << Q_FUNC_INFO << "LastFmInfoPlugin::nowPlaying no m_scrobbler, or cannot convert input!";
        if ( !m_scrobbler )
            tLog() << Q_FUNC_INFO << "No scrobbler!";
        return;
    }

    Tomahawk::InfoSystem::InfoStringHash hash = map[ "trackinfo" ].value< Tomahawk::InfoSystem::InfoStringHash >();

    if ( !hash.contains( "title" ) || !hash.contains( "artist" ) || !hash.contains( "album" ) || !hash.contains( "duration" ) )
        return;

    m_track.stamp();

    m_track.setTitle( hash["title"] );
    m_track.setArtist( hash["artist"] );
    m_track.setAlbum( hash["album"] );
    bool ok;
    m_track.setDuration( hash["duration"].toUInt( &ok ) );
    m_track.setSource( lastfm::Track::Player );

    m_scrobbler->nowPlaying( m_track );
}


void
LastFmInfoPlugin::scrobble()
{
    if ( !m_scrobbler || m_track.isNull() )
        return;

    tLog() << Q_FUNC_INFO << "Scrobbling now:" << m_track.toString();

    // FIXME: workaround for the duration-less dilandau (and others) tracks
    if ( m_track.duration() == 0 )
        m_track.setDuration( 31 );

    m_scrobbler->cache( m_track );
    m_scrobbler->submit();
}


void
LastFmInfoPlugin::sendLoveSong( const InfoType type, QVariant input )
{
    qDebug() << Q_FUNC_INFO;

    if ( !input.toMap().contains( "trackinfo" ) || !input.toMap()[ "trackinfo" ].canConvert< Tomahawk::InfoSystem::InfoStringHash >() )
    {
        tLog() << "LastFmInfoPlugin::nowPlaying cannot convert input!";
        return;
    }

    InfoStringHash hash = input.toMap()[ "trackinfo" ].value< Tomahawk::InfoSystem::InfoStringHash >();
    if ( !hash.contains( "title" ) || !hash.contains( "artist" ) || !hash.contains( "album" ) )
        return;

    lastfm::MutableTrack track;
    track.stamp();

    track.setTitle( hash["title"] );
    track.setArtist( hash["artist"] );
    track.setAlbum( hash["album"] );
    bool ok;
    track.setDuration( hash["duration"].toUInt( &ok ) );
    track.setSource( lastfm::Track::Player );

    if ( type == Tomahawk::InfoSystem::InfoLove )
    {
        track.love();
    }
    else if ( type == Tomahawk::InfoSystem::InfoUnLove )
    {
        track.unlove();
    }
}


void
LastFmInfoPlugin::fetchSimilarArtists( Tomahawk::InfoSystem::InfoRequestData requestData )
{
    if ( !requestData.input.canConvert< Tomahawk::InfoSystem::InfoStringHash >() )
    {
        dataError( requestData );
        return;
    }
    InfoStringHash hash = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash >();
    if ( !hash.contains( "artist" ) )
    {
        dataError( requestData );
        return;
    }

    Tomahawk::InfoSystem::InfoStringHash criteria;
    criteria["artist"] = hash["artist"];

    emit getCachedInfo( criteria, 2419200000, requestData );
}


void
LastFmInfoPlugin::fetchSimilarTracks( Tomahawk::InfoSystem::InfoRequestData requestData )
{
    if ( !requestData.input.canConvert< Tomahawk::InfoSystem::InfoStringHash >() )
    {
        dataError( requestData );
        return;
    }
    InfoStringHash hash = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash >();
    if ( !hash.contains( "artist" ) || !hash.contains( "track" ) )
    {
        dataError( requestData );
        return;
    }

    Tomahawk::InfoSystem::InfoStringHash criteria;
    criteria["artist"] = hash["artist"];
    criteria["track"] = hash["track"];

    emit getCachedInfo( criteria, 2419200000, requestData );
}


void
LastFmInfoPlugin::fetchTopTracks( Tomahawk::InfoSystem::InfoRequestData requestData )
{
    if ( !requestData.input.canConvert< Tomahawk::InfoSystem::InfoStringHash >() )
    {
        dataError( requestData );
        return;
    }
    InfoStringHash hash = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash >();
    if ( !hash.contains( "artist" ) )
    {
        dataError( requestData );
        return;
    }

    Tomahawk::InfoSystem::InfoStringHash criteria;
    criteria["artist"] = hash["artist"];

    emit getCachedInfo( criteria, 2419200000, requestData );
}


void
LastFmInfoPlugin::fetchArtistInfo( Tomahawk::InfoSystem::InfoRequestData requestData )
{
    if ( !requestData.input.canConvert< Tomahawk::InfoSystem::InfoStringHash >() )
    {
        dataError( requestData );
        return;
    }
    InfoStringHash hash = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash >();
    if ( !hash.contains( "artist" ) )
    {
        dataError( requestData );
        return;
    }

    Tomahawk::InfoSystem::InfoStringHash criteria;
    criteria["artist"] = hash["artist"];

    emit getCachedInfo( criteria, 2419200000, requestData );
}


void
LastFmInfoPlugin::fetchChart( Tomahawk::InfoSystem::InfoRequestData requestData )
{
    if ( !requestData.input.canConvert< Tomahawk::InfoSystem::InfoStringHash >() )
    {
        dataError( requestData );
        return;
    }
    InfoStringHash hash = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash >();
    Tomahawk::InfoSystem::InfoStringHash criteria;
    if ( !hash.contains( "chart_id" ) )
    {
        dataError( requestData );
        return;
    } else {
        criteria["chart_id"] = hash["chart_id"];
    }

    emit getCachedInfo( criteria, 0, requestData );
}


void
LastFmInfoPlugin::fetchChartCapabilities( Tomahawk::InfoSystem::InfoRequestData requestData )
{
    if ( !requestData.input.canConvert< Tomahawk::InfoSystem::InfoStringHash >() )
    {
        dataError( requestData );
        return;
    }
    InfoStringHash hash = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash >();
    Tomahawk::InfoSystem::InfoStringHash criteria;

    emit getCachedInfo( criteria, 0, requestData );
}


void
LastFmInfoPlugin::fetchAlbumInfo( Tomahawk::InfoSystem::InfoRequestData requestData )
{
    if ( !requestData.input.canConvert< Tomahawk::InfoSystem::InfoStringHash >() )
    {
        dataError( requestData );
        return;
    }
    InfoStringHash hash = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash >();
    if ( !hash.contains( "artist" ) || !hash.contains( "album" ) )
    {
        dataError( requestData );
        return;
    }

    Tomahawk::InfoSystem::InfoStringHash criteria;
    criteria["artist"] = hash["artist"];
    criteria["album"] = hash["album"];

    emit getCachedInfo( criteria, 2419200000, requestData );
}


void
LastFmInfoPlugin::notInCacheSlot( QHash<QString, QString> criteria, Tomahawk::InfoSystem::InfoRequestData requestData )
{
    if ( !TomahawkUtils::nam() )
    {
        tLog() << "Have a null QNAM, uh oh";
        emit info( requestData, QVariant() );
        return;
    }

    InfoStringHash hash = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash >();
    switch ( requestData.type )
    {
        case InfoChart:
        {
             /// We need something to check if the request is actually ment to go to this plugin
            if ( !hash.contains( "chart_source" ) )
            {
                dataError( requestData );
                break;
            }
            else
            {
                if( "last.fm" != hash["chart_source"] )
                {
                    dataError( requestData );
                    break;
                }

            }
            QMap<QString, QString> args;
            args["method"] = criteria["chart_id"];
            args["limit"] = "100";
            QNetworkReply* reply = lastfm::ws::get(args);
            reply->setProperty( "requestData", QVariant::fromValue< Tomahawk::InfoSystem::InfoRequestData >( requestData ) );

            connect( reply, SIGNAL( finished() ), SLOT( chartReturned() ) );
            return;
        }

        case InfoChartCapabilities:
        {
            QList< InfoStringHash > track_charts;
            InfoStringHash c;
            c[ "type" ] = "tracks";
            c[ "id" ] = "chart.getTopTracks";
            c[ "label" ] = tr( "Top Tracks" );
            track_charts.append( c );
            c[ "id" ] = "chart.getLovedTracks";
            c[ "label" ] = tr( "Loved Tracks" );
            track_charts.append( c );
            c[ "id" ] = "chart.getHypedTracks";
            c[ "label" ] = tr( "Hyped Tracks" );
            track_charts.append( c );

            QList< InfoStringHash > artist_charts;
            c[ "type" ] = "artists";
            c[ "id" ] = "chart.getTopArtists";
            c[ "label" ] = tr( "Top Artists" );
            artist_charts.append( c );
            c[ "id" ] = "chart.getHypedArtists";
            c[ "label" ] = tr( "Hyped Artists" );
            artist_charts.append( c );

            QVariantMap charts;
            charts.insert( "Tracks", QVariant::fromValue< QList< InfoStringHash > >( track_charts ) );
            charts.insert( "Artists", QVariant::fromValue< QList< InfoStringHash > >( artist_charts ) );

            QVariantMap result;
            result.insert( "Last.fm", QVariant::fromValue<QVariantMap>( charts ) );

            emit info( requestData, result );
            return;
        }

        case InfoArtistSimilars:
        {
            lastfm::Artist a( criteria["artist"] );
            QNetworkReply* reply = a.getSimilar();
            reply->setProperty( "requestData", QVariant::fromValue< Tomahawk::InfoSystem::InfoRequestData >( requestData ) );

            connect( reply, SIGNAL( finished() ), SLOT( similarArtistsReturned() ) );
            return;
        }

        case InfoTrackSimilars:
        {
            lastfm::MutableTrack t;
            t.setArtist( criteria["artist"] );
            t.setTitle( criteria["track"] );

            QNetworkReply* reply = t.getSimilar();
            reply->setProperty( "requestData", QVariant::fromValue< Tomahawk::InfoSystem::InfoRequestData >( requestData ) );

            connect( reply, SIGNAL( finished() ), SLOT( similarTracksReturned() ) );
            return;
        }

        case InfoArtistSongs:
        {
            lastfm::Artist a( criteria["artist"] );
            QNetworkReply* reply = a.getTopTracks();
            reply->setProperty( "requestData", QVariant::fromValue< Tomahawk::InfoSystem::InfoRequestData >( requestData ) );

            connect( reply, SIGNAL( finished() ), SLOT( topTracksReturned() ) );
            return;
        }

        case InfoArtistBiography:
        case InfoArtistImages:
        {
            lastfm::Artist a( criteria["artist"] );
            QNetworkReply* reply = a.getInfo();
            reply->setProperty( "requestData", QVariant::fromValue< Tomahawk::InfoSystem::InfoRequestData >( requestData ) );

            connect( reply, SIGNAL( finished() ), SLOT( artistInfoReturned() ) );
            return;
        }

        case InfoAlbumCoverArt:
        {
            lastfm::Album a( criteria["artist"], criteria["album"] );
            QNetworkReply* reply = a.getInfo();
            reply->setProperty( "requestData", QVariant::fromValue< Tomahawk::InfoSystem::InfoRequestData >( requestData ) );

            connect( reply, SIGNAL( finished() ), SLOT( albumInfoReturned() ) );
            return;
        }

        default:
        {
            tLog() << "Couldn't figure out what to do with this type of request after cache miss";
            emit info( requestData, QVariant() );
            return;
        }
    }
}


void
LastFmInfoPlugin::similarArtistsReturned()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>( sender() );

    QMap< int, QString > similarArtists = lastfm::Artist::getSimilar( reply );

    QStringList sortedArtists;
    QStringList sortedScores;
    QStringList al;
    QStringList sl;

    foreach ( const QString& artist, similarArtists.values() )
        al << artist;
    foreach ( int score, similarArtists.keys() )
        sl << QString::number( score );

    for ( int i = al.count() - 1; i >= 0; i-- )
    {
        sortedArtists << al.at( i );
        sortedScores << sl.at( i );
    }

    QVariantMap returnedData;
    returnedData["artists"] = sortedArtists;
    returnedData["score"] = sortedScores;

    Tomahawk::InfoSystem::InfoRequestData requestData = reply->property( "requestData" ).value< Tomahawk::InfoSystem::InfoRequestData >();

    emit info( requestData, returnedData );

    if ( !sortedArtists.isEmpty() )
    {
        Tomahawk::InfoSystem::InfoStringHash origData = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash>();
        Tomahawk::InfoSystem::InfoStringHash criteria;
        criteria["artist"] = origData["artist"];
        emit updateCache( criteria, 2419200000, requestData.type, returnedData );
    }
}


void
LastFmInfoPlugin::similarTracksReturned()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>( sender() );

    QMap< int, QPair< QString, QString > > similarTracks = lastfm::Track::getSimilar( reply );

    QStringList sortedArtists;
    QStringList sortedTracks;
    QStringList sortedScores;
    QStringList al;
    QStringList tl;
    QStringList sl;

    QPair< QString, QString > track;
    foreach ( track, similarTracks.values() )
    {
        tl << track.first;
        al << track.second;
    }
    foreach ( int score, similarTracks.keys() )
        sl << QString::number( score );

    for ( int i = tl.count() - 1; i >= 0; i-- )
    {
        sortedTracks << tl.at( i );
        sortedArtists << al.at( i );
        sortedScores << sl.at( i );
    }

    QVariantMap returnedData;
    returnedData["tracks"] = sortedTracks;
    returnedData["artists"] = sortedArtists;
    returnedData["score"] = sortedScores;

    tDebug( LOGVERBOSE ) << "Returning data, tracks:" << sortedTracks << "artists:" << sortedArtists << "scores:" << sortedScores;

    Tomahawk::InfoSystem::InfoRequestData requestData = reply->property( "requestData" ).value< Tomahawk::InfoSystem::InfoRequestData >();

    emit info( requestData, returnedData );

    if ( !sortedTracks.isEmpty() )
    {
        Tomahawk::InfoSystem::InfoStringHash origData = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash>();
        Tomahawk::InfoSystem::InfoStringHash criteria;
        criteria["artist"] = origData["artist"];
        criteria["track"] = origData["track"];
        emit updateCache( criteria, 2419200000, requestData.type, returnedData );
    }
}


void
LastFmInfoPlugin::chartReturned()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>( sender() );

    QVariantMap returnedData;
    const QRegExp tracks_rx( "chart\\.\\S+tracks\\S*", Qt::CaseInsensitive );
    const QRegExp artists_rx( "chart\\.\\S+artists\\S*", Qt::CaseInsensitive );
    const QString url = reply->url().toString();

    if ( url.contains( tracks_rx ) )
    {
        QList<lastfm::Track> tracks = parseTrackList( reply );
        QList<InfoStringHash> top_tracks;
        foreach( const lastfm::Track& t, tracks )
        {
            InfoStringHash pair;
            pair[ "artist" ] = t.artist().toString();
            pair[ "track" ] = t.title();
            top_tracks << pair;
        }
        returnedData["tracks"] = QVariant::fromValue( top_tracks );
        returnedData["type"] = "tracks";

    }
    else if ( url.contains( artists_rx ) )
    {
        QList<lastfm::Artist> list = lastfm::Artist::list( reply );
        QStringList al;
        foreach ( const lastfm::Artist& a, list )
            al << a.toString();
        returnedData["artists"] = al;
        returnedData["type"] = "artists";
    }
    else
    {
        tDebug() << Q_FUNC_INFO << "got non tracks and non artists";
    }

    Tomahawk::InfoSystem::InfoRequestData requestData = reply->property( "requestData" ).value< Tomahawk::InfoSystem::InfoRequestData >();

    emit info( requestData, returnedData );
    // TODO update cache
}


void
LastFmInfoPlugin::topTracksReturned()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>( sender() );

    QStringList topTracks = lastfm::Artist::getTopTracks( reply );
    topTracks.removeDuplicates();

    QVariantMap returnedData;
    returnedData["tracks"] = topTracks;

    Tomahawk::InfoSystem::InfoRequestData requestData = reply->property( "requestData" ).value< Tomahawk::InfoSystem::InfoRequestData >();

    emit info( requestData, returnedData );

    Tomahawk::InfoSystem::InfoStringHash origData = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash>();
    Tomahawk::InfoSystem::InfoStringHash criteria;
    criteria["artist"] = origData["artist"];
    emit updateCache( criteria, 0, requestData.type, returnedData );
}


void
LastFmInfoPlugin::artistInfoReturned()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>( sender() );
    Tomahawk::InfoSystem::InfoRequestData requestData = reply->property( "requestData" ).value< Tomahawk::InfoSystem::InfoRequestData >();

    if ( requestData.type == Tomahawk::InfoSystem::InfoArtistBiography )
    {
        QVariantMap returnedData;
        lastfm::XmlQuery lfm;
        if ( lfm.parse( reply->readAll() ) )
        {
            QRegExp tagRegExp( "<a href=\"http://www.last.fm/tag/([^\"]*)\" class=\"bbcode_tag\" rel=\"tag\">" );
            QRegExp artistRegExp( "<a href=\"http://www.last.fm/music/([^\"]*)\" class=\"bbcode_artist\">" );
            QRegExp albumRegExp( "<a title=\"([^\"]*)\" href=\"http://www.last.fm/music/([^\"]*)/([^\"]*)\" class=\"bbcode_album\">" );
            QRegExp trackRegExp( "<a title=\"([^\"]*)\" href=\"http://www.last.fm/music/([^\"]*)/([^\"]*)/([^\"]*)\" class=\"bbcode_track\">" );

            tagRegExp.setMinimal( true );
            artistRegExp.setMinimal( true );
            albumRegExp.setMinimal( true );
            trackRegExp.setMinimal( true );

            QString biography = lfm["artist"]["bio"]["content"].text().trimmed().replace( "User-contributed text is available under the Creative Commons By-SA License and may also be available under the GNU FDL.", "" );
            biography = biography.replace( tagRegExp, "<a href=\"tomahawk://view/tag?name=\\1\">" )
                                 .replace( artistRegExp, "<a href=\"tomahawk://view/artist?name=\\1\">" )
                                 .replace( albumRegExp, "<a href=\"tomahawk://view/album?artist=\\2&name=\\3\">" )
                                 .replace( trackRegExp, "<a href=\"tomahawk://view/track?artist=\\2&album=\\3&name=\\4\">" )
                                 .replace( "&album=_", "" );

            QVariantHash siteData;
            siteData[ "site" ] = "last.fm";
            siteData[ "text" ] = biography.replace( "\r", "\n" ).replace( "\n\n", "\n" );
            siteData[ "summary" ] = lfm["artist"]["bio"]["summary"].text().trimmed().replace( "\r", "\n" ).replace( "\n\n", "\n" );
            returnedData[ "last.fm" ] = siteData;

            Tomahawk::InfoSystem::InfoStringHash origData = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash>();
            Tomahawk::InfoSystem::InfoStringHash criteria;
            criteria["artist"] = origData["artist"];
            emit updateCache( criteria, 0, requestData.type, returnedData );
        }
        emit info( requestData, returnedData );
    }
    else if ( requestData.type == Tomahawk::InfoSystem::InfoArtistImages )
    {
        lastfm::Artist artist = lastfm::Artist::getInfo( reply );

        QUrl imgurl = artist.imageUrl( lastfm::AbstractType::ExtraLargeImage );
        if ( !imgurl.isValid() )
            imgurl = artist.imageUrl( lastfm::AbstractType::MegaImage );
        if ( !imgurl.isValid() )
            imgurl = artist.imageUrl( lastfm::AbstractType::LargeImage );

        QNetworkRequest req( imgurl );
        QNetworkReply* newReply = TomahawkUtils::nam()->get( req );
        newReply->setProperty( "requestData", reply->property( "requestData" ) );
        connect( newReply, SIGNAL( finished() ), SLOT( coverArtReturned() ) );
    }
}


void
LastFmInfoPlugin::albumInfoReturned()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>( sender() );
    Tomahawk::InfoSystem::InfoRequestData requestData = reply->property( "requestData" ).value< Tomahawk::InfoSystem::InfoRequestData >();

    if ( requestData.type == Tomahawk::InfoSystem::InfoAlbumCoverArt )
    {
        lastfm::XmlQuery lfm;
        if ( lfm.parse( reply->readAll() ) )
        {
            QUrl imgurl = QUrl( lfm["album"]["image size=extralarge"].text() );
            if ( !imgurl.isValid() )
                imgurl = QUrl( lfm["album"]["image size=mega"].text() );
            if ( !imgurl.isValid() )
                imgurl = QUrl( lfm["album"]["image size=large"].text() );

            QNetworkRequest req( imgurl );
            QNetworkReply* newReply = TomahawkUtils::nam()->get( req );
            newReply->setProperty( "requestData", reply->property( "requestData" ) );
            connect( newReply, SIGNAL( finished() ), SLOT( coverArtReturned() ) );
        }
    }
}


void
LastFmInfoPlugin::coverArtReturned()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>( sender() );
    QUrl redir = reply->attribute( QNetworkRequest::RedirectionTargetAttribute ).toUrl();
    if ( redir.isEmpty() )
    {
        Tomahawk::InfoSystem::InfoRequestData requestData = reply->property( "requestData" ).value< Tomahawk::InfoSystem::InfoRequestData >();
        Tomahawk::InfoSystem::InfoStringHash origData = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash>();

        QByteArray ba = reply->readAll();
        if ( ba.isNull() || !ba.length() )
        {
            tLog() << Q_FUNC_INFO << "Null byte array for cover of" << origData["artist"] << origData["album"];
            emit info( requestData, QVariant() );
            return;
        }

        foreach ( const QUrl& url, m_badUrls )
        {
            if ( reply->url().toString().startsWith( url.toString() ) )
                ba = QByteArray();
        }

        QVariantMap returnedData;
        returnedData["imgbytes"] = ba;
        returnedData["url"] = reply->url().toString();

        emit info( requestData, returnedData );

        Tomahawk::InfoSystem::InfoStringHash criteria;
        criteria["artist"] = origData["artist"];
        criteria["album"] = origData["album"];
        emit updateCache( criteria, 2419200000, requestData.type, returnedData );
    }
    else
    {
        // Follow HTTP redirect
        QNetworkRequest req( redir );
        QNetworkReply* newReply = TomahawkUtils::nam()->get( req );
        newReply->setProperty( "requestData", reply->property( "requestData" ) );
        connect( newReply, SIGNAL( finished() ), SLOT( coverArtReturned() ) );
    }

    reply->deleteLater();
}


void
LastFmInfoPlugin::artistImagesReturned()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>( sender() );
    QUrl redir = reply->attribute( QNetworkRequest::RedirectionTargetAttribute ).toUrl();
    if ( redir.isEmpty() )
    {
        QByteArray ba = reply->readAll();
        if ( ba.isNull() || !ba.length() )
        {
            tLog() << Q_FUNC_INFO << "Uh oh, null byte array";
            emit info( reply->property( "requestData" ).value< Tomahawk::InfoSystem::InfoRequestData >(), QVariant() );
            return;
        }
        foreach ( const QUrl& url, m_badUrls )
        {
            if ( reply->url().toString().startsWith( url.toString() ) )
                ba = QByteArray();
        }

        QVariantMap returnedData;
        returnedData["imgbytes"] = ba;
        returnedData["url"] = reply->url().toString();

        Tomahawk::InfoSystem::InfoRequestData requestData = reply->property( "requestData" ).value< Tomahawk::InfoSystem::InfoRequestData >();

        emit info( requestData, returnedData );

        Tomahawk::InfoSystem::InfoStringHash origData = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash>();
        Tomahawk::InfoSystem::InfoStringHash criteria;
        criteria["artist"] = origData["artist"];
        emit updateCache( criteria, 2419200000, requestData.type, returnedData );
    }
    else
    {
        if ( !TomahawkUtils::nam() )
        {
            tLog() << Q_FUNC_INFO << "Uh oh, nam is null";
            emit info( reply->property( "requestData" ).value< Tomahawk::InfoSystem::InfoRequestData >(), QVariant() );
            return;
        }
        // Follow HTTP redirect
        QNetworkRequest req( redir );
        QNetworkReply* newReply = TomahawkUtils::nam()->get( req );
        newReply->setProperty( "requestData", reply->property( "requestData" ) );
        connect( newReply, SIGNAL( finished() ), SLOT( artistImagesReturned() ) );
    }

    reply->deleteLater();
}


void
LastFmInfoPlugin::settingsChanged()
{
    if ( m_account.isNull() )
        return;

    if ( !m_scrobbler && m_account.data()->scrobble() )
    { // can simply create the scrobbler
        lastfm::ws::Username = m_account.data()->username();
        m_pw = m_account.data()->password();

        createScrobbler();
    }
    else if ( m_scrobbler && !m_account.data()->scrobble() )
    {
        delete m_scrobbler;
        m_scrobbler = 0;
    }
    else if ( m_account.data()->username() != lastfm::ws::Username ||
        m_account.data()->password() != m_pw )
    {
        qDebug() << "Last.fm credentials changed, re-creating scrobbler";
        lastfm::ws::Username = m_account.data()->username();
        m_pw = m_account.data()->password();
        // credentials have changed, have to re-create scrobbler for them to take effect
        if ( m_scrobbler )
        {
            delete m_scrobbler;
            m_scrobbler = 0;
        }

        m_account.data()->setSessionKey( QString() );
        createScrobbler();
    }
}


void
LastFmInfoPlugin::onAuthenticated()
{
    QNetworkReply* authJob = dynamic_cast<QNetworkReply*>( sender() );
    if ( !authJob || m_account.isNull() )
    {
        tLog() << Q_FUNC_INFO << "Help! No longer got a last.fm auth job!";
        return;
    }

    lastfm::XmlQuery lfm;
    lfm.parse( authJob->readAll() );
    if ( authJob->error() == QNetworkReply::NoError && lfm.attribute("status") == "ok" )
    {
        lastfm::ws::SessionKey = lfm[ "session" ][ "key" ].text();
        m_account.data()->setSessionKey( lastfm::ws::SessionKey.toLatin1() );

        if ( m_account.data()->scrobble() )
            m_scrobbler = new lastfm::Audioscrobbler( "thk" );
    }
    else
    {
        m_account.data()->setSessionKey( QByteArray() );

        QString error = "Got error in Last.fm authentication job";
        if ( lfm.children( "error" ).size() > 0 )
            error += ": " + lfm.text();
        else if ( authJob->error() != QNetworkReply::NoError )
            error += ": " + authJob->errorString();
        else
            error += ".";

        tLog() << error.simplified();
    }

    authJob->deleteLater();
}


void
LastFmInfoPlugin::createScrobbler()
{
    if ( m_account.isNull() || lastfm::ws::Username.isEmpty() )
        return;

    if ( m_account.data()->sessionKey().isEmpty() ) // no session key, so get one
    {
        qDebug() << "LastFmInfoPlugin::createScrobbler Session key is empty";
        QString authToken = TomahawkUtils::md5( ( lastfm::ws::Username.toLower() + TomahawkUtils::md5( m_pw.toUtf8() ) ).toUtf8() );

        QMap<QString, QString> query;
        query[ "method" ] = "auth.getMobileSession";
        query[ "username" ] = lastfm::ws::Username;
        query[ "authToken" ] = authToken;
        QNetworkReply* authJob = lastfm::ws::post( query );

        connect( authJob, SIGNAL( finished() ), SLOT( onAuthenticated() ) );
    }
    else
    {
        qDebug() << "LastFmInfoPlugin::createScrobbler Already have session key";
        lastfm::ws::SessionKey = m_account.data()->sessionKey();

        m_scrobbler = new lastfm::Audioscrobbler( "thk" );
    }
}


QList<lastfm::Track>
LastFmInfoPlugin::parseTrackList( QNetworkReply* reply )
{
    QList<lastfm::Track> tracks;
    try
    {
        lastfm::XmlQuery lfm;
        lfm.parse( reply->readAll() );
        foreach ( lastfm::XmlQuery xq, lfm.children( "track" ) )
        {
            tracks.append( lastfm::Track( xq ) );
        }
    }
    catch ( lastfm::ws::ParseError& e )
    {
        qWarning() << e.message();
    }

    return tracks;
}
