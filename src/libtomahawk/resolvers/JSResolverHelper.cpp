/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2013,      Teo Mrnjavac <teo@kde.org>
 *   Copyright 2013,      Uwe L. Korn <uwelk@xhochy.com>
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

#include "JSResolverHelper.h"

#include "database/Database.h"
#include "database/DatabaseImpl.h"
#include "playlist/PlaylistTemplate.h"
#include "playlist/XspfPlaylistTemplate.h"
#include "resolvers/ScriptEngine.h"
#include "network/Servent.h"
#include "utils/Closure.h"
#include "utils/Cloudstream.h"
#include "utils/Json.h"
#include "utils/NetworkAccessManager.h"
#include "utils/NetworkReply.h"
#include "utils/Logger.h"

#include "config.h"
#include "JSResolver_p.h"
#include "Pipeline.h"
#include "Result.h"
#include "SourceList.h"
#include "UrlHandler.h"
#include "JSPlugin.h"

#include <QFile>
#include <QFileInfo>
#include <QMap>
#include <QWebFrame>
#include <taglib/asffile.h>
#include <taglib/flacfile.h>
#include <taglib/id3v2framefactory.h>
#include <taglib/mp4file.h>
#include <taglib/mpegfile.h>
#include <taglib/oggfile.h>
#include <taglib/vorbisfile.h>

#if defined(TAGLIB_MAJOR_VERSION) && defined(TAGLIB_MINOR_VERSION)
#if TAGLIB_MAJOR_VERSION >= 1 && TAGLIB_MINOR_VERSION >= 9
    #include <taglib/opusfile.h>
#endif
#endif

using namespace Tomahawk;

JSResolverHelper::JSResolverHelper( const QString& scriptPath, JSResolver* parent )
    : QObject( parent )
    , m_resolver( parent )
    , m_scriptPath( scriptPath )
    , m_urlCallbackIsAsync( false )
{
}


QByteArray
JSResolverHelper::readRaw( const QString& fileName )
{
    QString path = QFileInfo( m_scriptPath ).absolutePath();
    // remove directories
    QString cleanedFileName = QFileInfo( fileName ).fileName();
    QString absoluteFilePath = path.append( "/" ).append( cleanedFileName );

    QFile file( absoluteFilePath );
    if ( !file.exists() )
    {
        Q_ASSERT(false);
        return QByteArray();
    }

    file.open( QIODevice::ReadOnly );
    return file.readAll();
}


QString
JSResolverHelper::compress( const QString& data )
{
    QByteArray comp = qCompress( data.toLatin1(), 9 );
    return comp.toBase64();
}


QString
JSResolverHelper::readCompressed( const QString& fileName )
{
    return compress( readRaw( fileName ) );
}


QString
JSResolverHelper::readBase64( const QString& fileName )
{
    return readRaw( fileName ).toBase64();
}


QVariantMap
JSResolverHelper::resolverData()
{
    QVariantMap resolver;
    resolver["config"] = m_resolverConfig;
    resolver["scriptPath"] = m_scriptPath;
    return resolver;
}


void
JSResolverHelper::log( const QString& message )
{
    tLog() << "JAVASCRIPT:" << m_scriptPath << ":" << message;
}


void
JSResolverHelper::addTrackResults( const QVariantMap& results )
{
    qDebug() << "Resolver reporting results:" << results;
    QList< Tomahawk::result_ptr > tracks = m_resolver->parseResultVariantList( results.value("results").toList() );

    QString qid = results.value("qid").toString();

    Tomahawk::Pipeline::instance()->reportResults( qid, tracks );
}


void
JSResolverHelper::addArtistResults( const QVariantMap& results )
{
    qDebug() << "Resolver reporting artists:" << results;
    QList< Tomahawk::artist_ptr > artists = m_resolver->parseArtistVariantList( results.value( "artists" ).toList() );

    QString qid = results.value("qid").toString();

    Tomahawk::collection_ptr collection = Tomahawk::collection_ptr();
    foreach ( const Tomahawk::collection_ptr& coll, m_resolver->collections() )
    {
        if ( coll->name() == qid )
        {
            collection = coll;
        }
    }
    if ( collection.isNull() )
        return;

    tDebug() << Q_FUNC_INFO << "about to push" << artists.count() << "artists";
    foreach( const Tomahawk::artist_ptr& artist, artists)
        tDebug() << artist->name();

    emit m_resolver->artistsFound( artists );
}


void
JSResolverHelper::addAlbumResults( const QVariantMap& results )
{
    qDebug() << "Resolver reporting albums:" << results;
    QString artistName = results.value( "artist" ).toString();
    if ( artistName.trimmed().isEmpty() )
        return;
    Tomahawk::artist_ptr artist = Tomahawk::Artist::get( artistName, false );
    QList< Tomahawk::album_ptr > albums = m_resolver->parseAlbumVariantList( artist, results.value( "albums" ).toList() );

    QString qid = results.value("qid").toString();

    Tomahawk::collection_ptr collection = Tomahawk::collection_ptr();
    foreach ( const Tomahawk::collection_ptr& coll, m_resolver->collections() )
    {
        if ( coll->name() == qid )
        {
            collection = coll;
        }
    }
    if ( collection.isNull() )
        return;

    tDebug() << Q_FUNC_INFO << "about to push" << albums.count() << "albums";
    foreach( const Tomahawk::album_ptr& album, albums)
        tDebug() << album->name();

    emit m_resolver->albumsFound( albums );
}


void
JSResolverHelper::addAlbumTrackResults( const QVariantMap& results )
{
    qDebug() << "Resolver reporting album tracks:" << results;
    QString artistName = results.value( "artist" ).toString();
    if ( artistName.trimmed().isEmpty() )
        return;
    QString albumName = results.value( "album" ).toString();
    if ( albumName.trimmed().isEmpty() )
        return;

    Tomahawk::artist_ptr artist = Tomahawk::Artist::get( artistName, false );
    Tomahawk::album_ptr  album  = Tomahawk::Album::get( artist, albumName, false );

    QList< Tomahawk::result_ptr > tracks = m_resolver->parseResultVariantList( results.value("results").toList() );

    QString qid = results.value("qid").toString();

    Tomahawk::collection_ptr collection = Tomahawk::collection_ptr();
    foreach ( const Tomahawk::collection_ptr& coll, m_resolver->collections() )
    {
        if ( coll->name() == qid )
        {
            collection = coll;
        }
    }
    if ( collection.isNull() )
        return;

    QList< Tomahawk::query_ptr > queries;
    foreach ( const Tomahawk::result_ptr& result, tracks )
    {
        result->setScore( 1.0 );
        queries.append( result->toQuery() );
    }

    tDebug() << Q_FUNC_INFO << "about to push" << tracks.count() << "tracks";

    emit m_resolver->tracksFound( queries );
}


query_ptr
JSResolverHelper::parseTrack( const QVariantMap& track )
{
    QString title = track.value( "title" ).toString();
    QString artist = track.value( "artist" ).toString();
    QString album = track.value( "album" ).toString();
    if ( title.isEmpty() || artist.isEmpty() )
    {
        return query_ptr();
    }

    Tomahawk::query_ptr query = Tomahawk::Query::get( artist, title, album );
    QString resultHint = track.value( "hint" ).toString();
    if ( !resultHint.isEmpty() )
    {
        query->setResultHint( resultHint );
        query->setSaveHTTPResultHint( true );
    }

    return query;
}


QString
JSResolverHelper::instanceUUID()
{
    return Tomahawk::Database::instance()->impl()->dbid();
}


void
JSResolverHelper::addUrlResult( const QString& url, const QVariantMap& result )
{
    // It may seem a bit weird, but currently no slot should do anything
    // more as we starting on a new URL and not task are waiting for it yet.
    m_pendingUrl = QString();
    m_pendingAlbum = album_ptr();

    QString type = result.value( "type" ).toString();
    if ( type == "artist" )
    {
        QString name = result.value( "name" ).toString();
        Q_ASSERT( !name.isEmpty() );
        emit m_resolver->informationFound( url, Artist::get( name, true ).objectCast<QObject>() );
    }
    else if ( type == "album" )
    {
        QString name = result.value( "name" ).toString();
        QString artist = result.value( "artist" ).toString();
        album_ptr album = Album::get( Artist::get( artist, true ), name );
        m_pendingUrl = url;
        m_pendingAlbum = album;
        connect( album.data(), SIGNAL( tracksAdded( QList<Tomahawk::query_ptr>, Tomahawk::ModelMode, Tomahawk::collection_ptr ) ),
                 SLOT( tracksAdded( QList<Tomahawk::query_ptr>, Tomahawk::ModelMode, Tomahawk::collection_ptr ) ) );
        if ( album->tracks().count() > 0 )
        {
            emit m_resolver->informationFound( url, album.objectCast<QObject>() );
        }
    }
    else if ( type == "track" )
    {
        Tomahawk::query_ptr query = parseTrack( result );
        if ( query.isNull() )
        {
            // A valid track result shoud have non-empty title and artist.
            tLog() << Q_FUNC_INFO << m_resolver->name() << "Got empty track information for " << url;
            emit m_resolver->informationFound( url, QSharedPointer<QObject>() );
        }
        else
        {
            emit m_resolver->informationFound( url, query.objectCast<QObject>() );
        }
    }
    else if ( type == "playlist" )
    {
        QString guid = result.value( "guid" ).toString();
        Q_ASSERT( !guid.isEmpty() );
        // Append nodeid to guid to make it globally unique.
        guid += instanceUUID();

        // Do we already have this playlist loaded?
        {
            playlist_ptr playlist = Playlist::get( guid );
            if ( !playlist.isNull() )
            {
                emit m_resolver->informationFound( url, playlist.objectCast<QObject>() );
                return;
            }
        }

        // Get all information to build a new playlist but do not build it until we know,
        // if it is really handled as a playlist and not as a set of tracks.
        Tomahawk::source_ptr source = SourceList::instance()->getLocal();
        const QString title = result.value( "title" ).toString();
        const QString info = result.value( "info" ).toString();
        const QString creator = result.value( "creator" ).toString();
        QList<query_ptr> queries;
        foreach( QVariant track, result.value( "tracks" ).toList() )
        {
            query_ptr query = parseTrack( track.toMap() );
            if ( !query.isNull() )
            {
                queries << query;
            }
        }
        tLog( LOGVERBOSE ) << Q_FUNC_INFO << m_resolver->name() << "Got playlist for " << url;
        playlisttemplate_ptr pltemplate( new PlaylistTemplate( source, guid, title, info, creator, false, queries ) );
        emit m_resolver->informationFound( url, pltemplate.objectCast<QObject>() );
    }
    else if ( type == "xspf-url" )
    {
        QString xspfUrl = result.value( "url" ).toString();
        Q_ASSERT( !xspfUrl.isEmpty() );
        QString guid = QString( "xspf-%1-%2" ).arg( xspfUrl.toUtf8().toBase64().constData() ).arg( instanceUUID() );

        // Do we already have this playlist loaded?
        {
            playlist_ptr playlist = Playlist::get( guid );
            if ( !playlist.isNull() )
            {
                emit m_resolver->informationFound( url, playlist.objectCast<QObject>() );
                return;
            }
        }


        // Get all information to build a new playlist but do not build it until we know,
        // if it is really handled as a playlist and not as a set of tracks.
        Tomahawk::source_ptr source = SourceList::instance()->getLocal();
        QSharedPointer<XspfPlaylistTemplate> pltemplate( new XspfPlaylistTemplate( xspfUrl, source, guid ) );
        NewClosure( pltemplate, SIGNAL( tracksLoaded( QList< Tomahawk::query_ptr > ) ),
                    this, SLOT( pltemplateTracksLoadedForUrl( QString, Tomahawk::playlisttemplate_ptr ) ),
                    url, pltemplate.objectCast<Tomahawk::PlaylistTemplate>() );
        tLog( LOGVERBOSE ) << Q_FUNC_INFO << m_resolver->name() << "Got playlist for " << url;
        pltemplate->load();
    }
    else
    {
        tLog( LOGVERBOSE ) << Q_FUNC_INFO << m_resolver->name() << "No usable information found for " << url;
        emit m_resolver->informationFound( url, QSharedPointer<QObject>() );
    }
}


void
JSResolverHelper::reportCapabilities( const QVariant& v )
{
    bool ok;
    int intCap = v.toInt( &ok );
    Tomahawk::ExternalResolver::Capabilities capabilities;
    if ( !ok )
        capabilities = Tomahawk::ExternalResolver::NullCapability;
    else
        capabilities = static_cast< Tomahawk::ExternalResolver::Capabilities >( intCap );

    m_resolver->onCapabilitiesChanged( capabilities );
}


void
JSResolverHelper::reportScriptJobResults( const QVariantMap& result )
{
    m_resolver->d_func()->scriptPlugin->reportScriptJobResult( result );
}


void
JSResolverHelper::registerScriptPlugin(const QString& type, const QString& objectId)
{
    m_resolver->d_func()->scriptPlugin->registerScriptPlugin( type, objectId );
}


void
JSResolverHelper::tracksAdded( const QList<query_ptr>&, const ModelMode, const collection_ptr&)
{
    // Check if we still are actively waiting
    if ( m_pendingAlbum.isNull() || m_pendingUrl.isNull() )
        return;

    emit m_resolver->informationFound( m_pendingUrl, m_pendingAlbum.objectCast<QObject>() );
    m_pendingAlbum = album_ptr();
    m_pendingUrl = QString();
}


void
JSResolverHelper::pltemplateTracksLoadedForUrl( const QString& url, const playlisttemplate_ptr& pltemplate )
{
    tLog() << Q_FUNC_INFO;
    emit m_resolver->informationFound( url, pltemplate.objectCast<QObject>() );
}


void
JSResolverHelper::setResolverConfig( const QVariantMap& config )
{
    m_resolverConfig = config;
}


QString
JSResolverHelper::accountId()
{
    return m_resolver->d_func()->accountId;
}


void
JSResolverHelper::addCustomUrlHandler( const QString& protocol,
                                             const QString& callbackFuncName,
                                             const QString& isAsynchronous )
{
    m_urlCallbackIsAsync = ( isAsynchronous.toLower() == "true" );

    std::function< void( const Tomahawk::result_ptr&, const QString&,
                           std::function< void( const QString&, QSharedPointer< QIODevice >& ) > )> fac =
            std::bind( &JSResolverHelper::customIODeviceFactory, this,
                       std::placeholders::_1, std::placeholders::_2,
                       std::placeholders::_3 );
    Tomahawk::UrlHandler::registerIODeviceFactory( protocol, fac );

    m_urlCallback = callbackFuncName;
}


void
JSResolverHelper::reportStreamUrl( const QString& qid, const QString& streamUrl )
{
    reportStreamUrl( qid, streamUrl, QVariantMap() );
}


void JSResolverHelper::nativeAssert(bool assertion, const QString& message)
{
    if ( !assertion )
    {
        tLog() << "Assertion failed" << message;
        Q_ASSERT( assertion );
    }
}



void
JSResolverHelper::customIODeviceFactory( const Tomahawk::result_ptr&, const QString& url,
                                               std::function< void( const QString&, QSharedPointer< QIODevice >& ) > callback )
{
    //can be sync or async
    if ( m_urlCallbackIsAsync )
    {
        QString qid = uuid();
        QString getUrl = QString( "Tomahawk.resolver.instance.%1( '%2', '%3' );" ).arg( m_urlCallback )
                                                                                  .arg( qid )
                                                                                  .arg( url );

        m_streamCallbacks.insert( qid, callback );
        m_resolver->d_func()->scriptPlugin->evaluateJavaScript( getUrl );
    }
    else
    {
        QString getUrl = QString( "Tomahawk.resolver.instance.%1( '%2' );" ).arg( m_urlCallback )
                                                                            .arg( url );

        QString urlStr = m_resolver->d_func()->scriptPlugin->evaluateJavaScriptWithResult( getUrl ).toString();

        returnStreamUrl( urlStr, QMap<QString, QString>(), callback );
    }
}


void
JSResolverHelper::reportStreamUrl( const QString& qid,
                                         const QString& streamUrl, const QVariantMap& headers )
{
    if ( !m_streamCallbacks.contains( qid ) )
        return;

    std::function< void( const QString&, QSharedPointer< QIODevice >& ) > callback = m_streamCallbacks.take( qid );

    QMap<QString, QString> parsedHeaders;
    foreach ( const QString& key,  headers.keys()) {
        Q_ASSERT_X( headers[key].canConvert( QVariant::String ), Q_FUNC_INFO, "Expected a Map of string for additional headers" );
        if ( headers[key].canConvert( QVariant::String ) ) {
            parsedHeaders.insert( key, headers[key].toString() );
        }
    }

    returnStreamUrl( streamUrl, parsedHeaders, callback );
}


void
JSResolverHelper::nativeRetrieveMetadata( int metadataId, const QString& url,
                                          const QString& mime_type, int sizehint,
                                          const QVariantMap& options )
{
    if ( sizehint <= 0 )
    {
        QString javascript = QString( "Tomahawk.retrievedMetadata( %1, null, 'Supplied size is not (yet) supported');" )
                .arg( metadataId );
        m_resolver->d_func()->scriptPlugin->evaluateJavaScript( javascript );
        return;
    }

    if ( TomahawkUtils::isHttpResult( url ) || TomahawkUtils::isHttpsResult( url ) )
    {
        QMap<QString, QString> headers;
        if ( options.contains( "headers" ) && options["headers"].canConvert( QVariant::Map ) )
        {
            const QVariantMap variantHeaders = options["headers"].toMap();
            foreach ( const QString& key, variantHeaders.keys() ) {
                headers.insert( key, variantHeaders[key].toString() );
            }
        }

        // TODO: Add heuristic if size is not defined
        CloudStream stream( url, sizehint, headers,
                            Tomahawk::Utils::nam() );
        stream.Precache();
        QScopedPointer<TagLib::File> tag;
        if ( mime_type == "audio/mpeg" )
        {
            tag.reset( new TagLib::MPEG::File( &stream,
                TagLib::ID3v2::FrameFactory::instance(),
                TagLib::AudioProperties::Accurate
            ));
        }
        else if ( mime_type == "audio/mp4" )
        {
            tag.reset( new TagLib::MP4::File( &stream,
                true, TagLib::AudioProperties::Accurate
            ));
        }
#if defined(TAGLIB_MAJOR_VERSION) && defined(TAGLIB_MINOR_VERSION)
#if TAGLIB_MAJOR_VERSION >= 1 && TAGLIB_MINOR_VERSION >= 9
        else if ( mime_type == "application/opus" || mime_type == "audio/opus" )
        {
            tag.reset( new TagLib::Ogg::Opus::File( &stream, true,
                TagLib::AudioProperties::Accurate
            ));
        }
#endif
#endif
        else if ( mime_type == "application/ogg" || mime_type == "audio/ogg" )
        {
            tag.reset( new TagLib::Ogg::Vorbis::File( &stream, true,
                TagLib::AudioProperties::Accurate
            ));
        }
        else if ( mime_type == "application/x-flac" || mime_type == "audio/flac" ||
                   mime_type == "audio/x-flac" )
        {
            tag.reset( new TagLib::FLAC::File( &stream,
                TagLib::ID3v2::FrameFactory::instance(),
                true, TagLib::AudioProperties::Accurate
            ));
        }
        else if ( mime_type == "audio/x-ms-wma" )
        {
            tag.reset( new TagLib::ASF::File( &stream, true,
                TagLib::AudioProperties::Accurate
            ));
        }
        else
        {
            QString javascript = QString( "Tomahawk.retrievedMetadata( %1, null, 'Unknown mime type for tagging: %2');" )
                    .arg( metadataId ).arg( mime_type );
            m_resolver->d_func()->scriptPlugin->evaluateJavaScript( javascript );
            return;
        }

        if ( stream.num_requests() > 2)
        {
            // Warn if pre-caching failed.
            tLog() << "Total requests for file:" << url
                   << stream.num_requests() << stream.cached_bytes();
        }

        if ( !tag->tag() || tag->tag()->isEmpty() )
        {
            QString javascript = QString( "Tomahawk.retrievedMetadata( %1, null, 'Could not read tag information.');" )
                    .arg( metadataId );
            m_resolver->d_func()->scriptPlugin->evaluateJavaScript( javascript );
            return;
        }

        QVariantMap m;
        m["url"] = url;
        m["track"] = QString( tag->tag()->title().toCString() ).trimmed();
        m["album"] = QString( tag->tag()->album().toCString() ).trimmed();
        m["artist"] = QString( tag->tag()->artist().toCString() ).trimmed();

        if ( m["track"].toString().isEmpty() )
        {
            QString javascript = QString( "Tomahawk.retrievedMetadata( %1, null, 'Empty track returnd');" )
                    .arg( metadataId );
            m_resolver->d_func()->scriptPlugin->evaluateJavaScript( javascript );
            return;
        }

        if ( m["artist"].toString().isEmpty() )
        {
            QString javascript = QString( "Tomahawk.retrievedMetadata( %1, null, 'Empty artist returnd');" )
                    .arg( metadataId );
            m_resolver->d_func()->scriptPlugin->evaluateJavaScript( javascript );
            return;
        }

        if ( tag->audioProperties() )
        {
            m["bitrate"] = tag->audioProperties()->bitrate();
            m["channels"] = tag->audioProperties()->channels();
            m["duration"] = tag->audioProperties()->length();
            m["samplerate"] = tag->audioProperties()->sampleRate();
        }

        QString javascript = QString( "Tomahawk.retrievedMetadata( %1, %2 );" )
                .arg( metadataId )
                .arg( QString::fromLatin1( TomahawkUtils::toJson( m ) ) );
        m_resolver->d_func()->scriptPlugin->evaluateJavaScript( javascript );
    }
    else
    {
        QString javascript = QString( "Tomahawk.retrievedMetadata( %1, null, 'Protocol not supported');" )
                .arg( metadataId );
        m_resolver->d_func()->scriptPlugin->evaluateJavaScript( javascript );
    }
}


void
JSResolverHelper::nativeAsyncRequest( const int requestId, const QString& url,
                                      const QVariantMap& headers,
                                      const QVariantMap& options )
{
    QNetworkRequest req( url );
    foreach ( const QString& key , headers.keys() ) {
        req.setRawHeader( key.toLatin1(), headers[key].toString().toLatin1() );
    }

    if ( options.contains( "username" ) && options.contains( "password" ) )
    {
        // If we have sufficient authentication data, we will send
        // username+password as HTTP Basic Auth
        QString credentials = QString( "Basic %1" )
                .arg( QString( QString("%1:%2")
                        .arg( options["username"].toString() )
                        .arg( options["password"].toString() )
                        .toLatin1().toBase64() )
                );
        req.setRawHeader( "Authorization", credentials.toLatin1() );
    }

    NetworkReply* reply = NULL;
    if ( options.contains( "method") && options["method"].toString().toUpper() == "POST" ) {
        QByteArray data;
        if ( options.contains( "data" ) ) {
            data = options["data"].toString().toLatin1();
        }
        reply = new NetworkReply( Tomahawk::Utils::nam()->post( req, data ) );
    } else if ( options.contains( "method") && options["method"].toString().toUpper() == "HEAD" ) {
        reply = new NetworkReply( Tomahawk::Utils::nam()->head( req ) );
    } else {
        reply = new NetworkReply( Tomahawk::Utils::nam()->get( req ) );
    }

    NewClosure( reply , SIGNAL( finished() ), this, SLOT( nativeAsyncRequestDone( int, NetworkReply* ) ), requestId, reply );
}


void
JSResolverHelper::nativeAsyncRequestDone( int requestId, NetworkReply* reply )
{
    QVariantMap map;
    map["response"] = QString::fromUtf8( reply->reply()->readAll() );
    map["responseText"] = map["response"];
    map["responseType"] = QString(); // Default, indicates a string in map["response"]
    map["readyState"] = 4;
    map["status"] = reply->reply()->attribute( QNetworkRequest::HttpStatusCodeAttribute ).toInt();
    map["statusText"] = QString("%1 %2").arg( map["status"].toString() )
            .arg( reply->reply()->attribute( QNetworkRequest::HttpReasonPhraseAttribute ).toString() );

    bool ok = false;
    QString json = QString::fromUtf8( TomahawkUtils::toJson( map, &ok ) );
    Q_ASSERT( ok );

    QString javascript = QString( "Tomahawk.nativeAsyncRequestDone( %1, %2 );" )
            .arg( QString::number( requestId ) )
            .arg( json );
    m_resolver->d_func()->scriptPlugin->evaluateJavaScript( javascript );
}


bool
JSResolverHelper::hasFuzzyIndex()
{
    return !m_resolver->d_func()->fuzzyIndex.isNull();
}


bool
JSResolverHelper::indexDataFromVariant( const QVariantMap &map, struct Tomahawk::IndexData& indexData )
{
    // We do not use artistId at the moment
    indexData.artistId = 0;

    if ( map.contains( "album" ) )
    {
        indexData.album = map["album"].toString();
    }
    else
    {
        indexData.album = QString();
    }

    // Check that we have the three required attributes
    if ( !map.contains( "id" ) || !map["id"].canConvert( QVariant::Int )
         || !map.contains( "track" ) || !map.contains( "artist" ) )
    {
        return false;
    }

    bool ok;
    indexData.id = map["id"].toInt( &ok );
    if ( !ok )
    {
        return false;
    }

    indexData.artist = map["artist"].toString().trimmed();
    if ( indexData.artist.isEmpty() )
    {
        return false;
    }

    indexData.track = map["track"].toString().trimmed();
    if ( indexData.track.isEmpty() )
    {
        return false;
    }

    return true;
}


void
JSResolverHelper::createFuzzyIndex( const QVariantList& list )
{
    if ( hasFuzzyIndex() )
    {
        m_resolver->d_func()->fuzzyIndex->wipeIndex();
    }
    else
    {
        m_resolver->d_func()->fuzzyIndex.reset( new FuzzyIndex( m_resolver, accountId() + ".lucene" , true ) );
    }

    addToFuzzyIndex( list );
}


void
JSResolverHelper::addToFuzzyIndex( const QVariantList& list )
{
    if ( !hasFuzzyIndex() )
    {
        tLog() << Q_FUNC_INFO << "Cannot add entries to non-existing index.";
        return;
    }

    m_resolver->d_func()->fuzzyIndex->beginIndexing();

    foreach ( const QVariant& variant, list )
    {
        // Convert each entry to IndexData
        if ( variant.canConvert( QVariant::Map ) ) {
            QVariantMap map = variant.toMap();

            // Convert each entry and do multiple checks that we have valid data.
            struct IndexData indexData;

            if ( indexDataFromVariant( map, indexData ) )
            {
                m_resolver->d_func()->fuzzyIndex->appendFields( indexData );
            }
        }
    }

    m_resolver->d_func()->fuzzyIndex->endIndexing();
}


bool
cmpTuple ( const QVariant& x, const QVariant& y )
{
    return x.toList().at( 1 ).toFloat() < y.toList().at( 1 ).toFloat();
}


QVariantList
JSResolverHelper::searchInFuzzyIndex( const query_ptr& query )
{
    if ( m_resolver->d_func()->fuzzyIndex )
    {
        QMap<int, float> map = m_resolver->d_func()->fuzzyIndex->search( query );

        // Convert map to sorted QVariantList
        QVariantList list;
        foreach ( int id, map.keys() ) {
            QVariantList innerList;
            innerList.append( QVariant( id ) );
            innerList.append( QVariant( map[id] ) );
            // Wrap into QVariant or the list will be flattend
            list.append( QVariant( innerList  ));
        }
        std::sort( list.begin(), list.end(), cmpTuple );

        return list;
    }
    return QVariantList();
}


QVariantList
JSResolverHelper::searchFuzzyIndex( const QString& query )
{
    return searchInFuzzyIndex( Query::get( query, QString() ) );
}


QVariantList
JSResolverHelper::resolveFromFuzzyIndex( const QString& artist, const QString& album, const QString& track )
{
    // Important: Do not autoresolve!
    query_ptr query = Query::get( artist, track, album, QString(), false );
    if ( query.isNull() ) {
        tLog() << Q_FUNC_INFO << "Could not create a query for" << artist << "-" << track;
        return QVariantList();
    }
    return searchInFuzzyIndex( query );
}


void
JSResolverHelper::deleteFuzzyIndex()
{
    if ( m_resolver->d_func()->fuzzyIndex )
    {
        m_resolver->d_func()->fuzzyIndex->deleteIndex();
        m_resolver->d_func()->fuzzyIndex->deleteLater();
        m_resolver->d_func()->fuzzyIndex.reset();
    }
}


void
JSResolverHelper::returnStreamUrl( const QString& streamUrl, const QMap<QString, QString>& headers,
                                   std::function< void( const QString&, QSharedPointer< QIODevice >& ) > callback )
{
    if ( streamUrl.isEmpty() || !( TomahawkUtils::isHttpResult( streamUrl ) || TomahawkUtils::isHttpsResult( streamUrl ) ) )
    {
        // Not an https? URL, so let Phonon handle it
        QSharedPointer< QIODevice > sp;
        callback( streamUrl, sp );
    }
    else
    {
        QUrl url = QUrl::fromEncoded( streamUrl.toUtf8() );
        QNetworkRequest req( url );
        foreach ( const QString& key , headers.keys() ) {
            req.setRawHeader( key.toLatin1(), headers[key].toLatin1() );
        }
        tDebug() << "Creating a QNetowrkReply with url:" << req.url().toString();
        NetworkReply* reply = new NetworkReply( Tomahawk::Utils::nam()->get( req ) );

        NewClosure( reply , SIGNAL( finalUrlReached() ), this, SLOT( gotStreamUrl( IODeviceCallback, NetworkReply* )), callback, reply );
    }
}


Q_DECLARE_METATYPE( IODeviceCallback )

void
JSResolverHelper::gotStreamUrl( std::function< void( const QString&, QSharedPointer< QIODevice >& ) > callback, NetworkReply* reply )
{
    // std::functions cannot accept temporaries as parameters
    QSharedPointer< QIODevice > sp ( reply->reply(), &QObject::deleteLater );
    QString url = reply->reply()->url().toString();
    reply->disconnectFromReply();
    reply->deleteLater();

    callback( url, sp );
}
