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

#include "resolvers/ScriptEngine.h"
#include "network/Servent.h"
#include "config.h"
#include "JSResolver.h"
#include "Pipeline.h"
#include "Result.h"

#include <boost/bind.hpp>
#include <QtCrypto>

JSResolverHelper::JSResolverHelper( const QString& scriptPath, JSResolver* parent )
    : QObject( parent )
    , m_urlCallbackIsAsync( false )
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

    boost::function< void( const Tomahawk::result_ptr&,
                           boost::function< void( QSharedPointer< QIODevice >& ) > )> fac =
            boost::bind( &JSResolverHelper::customIODeviceFactory, this, _1, _2 );
    Servent::instance()->registerIODeviceFactory( protocol, fac );

    m_urlCallback = callbackFuncName;
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
JSResolverHelper::customIODeviceFactory( const Tomahawk::result_ptr& result,
                                               boost::function< void( QSharedPointer< QIODevice >& ) > callback )
{
    //can be sync or async
    QString origResultUrl = QString( QUrl( result->url() ).toEncoded() );

    if ( m_urlCallbackIsAsync )
    {
        QString qid = uuid();
        QString getUrl = QString( "Tomahawk.resolver.instance.%1( '%2', '%3' );" ).arg( m_urlCallback )
                                                                                  .arg( qid )
                                                                                  .arg( origResultUrl );

        m_streamCallbacks.insert( qid, callback );
        m_resolver->m_engine->mainFrame()->evaluateJavaScript( getUrl );
    }
    else
    {
        QString getUrl = QString( "Tomahawk.resolver.instance.%1( '%2' );" ).arg( m_urlCallback )
                                                                            .arg( origResultUrl );

        QString urlStr = m_resolver->m_engine->mainFrame()->evaluateJavaScript( getUrl ).toString();

        returnStreamUrl( urlStr, callback );
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
    QNetworkReply* reply = TomahawkUtils::nam()->get( req );

    //boost::functions cannot accept temporaries as parameters
    sp = QSharedPointer< QIODevice >( reply, &QObject::deleteLater );
    callback( sp );
}
