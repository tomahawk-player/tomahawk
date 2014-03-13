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
#include "utils/NetworkAccessManager.h"
#include "utils/Logger.h"

#include "config.h"
#include "JSResolver_p.h"
#include "Pipeline.h"
#include "Result.h"
#include "SourceList.h"
#include "UrlHandler.h"

#include <boost/bind.hpp>
#include <QFile>
#include <QFileInfo>
#include <QtCrypto>
#include <QWebFrame>

using namespace Tomahawk;

JSResolverHelper::JSResolverHelper( const QString& scriptPath, JSResolver* parent )
    : QObject( parent )
    , m_urlCallbackIsAsync( false )
    , m_urlTranslatorIsAsync( false )
{
    m_scriptPath = scriptPath;
    m_resolver = parent;
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
    tLog() << m_scriptPath << ":" << message;
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
        guid += Tomahawk::Database::instance()->impl()->dbid();

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
        QString guid = QString( "xspf-%1-%2" ).arg( xspfUrl.toUtf8().toBase64().constData() ).arg( Tomahawk::Database::instance()->impl()->dbid() );

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
    bool ok = 0;
    int intCap = v.toInt( &ok );
    Tomahawk::ExternalResolver::Capabilities capabilities;
    if ( !ok )
        capabilities = Tomahawk::ExternalResolver::NullCapability;
    else
        capabilities = static_cast< Tomahawk::ExternalResolver::Capabilities >( intCap );

    m_resolver->onCapabilitiesChanged( capabilities );
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
JSResolverHelper::hmac( const QByteArray& key, const QByteArray &input )
{
#ifdef QCA2_FOUND
    if ( !QCA::isSupported( "hmac(md5)" ) )
    {
        tLog() << "HMAC(md5) not supported with qca-ossl plugin, or qca-ossl plugin is not installed! Unable to generate signature!";
        return QByteArray();
    }

    QCA::MessageAuthenticationCode md5hmac1( "hmac(md5)", QCA::SecureArray() );
    QCA::SymmetricKey keyObject( key );
    md5hmac1.setup( keyObject );

    md5hmac1.update( QCA::SecureArray( input ) );
    QCA::SecureArray resultArray = md5hmac1.final();

    QString result = QCA::arrayToHex( resultArray.toByteArray() );
    return result.toUtf8();
#else
    tLog() << "Tomahawk compiled without QCA support, cannot generate HMAC signature";
    return QString();
#endif
}


QString
JSResolverHelper::md5( const QByteArray& input )
{
    QByteArray const digest = QCryptographicHash::hash( input, QCryptographicHash::Md5 );
    return QString::fromLatin1( digest.toHex() );
}

void
JSResolverHelper::addCustomUrlHandler( const QString& protocol,
                                             const QString& callbackFuncName,
                                             const QString& isAsynchronous )
{
    m_urlCallbackIsAsync = ( isAsynchronous.toLower() == "true" ) ? true : false;

    boost::function< void( const Tomahawk::result_ptr&, const QString&,
                           boost::function< void( QSharedPointer< QIODevice >& ) > )> fac =
            boost::bind( &JSResolverHelper::customIODeviceFactory, this, _1, _2, _3 );
    Tomahawk::UrlHandler::registerIODeviceFactory( protocol, fac );

    m_urlCallback = callbackFuncName;
}


void
JSResolverHelper::addCustomUrlTranslator( const QString& protocol,
                                             const QString& callbackFuncName,
                                             const QString& isAsynchronous )
{
    m_urlTranslatorIsAsync = ( isAsynchronous.toLower() == "true" ) ? true : false;

    boost::function< void( const Tomahawk::result_ptr&, const QString&,
                           boost::function< void( const QString& ) > )> fac =
            boost::bind( &JSResolverHelper::customUrlTranslator, this, _1, _2, _3 );
    Tomahawk::UrlHandler::registerUrlTranslator( protocol, fac );

    m_urlTranslator = callbackFuncName;
}


void
JSResolverHelper::reportUrlTranslation( const QString& qid, const QString& streamUrl )
{
    if ( !m_translatorCallbacks.contains( qid ) )
        return;

    boost::function< void( const QString& ) > callback = m_translatorCallbacks.take( qid );

    returnUrlTranslation( streamUrl, callback );
}


QByteArray
JSResolverHelper::base64Encode( const QByteArray& input )
{
    return input.toBase64();
}


QByteArray
JSResolverHelper::base64Decode( const QByteArray& input )
{
    return QByteArray::fromBase64( input );
}


void
JSResolverHelper::customIODeviceFactory( const Tomahawk::result_ptr&, const QString& url,
                                               boost::function< void( QSharedPointer< QIODevice >& ) > callback )
{
    //can be sync or async
    QString origResultUrl = QString( QUrl( url ).toEncoded() );

    if ( m_urlCallbackIsAsync )
    {
        QString qid = uuid();
        QString getUrl = QString( "Tomahawk.resolver.instance.%1( '%2', '%3' );" ).arg( m_urlCallback )
                                                                                  .arg( qid )
                                                                                  .arg( origResultUrl );

        m_streamCallbacks.insert( qid, callback );
        m_resolver->d_func()->engine->mainFrame()->evaluateJavaScript( getUrl );
    }
    else
    {
        QString getUrl = QString( "Tomahawk.resolver.instance.%1( '%2' );" ).arg( m_urlCallback )
                                                                            .arg( origResultUrl );

        QString urlStr = m_resolver->d_func()->engine->mainFrame()->evaluateJavaScript( getUrl ).toString();

        returnStreamUrl( urlStr, callback );
    }
}


void
JSResolverHelper::customUrlTranslator( const Tomahawk::result_ptr&, const QString& url, boost::function<void (const QString& )> callback )
{
    //can be sync or async
    QString origResultUrl = QString( QUrl( url ).toEncoded() );

    if ( m_urlTranslatorIsAsync )
    {
        QString qid = uuid();
        QString getUrl = QString( "Tomahawk.resolver.instance.%1( '%2', '%3' );" ).arg( m_urlTranslator )
                                                                                  .arg( qid )
                                                                                  .arg( origResultUrl );

        m_translatorCallbacks.insert( qid, callback );
        m_resolver->d_func()->engine->mainFrame()->evaluateJavaScript( getUrl );
    }
    else
    {
        QString getUrl = QString( "Tomahawk.resolver.instance.%1( '%2' );" ).arg( m_urlTranslator )
                                                                            .arg( origResultUrl );

        QString urlStr = m_resolver->d_func()->engine->mainFrame()->evaluateJavaScript( getUrl ).toString();

        returnUrlTranslation( urlStr, callback );
    }
}


void
JSResolverHelper::reportStreamUrl( const QString& qid,
                                         const QString& streamUrl )
{
    if ( !m_streamCallbacks.contains( qid ) )
        return;

    boost::function< void( QSharedPointer< QIODevice >& ) > callback = m_streamCallbacks.take( qid );

    returnStreamUrl( streamUrl, callback );
}


void
JSResolverHelper::returnStreamUrl( const QString& streamUrl, boost::function< void( QSharedPointer< QIODevice >& ) > callback )
{
    QSharedPointer< QIODevice > sp;
    if ( streamUrl.isEmpty() )
    {
        callback( sp );
        return;
    }

    QUrl url = QUrl::fromEncoded( streamUrl.toUtf8() );
    QNetworkRequest req( url );
    tDebug() << "Creating a QNetowrkReply with url:" << req.url().toString();
    QNetworkReply* reply = Tomahawk::Utils::nam()->get( req );

    //boost::functions cannot accept temporaries as parameters
    sp = QSharedPointer< QIODevice >( reply, &QObject::deleteLater );
    callback( sp );
}


void
JSResolverHelper::returnUrlTranslation( const QString& streamUrl, boost::function<void (const QString& )> callback )
{
    if ( streamUrl.isEmpty() )
    {
        callback( QString() );
        return;
    }

    //boost::functions cannot accept temporaries as parameters
    // sp = QSharedPointer< QString >( streamUrl , &QObject::deleteLater );
    callback( streamUrl );
}
