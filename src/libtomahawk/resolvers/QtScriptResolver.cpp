/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2013,      Teo Mrnjavac <teo@kde.org>
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

#include "QtScriptResolver.h"

#include "Artist.h"
#include "Album.h"
#include "config.h"
#include "Pipeline.h"
#include "ScriptCollection.h"
#include "SourceList.h"

#include "accounts/AccountConfigWidget.h"

#include "network/Servent.h"

#include "jobview/JobStatusView.h"
#include "jobview/JobStatusModel.h"
#include "jobview/ErrorStatusMessage.h"

#include "utils/TomahawkUtilsGui.h"

#include "config.h"

#include <QImageReader>
#include <QMessageBox>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QMetaProperty>
#include <QCryptographicHash>

#include <boost/bind.hpp>

// FIXME: bloody hack, remove this for 0.3
// this one adds new functionality to old resolvers
#define RESOLVER_LEGACY_CODE "var resolver = Tomahawk.resolver.instance ? Tomahawk.resolver.instance : TomahawkResolver;"
// this one keeps old code invokable
#define RESOLVER_LEGACY_CODE2 "var resolver = Tomahawk.resolver.instance ? Tomahawk.resolver.instance : window;"


QtScriptResolverHelper::QtScriptResolverHelper( const QString& scriptPath, QtScriptResolver* parent )
    : QObject( parent )
    , m_urlCallbackIsAsync( false )
{
    m_scriptPath = scriptPath;
    m_resolver = parent;
}


QByteArray
QtScriptResolverHelper::readRaw( const QString& fileName )
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
QtScriptResolverHelper::compress( const QString& data )
{
    QByteArray comp = qCompress( data.toLatin1(), 9 );
    return comp.toBase64();
}


QString
QtScriptResolverHelper::readCompressed( const QString& fileName )
{
    return compress( readRaw( fileName ) );
}


QString
QtScriptResolverHelper::readBase64( const QString& fileName )
{
    return readRaw( fileName ).toBase64();
}


QVariantMap
QtScriptResolverHelper::resolverData()
{
    QVariantMap resolver;
    resolver["config"] = m_resolverConfig;
    resolver["scriptPath"] = m_scriptPath;
    return resolver;
}


void
QtScriptResolverHelper::log( const QString& message )
{
    tLog() << m_scriptPath << ":" << message;
}


void
QtScriptResolverHelper::addTrackResults( const QVariantMap& results )
{
    qDebug() << "Resolver reporting results:" << results;
    QList< Tomahawk::result_ptr > tracks = m_resolver->parseResultVariantList( results.value("results").toList() );

    QString qid = results.value("qid").toString();

    Tomahawk::Pipeline::instance()->reportResults( qid, tracks );
}


void
QtScriptResolverHelper::addArtistResults( const QVariantMap& results )
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
QtScriptResolverHelper::addAlbumResults( const QVariantMap& results )
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
QtScriptResolverHelper::addAlbumTrackResults( const QVariantMap& results )
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
QtScriptResolverHelper::reportCapabilities( const QVariant& v )
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
QtScriptResolverHelper::setResolverConfig( const QVariantMap& config )
{
    m_resolverConfig = config;
}


QString
QtScriptResolverHelper::hmac( const QByteArray& key, const QByteArray &input )
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
QtScriptResolverHelper::md5( const QByteArray& input )
{
    QByteArray const digest = QCryptographicHash::hash( input, QCryptographicHash::Md5 );
    return QString::fromLatin1( digest.toHex() );
}


void
QtScriptResolverHelper::addCustomUrlHandler( const QString& protocol,
                                             const QString& callbackFuncName,
                                             const QString& isAsynchronous )
{
    m_urlCallbackIsAsync = ( isAsynchronous.toLower() == "true" ) ? true : false;

    boost::function< void( const Tomahawk::result_ptr&,
                           boost::function< void( QSharedPointer< QIODevice >& ) > )> fac =
            boost::bind( &QtScriptResolverHelper::customIODeviceFactory, this, _1, _2 );
    Servent::instance()->registerIODeviceFactory( protocol, fac );

    m_urlCallback = callbackFuncName;
}


QByteArray
QtScriptResolverHelper::base64Encode( const QByteArray& input )
{
    return input.toBase64();
}


QByteArray
QtScriptResolverHelper::base64Decode( const QByteArray& input )
{
    return QByteArray::fromBase64( input );
}


void
QtScriptResolverHelper::customIODeviceFactory( const Tomahawk::result_ptr& result,
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
QtScriptResolverHelper::reportStreamUrl( const QString& qid,
                                         const QString& streamUrl )
{
    if ( !m_streamCallbacks.contains( qid ) )
        return;

    boost::function< void( QSharedPointer< QIODevice >& ) > callback = m_streamCallbacks.take( qid );

    returnStreamUrl( streamUrl, callback );
}


void
QtScriptResolverHelper::returnStreamUrl( const QString& streamUrl, boost::function< void( QSharedPointer< QIODevice >& ) > callback )
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


void
ScriptEngine::javaScriptConsoleMessage( const QString& message, int lineNumber, const QString& sourceID )
{
    tLog() << "JAVASCRIPT:" << m_scriptPath << message << lineNumber << sourceID;
#ifndef DEBUG_BUILD
    JobStatusView::instance()->model()->addJob( new ErrorStatusMessage( tr( "Script Resolver Error: %1 %2 %3 %4" ).arg( m_scriptPath ).arg( message ).arg( lineNumber ).arg( sourceID ) ) );
#endif
}


QtScriptResolver::QtScriptResolver( const QString& scriptPath, const QStringList& additionalScriptPaths )
    : Tomahawk::ExternalResolverGui( scriptPath )
    , m_ready( false )
    , m_stopped( true )
    , m_error( Tomahawk::ExternalResolver::NoError )
    , m_resolverHelper( new QtScriptResolverHelper( scriptPath, this ) )
    , m_requiredScriptPaths( additionalScriptPaths )
{
    tLog() << Q_FUNC_INFO << "Loading JS resolver:" << scriptPath;

    m_engine = new ScriptEngine( this );
    m_name = QFileInfo( filePath() ).baseName();

    // set the icon, if we launch properly we'll get the icon the resolver reports
    m_icon = TomahawkUtils::defaultPixmap( TomahawkUtils::DefaultResolver, TomahawkUtils::Original, QSize( 128, 128 ) );

    if ( !QFile::exists( filePath() ) )
    {
        tLog() << Q_FUNC_INFO << "Failed loading JavaScript resolver:" << scriptPath;
        m_error = Tomahawk::ExternalResolver::FileNotFound;
    }
    else
    {
        init();
    }
}


QtScriptResolver::~QtScriptResolver()
{
    if ( !m_stopped )
        stop();

    delete m_engine;
}


Tomahawk::ExternalResolver* QtScriptResolver::factory( const QString& scriptPath, const QStringList& additionalScriptPaths )
{
    ExternalResolver* res = 0;

    const QFileInfo fi( scriptPath );
    if ( fi.suffix() == "js" || fi.suffix() == "script" )
    {
        res = new QtScriptResolver( scriptPath, additionalScriptPaths );
        tLog() << Q_FUNC_INFO << scriptPath << "Loaded.";
    }

    return res;
}


bool
QtScriptResolver::running() const
{
    return m_ready && !m_stopped;
}


void
QtScriptResolver::reload()
{
    if ( QFile::exists( filePath() ) )
    {
        init();
        m_error = Tomahawk::ExternalResolver::NoError;
    } else
    {
        m_error = Tomahawk::ExternalResolver::FileNotFound;
    }
}


void
QtScriptResolver::init()
{
    QFile scriptFile( filePath() );
    if( !scriptFile.open( QIODevice::ReadOnly ) )
    {
        qWarning() << "Failed to read contents of file:" << filePath() << scriptFile.errorString();
        return;
    }
    const QByteArray scriptContents = scriptFile.readAll();

    m_engine->mainFrame()->setHtml( "<html><body></body></html>", QUrl( "file:///invalid/file/for/security/policy" ) );

    // add c++ part of tomahawk javascript library
    m_engine->mainFrame()->addToJavaScriptWindowObject( "Tomahawk", m_resolverHelper );

    // add rest of it
    m_engine->setScriptPath( "tomahawk.js" );
    QFile jslib( RESPATH "js/tomahawk.js" );
    jslib.open( QIODevice::ReadOnly );
    m_engine->mainFrame()->evaluateJavaScript( jslib.readAll() );
    jslib.close();

    // add resolver dependencies, if any
    foreach ( QString s, m_requiredScriptPaths )
    {
        QFile reqFile( s );
        if( !reqFile.open( QIODevice::ReadOnly ) )
        {
            qWarning() << "Failed to read contents of file:" << s << reqFile.errorString();
            return;
        }
        const QByteArray reqContents = reqFile.readAll();

        m_engine->setScriptPath( s );
        m_engine->mainFrame()->evaluateJavaScript( reqContents );
    }

    // add resolver
    m_engine->setScriptPath( filePath() );
    m_engine->mainFrame()->evaluateJavaScript( scriptContents );

    // init resolver
    resolverInit();

    QVariantMap m = resolverSettings();
    m_name    = m.value( "name" ).toString();
    m_weight  = m.value( "weight", 0 ).toUInt();
    m_timeout = m.value( "timeout", 25 ).toUInt() * 1000;
    bool compressed = m.value( "compressed", "false" ).toString() == "true";

    QByteArray icoData = m.value( "icon" ).toByteArray();
    if( compressed )
        icoData = qUncompress( QByteArray::fromBase64( icoData ) );
    else
        icoData = QByteArray::fromBase64( icoData );
    QPixmap ico;
    ico.loadFromData( icoData );

    bool success = false;
    if ( !ico.isNull() )
    {
        m_icon = ico.scaled( m_icon.size(), Qt::IgnoreAspectRatio );
        success = true;
    }
    // see if the resolver sent an icon path to not break the old (unofficial) api.
    // TODO: remove this and publish a definitive api
    if ( !success )
    {
        QString iconPath = QFileInfo( filePath() ).path() + "/" + m.value( "icon" ).toString();
        success = m_icon.load( iconPath );
    }

    // load config widget and apply settings
    loadUi();
    QVariantMap config = resolverUserConfig();
    fillDataInWidgets( config );

    qDebug() << "JS" << filePath() << "READY," << "name" << m_name << "weight" << m_weight << "timeout" << m_timeout << "icon received" << success;

    m_ready = true;
}


void
QtScriptResolver::start()
{
    m_stopped = false;
    if ( m_ready )
        Tomahawk::Pipeline::instance()->addResolver( this );
    else
        init();
}


void
QtScriptResolver::artists( const Tomahawk::collection_ptr& collection )
{
    if ( QThread::currentThread() != thread() )
    {
        QMetaObject::invokeMethod( this, "artists", Qt::QueuedConnection, Q_ARG( Tomahawk::collection_ptr, collection ) );
        return;
    }

    if ( !m_collections.contains( collection->name() ) || //if the collection doesn't belong to this resolver
         !capabilities().testFlag( Browsable ) )          //or this resolver doesn't even support collections
    {
        emit artistsFound( QList< Tomahawk::artist_ptr >() );
        return;
    }

    QString eval = QString( "resolver.artists( '%1' );" )
                   .arg( collection->name().replace( "'", "\\'" ) );

    QVariantMap m = m_engine->mainFrame()->evaluateJavaScript( eval ).toMap();
    if ( m.isEmpty() )
    {
        // if the resolver doesn't return anything, async api is used
        return;
    }

    QString errorMessage = tr( "Script Resolver Warning: API call %1 returned data synchronously." ).arg( eval );
    JobStatusView::instance()->model()->addJob( new ErrorStatusMessage( errorMessage ) );
    tDebug() << errorMessage << m;
}


void
QtScriptResolver::albums( const Tomahawk::collection_ptr& collection, const Tomahawk::artist_ptr& artist )
{
    if ( QThread::currentThread() != thread() )
    {
        QMetaObject::invokeMethod( this, "albums", Qt::QueuedConnection,
                                   Q_ARG( Tomahawk::collection_ptr, collection ),
                                   Q_ARG( Tomahawk::artist_ptr, artist ) );
        return;
    }

    if ( !m_collections.contains( collection->name() ) || //if the collection doesn't belong to this resolver
         !capabilities().testFlag( Browsable ) )          //or this resolver doesn't even support collections
    {
        emit albumsFound( QList< Tomahawk::album_ptr >() );
        return;
    }

    QString eval = QString( "resolver.albums( '%1', '%2' );" )
                   .arg( collection->name().replace( "'", "\\'" ) )
                   .arg( artist->name().replace( "'", "\\'" ) );

    QVariantMap m = m_engine->mainFrame()->evaluateJavaScript( eval ).toMap();
    if ( m.isEmpty() )
    {
        // if the resolver doesn't return anything, async api is used
        return;
    }

    QString errorMessage = tr( "Script Resolver Warning: API call %1 returned data synchronously." ).arg( eval );
    JobStatusView::instance()->model()->addJob( new ErrorStatusMessage( errorMessage ) );
    tDebug() << errorMessage << m;
}


void
QtScriptResolver::tracks( const Tomahawk::collection_ptr& collection, const Tomahawk::album_ptr& album )
{
    if ( QThread::currentThread() != thread() )
    {
        QMetaObject::invokeMethod( this, "tracks", Qt::QueuedConnection,
                                   Q_ARG( Tomahawk::collection_ptr, collection ),
                                   Q_ARG( Tomahawk::album_ptr, album ) );
        return;
    }

    if ( !m_collections.contains( collection->name() ) || //if the collection doesn't belong to this resolver
         !capabilities().testFlag( Browsable ) )          //or this resolver doesn't even support collections
    {
        emit tracksFound( QList< Tomahawk::query_ptr >() );
        return;
    }

    QString eval = QString( "resolver.tracks( '%1', '%2', '%3' );" )
                   .arg( collection->name().replace( "'", "\\'" ) )
                   .arg( album->artist()->name().replace( "'", "\\'" ) )
                   .arg( album->name().replace( "'", "\\'" ) );

    QVariantMap m = m_engine->mainFrame()->evaluateJavaScript( eval ).toMap();
    if ( m.isEmpty() )
    {
        // if the resolver doesn't return anything, async api is used
        return;
    }

    QString errorMessage = tr( "Script Resolver Warning: API call %1 returned data synchronously." ).arg( eval );
    JobStatusView::instance()->model()->addJob( new ErrorStatusMessage( errorMessage ) );
    tDebug() << errorMessage << m;
}


Tomahawk::ExternalResolver::ErrorState
QtScriptResolver::error() const
{
    return m_error;
}


void
QtScriptResolver::resolve( const Tomahawk::query_ptr& query )
{
    if ( QThread::currentThread() != thread() )
    {
        QMetaObject::invokeMethod( this, "resolve", Qt::QueuedConnection, Q_ARG(Tomahawk::query_ptr, query) );
        return;
    }

    QString eval;
    if ( !query->isFullTextQuery() )
    {
        eval = QString( RESOLVER_LEGACY_CODE2 "resolver.resolve( '%1', '%2', '%3', '%4' );" )
                  .arg( query->id().replace( "'", "\\'" ) )
                  .arg( query->artist().replace( "'", "\\'" ) )
                  .arg( query->album().replace( "'", "\\'" ) )
                  .arg( query->track().replace( "'", "\\'" ) );
    }
    else
    {
        eval = QString( "if(Tomahawk.resolver.instance !== undefined) {"
                        "   resolver.search( '%1', '%2' );"
                        "} else {"
                        "   resolve( '%1', '', '', '%2' );"
                        "}"
                      )
                  .arg( query->id().replace( "'", "\\'" ) )
                  .arg( query->fullTextQuery().replace( "'", "\\'" ) );
    }

    QVariantMap m = m_engine->mainFrame()->evaluateJavaScript( eval ).toMap();
    if ( m.isEmpty() )
    {
        // if the resolver doesn't return anything, async api is used
        return;
    }

    qDebug() << "JavaScript Result:" << m;

    const QString qid = query->id();
    const QVariantList reslist = m.value( "results" ).toList();

    QList< Tomahawk::result_ptr > results = parseResultVariantList( reslist );

    Tomahawk::Pipeline::instance()->reportResults( qid, results );
}


QList< Tomahawk::result_ptr >
QtScriptResolver::parseResultVariantList( const QVariantList& reslist )
{
    QList< Tomahawk::result_ptr > results;

    foreach( const QVariant& rv, reslist )
    {
        QVariantMap m = rv.toMap();
        if ( m.value( "artist" ).toString().trimmed().isEmpty() || m.value( "track" ).toString().trimmed().isEmpty() )
            continue;

        // TODO we need to handle preview urls separately. they should never trump a real url, and we need to display
        // the purchaseUrl for the user to upgrade to a full stream.
        if ( m.value( "preview" ).toBool() == true )
            continue;

        Tomahawk::result_ptr rp = Tomahawk::Result::get( m.value( "url" ).toString() );
        Tomahawk::artist_ptr ap = Tomahawk::Artist::get( m.value( "artist" ).toString(), false );
        rp->setArtist( ap );
        rp->setAlbum( Tomahawk::Album::get( ap, m.value( "album" ).toString(), false ) );
        rp->setTrack( m.value( "track" ).toString() );
        rp->setAlbumPos( m.value( "albumpos" ).toUInt() );
        rp->setBitrate( m.value( "bitrate" ).toUInt() );
        rp->setSize( m.value( "size" ).toUInt() );
        rp->setRID( uuid() );
        rp->setFriendlySource( name() );
        rp->setPurchaseUrl( m.value( "purchaseUrl" ).toString() );
        rp->setLinkUrl( m.value( "linkUrl" ).toString() );
        rp->setScore( m.value( "score" ).toFloat() );
        rp->setDiscNumber( m.value( "discnumber" ).toUInt() );

        if ( m.contains( "year" ) )
        {
            QVariantMap attr;
            attr[ "releaseyear" ] = m.value( "year" );
            rp->setAttributes( attr );
        }

        rp->setDuration( m.value( "duration", 0 ).toUInt() );
        if ( rp->duration() <= 0 && m.contains( "durationString" ) )
        {
            QTime time = QTime::fromString( m.value( "durationString" ).toString(), "hh:mm:ss" );
            rp->setDuration( time.secsTo( QTime( 0, 0 ) ) * -1 );
        }

        rp->setMimetype( m.value( "mimetype" ).toString() );
        if ( rp->mimetype().isEmpty() )
        {
            rp->setMimetype( TomahawkUtils::extensionToMimetype( m.value( "extension" ).toString() ) );
            Q_ASSERT( !rp->mimetype().isEmpty() );
        }

        rp->setResolvedBy( this );
        results << rp;
    }

    return results;
}

QList< Tomahawk::artist_ptr >
QtScriptResolver::parseArtistVariantList( const QVariantList& reslist )
{
    QList< Tomahawk::artist_ptr > results;

    foreach( const QVariant& rv, reslist )
    {
        if ( rv.toString().trimmed().isEmpty() )
            continue;

        Tomahawk::artist_ptr ap = Tomahawk::Artist::get( rv.toString(), false );

        results << ap;
    }

    return results;
}


QList< Tomahawk::album_ptr >
QtScriptResolver::parseAlbumVariantList( const Tomahawk::artist_ptr& artist, const QVariantList& reslist )
{
    QList< Tomahawk::album_ptr > results;

    foreach( const QVariant& rv, reslist )
    {
        if ( rv.toString().trimmed().isEmpty() )
            continue;

        Tomahawk::album_ptr ap = Tomahawk::Album::get( artist, rv.toString(), false );

        results << ap;
    }

    return results;
}


void
QtScriptResolver::stop()
{
    m_stopped = true;

    foreach ( const Tomahawk::collection_ptr& collection, m_collections )
    {
        emit collectionRemoved( collection );
    }

    Tomahawk::Pipeline::instance()->removeResolver( this );
    emit stopped();
}


void
QtScriptResolver::loadUi()
{
    QVariantMap m = m_engine->mainFrame()->evaluateJavaScript( RESOLVER_LEGACY_CODE  "resolver.getConfigUi();" ).toMap();
    m_dataWidgets = m["fields"].toList();

    bool compressed = m.value( "compressed", "false" ).toBool();
    qDebug() << "Resolver has a preferences widget! compressed?" << compressed;

    QByteArray uiData = m[ "widget" ].toByteArray();

    if( compressed )
        uiData = qUncompress( QByteArray::fromBase64( uiData ) );
    else
        uiData = QByteArray::fromBase64( uiData );

    QVariantMap images;
    foreach(const QVariant& item, m[ "images" ].toList())
    {
        QString key = item.toMap().keys().first();
        QVariant value = item.toMap().value(key);
        images[key] = value;
    }

    if( m.contains( "images" ) )
        uiData = fixDataImagePaths( uiData, compressed, images );

    m_configWidget = QPointer< AccountConfigWidget >( widgetFromData( uiData, 0 ) );

    emit changed();
}


AccountConfigWidget*
QtScriptResolver::configUI() const
{
    if( m_configWidget.isNull() )
        return 0;
    else
        return m_configWidget.data();
}


void
QtScriptResolver::saveConfig()
{
    QVariant saveData = loadDataFromWidgets();
//    qDebug() << Q_FUNC_INFO << saveData;

    m_resolverHelper->setResolverConfig( saveData.toMap() );
    m_engine->mainFrame()->evaluateJavaScript( RESOLVER_LEGACY_CODE "resolver.saveUserConfig();" );
}


QVariant
QtScriptResolver::widgetData(QWidget* widget, const QString& property)
{
    for( int i = 0; i < widget->metaObject()->propertyCount(); i++ )
    {
        if( widget->metaObject()->property( i ).name() == property )
        {
            return widget->property( property.toLatin1() );
        }
    }

    return QVariant();
}


void
QtScriptResolver::setWidgetData(const QVariant& value, QWidget* widget, const QString& property)
{
    for( int i = 0; i < widget->metaObject()->propertyCount(); i++ )
    {
        if( widget->metaObject()->property( i ).name() == property )
        {
            widget->metaObject()->property( i ).write( widget, value);
            return;
        }
    }
}


QVariantMap
QtScriptResolver::loadDataFromWidgets()
{
    QVariantMap saveData;
    foreach(const QVariant& dataWidget, m_dataWidgets)
    {
        QVariantMap data = dataWidget.toMap();

        QString widgetName = data["widget"].toString();
        QWidget* widget= m_configWidget.data()->findChild<QWidget*>( widgetName );

        QVariant value = widgetData( widget, data["property"].toString() );

        saveData[ data["name"].toString() ] = value;
    }

    return saveData;
}


void
QtScriptResolver::fillDataInWidgets( const QVariantMap& data )
{
    foreach(const QVariant& dataWidget, m_dataWidgets)
    {
        QString widgetName = dataWidget.toMap()["widget"].toString();
        QWidget* widget= m_configWidget.data()->findChild<QWidget*>( widgetName );
        if( !widget )
        {
            tLog() << Q_FUNC_INFO << "Widget specified in resolver was not found:" << widgetName;
            Q_ASSERT(false);
            return;
        }

        QString propertyName = dataWidget.toMap()["property"].toString();
        QString name = dataWidget.toMap()["name"].toString();

        setWidgetData( data[ name ], widget, propertyName );
    }
}


void
QtScriptResolver::onCapabilitiesChanged( Tomahawk::ExternalResolver::Capabilities capabilities )
{
    m_capabilities = capabilities;
    loadCollections();
}


void
QtScriptResolver::loadCollections()
{
    if ( m_capabilities.testFlag( Browsable ) )
    {
        QVariantMap collectionInfo = m_engine->mainFrame()->evaluateJavaScript( "resolver.collection();" ).toMap();
        if ( collectionInfo.isEmpty() ||
             !collectionInfo.contains( "prettyname" ) ||
             !collectionInfo.contains( "description" ) )
            return;

        QString prettyname = collectionInfo.value( "prettyname" ).toString();
        QString desc = collectionInfo.value( "description" ).toString();

        foreach ( Tomahawk::collection_ptr collection, m_collections )
        {
            emit collectionRemoved( collection );
        }

        m_collections.clear();
        // at this point we assume that all the tracks browsable through a resolver belong to the local source
        Tomahawk::ScriptCollection* sc = new Tomahawk::ScriptCollection( SourceList::instance()->getLocal(), this );
        sc->setServiceName( prettyname );
        sc->setDescription( desc );

        if ( collectionInfo.contains( "trackcount" ) ) //a resolver might not expose this
        {
            bool ok = false;
            int trackCount = collectionInfo.value( "trackcount" ).toInt( &ok );
            if ( ok )
                sc->setTrackCount( trackCount );
        }

        if ( collectionInfo.contains( "iconfile" ) )
        {
            bool ok = false;
            QString iconPath = QFileInfo( filePath() ).path() + "/"
                               + collectionInfo.value( "iconfile" ).toString();

            QPixmap iconPixmap;
            ok = iconPixmap.load( iconPath );
            if ( ok && !iconPixmap.isNull() )
                sc->setIcon( QIcon( iconPixmap ) );
        }

        Tomahawk::collection_ptr collection( sc );

        m_collections.insert( collection->name(), collection );
        emit collectionAdded( collection );

        if ( collectionInfo.contains( "iconurl" ) )
        {
            QString iconUrlString = collectionInfo.value( "iconurl" ).toString();
            if ( !iconUrlString.isEmpty() )
            {
                QUrl iconUrl = QUrl::fromEncoded( iconUrlString.toLatin1() );
                if ( iconUrl.isValid() )
                {
                    QNetworkRequest req( iconUrl );
                    tDebug() << "Creating a QNetworkReply with url:" << req.url().toString();
                    QNetworkReply* reply = TomahawkUtils::nam()->get( req );
                    reply->setProperty( "collectionName", collection->name() );

                    connect( reply, SIGNAL( finished() ),
                             this, SLOT( onCollectionIconFetched() ) );
                }
            }
        }

        //TODO: implement multiple collections from a resolver
    }
}


void
QtScriptResolver::onCollectionIconFetched()
{
    QNetworkReply* reply = qobject_cast< QNetworkReply* >( sender() );
    if ( reply != 0 )
    {
        Tomahawk::collection_ptr collection;
        collection = m_collections.value( reply->property( "collectionName" ).toString() );
        if ( !collection.isNull() )
        {
            if ( reply->error() == QNetworkReply::NoError )
            {
                QImageReader imageReader( reply );
                QPixmap collectionIcon = QPixmap::fromImageReader( &imageReader );

                if ( !collectionIcon.isNull() )
                    qobject_cast< Tomahawk::ScriptCollection* >( collection.data() )->setIcon( collectionIcon );
            }
        }
        reply->deleteLater();
    }
}


QVariantMap
QtScriptResolver::resolverSettings()
{
    return m_engine->mainFrame()->evaluateJavaScript( RESOLVER_LEGACY_CODE "if(resolver.settings) resolver.settings; else getSettings(); " ).toMap();
}


QVariantMap
QtScriptResolver::resolverUserConfig()
{
    return m_engine->mainFrame()->evaluateJavaScript( RESOLVER_LEGACY_CODE "resolver.getUserConfig();" ).toMap();
}


QVariantMap
QtScriptResolver::resolverInit()
{
    return m_engine->mainFrame()->evaluateJavaScript( RESOLVER_LEGACY_CODE "resolver.init();" ).toMap();
}


QVariantMap
QtScriptResolver::resolverCollections()
{
    return QVariantMap(); //TODO: add a way to distinguish collections
    // the resolver should provide a unique ID string for each collection, and then be queriable
    // against this ID. doesn't matter what kind of ID string as long as it's unique.
    // Then when there's callbacks from a resolver, it sends source name, collection id
    // + data.
}

