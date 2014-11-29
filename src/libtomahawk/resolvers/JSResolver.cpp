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

#include "JSResolver_p.h"

#include "accounts/AccountConfigWidget.h"
#include "network/Servent.h"
#include "jobview/JobStatusView.h"
#include "jobview/JobStatusModel.h"
#include "jobview/ErrorStatusMessage.h"
#include "utils/Logger.h"
#include "utils/NetworkAccessManager.h"
#include "utils/TomahawkUtilsGui.h"

#include "Artist.h"
#include "Album.h"
#include "config.h"
#include "JSResolverHelper.h"
#include "Pipeline.h"
#include "Result.h"
#include "ScriptCollection.h"
#include "ScriptEngine.h"
#include "SourceList.h"
#include "TomahawkSettings.h"
#include "TomahawkVersion.h"
#include "Track.h"
#include "JSInfoPlugin.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QMetaProperty>
#include <QTime>
#include <QWebFrame>

using namespace Tomahawk;

JSResolver::JSResolver( const QString& accountId, const QString& scriptPath, const QStringList& additionalScriptPaths )
    : Tomahawk::ExternalResolverGui( scriptPath )
    , d_ptr( new JSResolverPrivate( this, accountId, scriptPath, additionalScriptPaths ) )
{
    Q_D( JSResolver );
    tLog() << Q_FUNC_INFO << "Loading JS resolver:" << scriptPath;

    d->scriptPlugin = new JSPlugin();
    d->name = QFileInfo( filePath() ).baseName();

    // set the icon, if we launch properly we'll get the icon the resolver reports
    d->icon = TomahawkUtils::defaultPixmap( TomahawkUtils::DefaultResolver, TomahawkUtils::Original, QSize( 128, 128 ) );

    if ( !QFile::exists( filePath() ) )
    {
        tLog() << Q_FUNC_INFO << "Failed loading JavaScript resolver:" << scriptPath;
        d->error = Tomahawk::ExternalResolver::FileNotFound;
    }
    else
    {
        init();
    }
}


JSResolver::~JSResolver()
{
    Q_D( JSResolver );
    if ( !d->stopped )
        stop();

    delete d->scriptPlugin;
}


Tomahawk::ExternalResolver* JSResolver::factory( const QString& accountId, const QString& scriptPath, const QStringList& additionalScriptPaths )
{
    ExternalResolver* res = nullptr;

    const QFileInfo fi( scriptPath );
    if ( fi.suffix() == "js" || fi.suffix() == "script" )
    {
        res = new JSResolver( accountId, scriptPath, additionalScriptPaths );
        tLog() << Q_FUNC_INFO << scriptPath << "Loaded.";
    }

    return res;
}


Tomahawk::ExternalResolver::Capabilities
JSResolver::capabilities() const
{
    Q_D( const JSResolver );

    return d->capabilities;
}


QString
JSResolver::name() const
{
    Q_D( const JSResolver );

    return d->name;
}


QPixmap
JSResolver::icon() const
{
    Q_D( const JSResolver );

    return d->icon;
}


unsigned int
JSResolver::weight() const
{
    Q_D( const JSResolver );

    return d->weight;
}


unsigned int
JSResolver::timeout() const
{
    Q_D( const JSResolver );

    return d->timeout;
}


bool
JSResolver::running() const
{
    Q_D( const JSResolver );

    return d->ready && !d->stopped;
}


void
JSResolver::reload()
{
    Q_D( JSResolver );

    if ( QFile::exists( filePath() ) )
    {
        init();
        d->error = Tomahawk::ExternalResolver::NoError;
    } else
    {
        d->error = Tomahawk::ExternalResolver::FileNotFound;
    }
}


void
JSResolver::setIcon( const QPixmap& icon )
{
    Q_D( JSResolver );

    d->icon = icon;
}


void
JSResolver::init()
{
    Q_D( JSResolver );

    QString lucenePath = d->accountId + ".lucene";
    QDir luceneDir( TomahawkUtils::appDataDir().absoluteFilePath( lucenePath ) );
    if ( luceneDir.exists() )
    {
        d->fuzzyIndex.reset( new FuzzyIndex( this, lucenePath, false ) );
    }

    QFile scriptFile( filePath() );
    if ( !scriptFile.open( QIODevice::ReadOnly ) )
    {
        qWarning() << "Failed to read contents of file:" << filePath() << scriptFile.errorString();
        return;
    }
    const QByteArray scriptContents = scriptFile.readAll();

    // tomahawk.js
    {
        // add c++ part of tomahawk javascript library
        d->scriptPlugin->addToJavaScriptWindowObject( "Tomahawk", d->resolverHelper );

        // load es6-promises shim
        d->scriptPlugin->loadScript( RESPATH "js/es6-promise-2.0.0.min.js" );


        // Load CrytoJS core
        d->scriptPlugin->loadScript( RESPATH "js/cryptojs-core.js" );

        // Load CryptoJS modules
        QStringList jsfiles;
        jsfiles << "*.js";
        QDir cryptojs( RESPATH "js/cryptojs" );
        foreach ( QString jsfile, cryptojs.entryList( jsfiles ) )
        {
            d->scriptPlugin->loadScript( RESPATH "js/cryptojs/" +  jsfile );
        }

        // Load tomahawk.js
        d->scriptPlugin->loadScript( RESPATH "js/tomahawk.js" );
    }

    // tomahawk-infosystem.js
    {
        // add c++ part of tomahawk infosystem bindings as Tomahawk.InfoSystem
        d->infoSystemHelper = new JSInfoSystemHelper( d->scriptPlugin );
        d->scriptPlugin->addToJavaScriptWindowObject( "_TomahawkInfoSystem", d->infoSystemHelper );
        d->scriptPlugin->evaluateJavaScript( "Tomahawk.InfoSystem = _TomahawkInfoSystem;" );

        // add deps
        d->scriptPlugin->loadScripts( d->infoSystemHelper->requiredScriptPaths() );
    }

    // add resolver dependencies, if any
    d->scriptPlugin->loadScripts( d->requiredScriptPaths );


    // add resolver
    d->scriptPlugin->loadScript( filePath() );

    // init resolver
    resolverInit();

    QVariantMap m = resolverSettings();
    d->name    = m.value( "name" ).toString();
    d->weight  = m.value( "weight", 0 ).toUInt();
    d->timeout = m.value( "timeout", 25 ).toUInt() * 1000;
    bool compressed = m.value( "compressed", "false" ).toString() == "true";

    QByteArray icoData = QByteArray::fromBase64( m.value( "icon" ).toByteArray() );
    if ( compressed )
        icoData = qUncompress( icoData );
    QPixmap ico;
    ico.loadFromData( icoData );

    bool success = false;
    if ( !ico.isNull() )
    {
        d->icon = ico.scaled( d->icon.size(), Qt::IgnoreAspectRatio );
        success = true;
    }
    // see if the resolver sent an icon path to not break the old (unofficial) api.
    // TODO: remove this and publish a definitive api
    if ( !success )
    {
        QString iconPath = QFileInfo( filePath() ).path() + "/" + m.value( "icon" ).toString();
        success = d->icon.load( iconPath );
    }
    // if we still couldn't load the cover, set the default resolver icon
    if ( d->icon.isNull() )
    {
        d->icon = TomahawkUtils::defaultPixmap( TomahawkUtils::DefaultResolver, TomahawkUtils::Original, QSize( 128, 128 ) );
    }

    // load config widget and apply settings
    loadUi();
    QVariantMap config = resolverUserConfig();
    fillDataInWidgets( config );

    qDebug() << "JS" << filePath() << "READY," << "name" << d->name << "weight" << d->weight << "timeout" << d->timeout << "icon received" << success;

    d->ready = true;
}


void
JSResolver::start()
{
    Q_D( JSResolver );

    d->stopped = false;
    if ( d->ready )
        Tomahawk::Pipeline::instance()->addResolver( this );
    else
        init();
}


void
JSResolver::artists( const Tomahawk::collection_ptr& collection )
{
    if ( QThread::currentThread() != thread() )
    {
        QMetaObject::invokeMethod( this, "artists", Qt::QueuedConnection, Q_ARG( Tomahawk::collection_ptr, collection ) );
        return;
    }

    Q_D( const JSResolver );

    if ( !m_collections.contains( collection->name() ) || //if the collection doesn't belong to this resolver
         !d->capabilities.testFlag( Browsable ) )          //or this resolver doesn't even support collections
    {
        emit artistsFound( QList< Tomahawk::artist_ptr >() );
        return;
    }

    QString eval = QString( "artists( '%1' )" )
                   .arg( JSPlugin::escape( collection->name() ) );

    QVariantMap m = callOnResolver( eval ).toMap();
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
JSResolver::albums( const Tomahawk::collection_ptr& collection, const Tomahawk::artist_ptr& artist )
{
    if ( QThread::currentThread() != thread() )
    {
        QMetaObject::invokeMethod( this, "albums", Qt::QueuedConnection,
                                   Q_ARG( Tomahawk::collection_ptr, collection ),
                                   Q_ARG( Tomahawk::artist_ptr, artist ) );
        return;
    }

    Q_D( const JSResolver );

    if ( !m_collections.contains( collection->name() ) || //if the collection doesn't belong to this resolver
         !d->capabilities.testFlag( Browsable ) )          //or this resolver doesn't even support collections
    {
        emit albumsFound( QList< Tomahawk::album_ptr >() );
        return;
    }

    QString eval = QString( "albums( '%1', '%2' )" )
                   .arg( JSPlugin::escape( collection->name() ) )
                   .arg( JSPlugin::escape( artist->name() ) );

    QVariantMap m = callOnResolver( eval ).toMap();
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
JSResolver::tracks( const Tomahawk::collection_ptr& collection, const Tomahawk::album_ptr& album )
{
    if ( QThread::currentThread() != thread() )
    {
        QMetaObject::invokeMethod( this, "tracks", Qt::QueuedConnection,
                                   Q_ARG( Tomahawk::collection_ptr, collection ),
                                   Q_ARG( Tomahawk::album_ptr, album ) );
        return;
    }

    Q_D( const JSResolver );

    if ( !m_collections.contains( collection->name() ) || //if the collection doesn't belong to this resolver
         !d->capabilities.testFlag( Browsable ) )          //or this resolver doesn't even support collections
    {
        emit tracksFound( QList< Tomahawk::query_ptr >() );
        return;
    }

    QString eval = QString( "tracks( '%1', '%2', '%3' )" )
                   .arg( JSPlugin::escape( collection->name() ) )
                   .arg( JSPlugin::escape( album->artist()->name() ) )
                   .arg( JSPlugin::escape( album->name() ) );

    QVariantMap m = callOnResolver( eval ).toMap();
    if ( m.isEmpty() )
    {
        // if the resolver doesn't return anything, async api is used
        return;
    }

    QString errorMessage = tr( "Script Resolver Warning: API call %1 returned data synchronously." ).arg( eval );
    JobStatusView::instance()->model()->addJob( new ErrorStatusMessage( errorMessage ) );
    tDebug() << errorMessage << m;
}


bool
JSResolver::canParseUrl( const QString& url, UrlType type )
{
    Q_D( const JSResolver );

    // FIXME: How can we do this?
    /*if ( QThread::currentThread() != thread() )
    {
        QMetaObject::invokeMethod( this, "canParseUrl", Qt::QueuedConnection,
                                   Q_ARG( QString, url ) );
        return;
    }*/

    if ( d->capabilities.testFlag( UrlLookup ) )
    {
        QString eval = QString( "canParseUrl( '%1', %2 )" )
                       .arg( JSPlugin::escape( QString( url ) ) )
                       .arg( (int) type );
        return callOnResolver( eval ).toBool();
    }
    else
    {
        // We cannot do URL lookup.
        return false;
    }
}


void
JSResolver::lookupUrl( const QString& url )
{
    if ( QThread::currentThread() != thread() )
    {
        QMetaObject::invokeMethod( this, "lookupUrl", Qt::QueuedConnection,
                                   Q_ARG( QString, url ) );
        return;
    }

    Q_D( const JSResolver );

    if ( !d->capabilities.testFlag( UrlLookup ) )
    {
        emit informationFound( url, QSharedPointer<QObject>() );
        return;
    }

    QString eval = QString( "lookupUrl( '%1' )" )
                   .arg( JSPlugin::escape( QString( url ) ) );

    QVariantMap m = callOnResolver( eval ).toMap();
    if ( m.isEmpty() )
    {
        // if the resolver doesn't return anything, async api is used
        return;
    }

    QString errorMessage = tr( "Script Resolver Warning: API call %1 returned data synchronously." ).arg( eval );
    JobStatusView::instance()->model()->addJob( new ErrorStatusMessage( errorMessage ) );
    tDebug() << errorMessage << m;
}


QVariant
JSPlugin::evaluateJavaScriptInternal( const QString& scriptSource )
{
    return m_engine->mainFrame()->evaluateJavaScript( scriptSource );
}


void
JSPlugin::evaluateJavaScript( const QString& scriptSource )
{
    if ( QThread::currentThread() != thread() )
    {
        QMetaObject::invokeMethod( this, "evaluateJavaScript", Qt::QueuedConnection, Q_ARG( QString, scriptSource ) );
        return;
    }

    evaluateJavaScriptInternal( scriptSource );
}


QVariant
JSPlugin::evaluateJavaScriptWithResult( const QString& scriptSource )
{
    Q_ASSERT( QThread::currentThread() == thread() );

    return evaluateJavaScriptInternal( scriptSource );
}


Tomahawk::ExternalResolver::ErrorState
JSResolver::error() const
{
    Q_D( const JSResolver );

    return d->error;
}


void
JSResolver::resolve( const Tomahawk::query_ptr& query )
{
    if ( QThread::currentThread() != thread() )
    {
        QMetaObject::invokeMethod( this, "resolve", Qt::QueuedConnection, Q_ARG(Tomahawk::query_ptr, query) );
        return;
    }

    QString eval;
    if ( !query->isFullTextQuery() )
    {
        eval = QString( "resolve( '%1', '%2', '%3', '%4' )" )
                  .arg( JSPlugin::escape( query->id() ) )
                  .arg( JSPlugin::escape( query->queryTrack()->artist() ) )
                  .arg( JSPlugin::escape( query->queryTrack()->album() ) )
                  .arg( JSPlugin::escape( query->queryTrack()->track() ) );
    }
    else
    {
        eval = QString( "search( '%1', '%2' )" )
                  .arg( JSPlugin::escape( query->id() ) )
                  .arg( JSPlugin::escape( query->fullTextQuery() ) );
    }

    QVariantMap m = callOnResolver( eval ).toMap();
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
JSResolver::parseResultVariantList( const QVariantList& reslist )
{
    QList< Tomahawk::result_ptr > results;

    foreach( const QVariant& rv, reslist )
    {
        QVariantMap m = rv.toMap();
        // TODO we need to handle preview urls separately. they should never trump a real url, and we need to display
        // the purchaseUrl for the user to upgrade to a full stream.
        if ( m.value( "preview" ).toBool() == true )
            continue;

        int duration = m.value( "duration", 0 ).toInt();
        if ( duration <= 0 && m.contains( "durationString" ) )
        {
            QTime time = QTime::fromString( m.value( "durationString" ).toString(), "hh:mm:ss" );
            duration = time.secsTo( QTime( 0, 0 ) ) * -1;
        }

        Tomahawk::track_ptr track = Tomahawk::Track::get( m.value( "artist" ).toString(),
                                                          m.value( "track" ).toString(),
                                                          m.value( "album" ).toString(),
                                                          m.value( "albumArtist" ).toString(),
                                                          duration,
                                                          QString(),
                                                          m.value( "albumpos" ).toUInt(),
                                                          m.value( "discnumber" ).toUInt() );
        if ( !track )
            continue;

        Tomahawk::result_ptr rp = Tomahawk::Result::get( m.value( "url" ).toString(), track );
        if ( !rp )
            continue;

        rp->setBitrate( m.value( "bitrate" ).toUInt() );
        rp->setSize( m.value( "size" ).toUInt() );
        rp->setRID( uuid() );
        rp->setFriendlySource( name() );
        rp->setPurchaseUrl( m.value( "purchaseUrl" ).toString() );
        rp->setLinkUrl( m.value( "linkUrl" ).toString() );
        rp->setScore( m.value( "score" ).toFloat() );
        rp->setChecked( m.value( "checked" ).toBool() );

        //FIXME
        if ( m.contains( "year" ) )
        {
            QVariantMap attr;
            attr[ "releaseyear" ] = m.value( "year" );
//            rp->track()->setAttributes( attr );
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
JSResolver::parseArtistVariantList( const QVariantList& reslist )
{
    QList< Tomahawk::artist_ptr > results;

    foreach( const QVariant& rv, reslist )
    {
        const QString val = rv.toString();
        if ( val.trimmed().isEmpty() )
            continue;

        Tomahawk::artist_ptr ap = Tomahawk::Artist::get( val, false );

        results << ap;
    }

    return results;
}


QList< Tomahawk::album_ptr >
JSResolver::parseAlbumVariantList( const Tomahawk::artist_ptr& artist, const QVariantList& reslist )
{
    QList< Tomahawk::album_ptr > results;

    foreach( const QVariant& rv, reslist )
    {
        const QString val = rv.toString();
        if ( val.trimmed().isEmpty() )
            continue;

        Tomahawk::album_ptr ap = Tomahawk::Album::get( artist, val, false );

        results << ap;
    }

    return results;
}


void
JSResolver::stop()
{
    Q_D( JSResolver );

    d->stopped = true;

    foreach ( const Tomahawk::collection_ptr& collection, m_collections )
    {
        emit collectionRemoved( collection );
    }

    Tomahawk::Pipeline::instance()->removeResolver( this );
    emit stopped();
}


void
JSResolver::loadUi()
{
    Q_D( JSResolver );

    QVariantMap m = callOnResolver( "getConfigUi()" ).toMap();
    d->dataWidgets = m["fields"].toList();

    bool compressed = m.value( "compressed", "false" ).toBool();
    qDebug() << "Resolver has a preferences widget! compressed?" << compressed;

    QByteArray uiData = QByteArray::fromBase64( m[ "widget" ].toByteArray() );
    if ( compressed )
        uiData = qUncompress(  uiData );

    QVariantMap images;
    foreach(const QVariant& item, m[ "images" ].toList())
    {
        const QVariantMap m = item.toMap();
        QString key = m.keys().first();
        QVariant value = m.value(key);
        images[key] = value;
    }

    if ( m.contains( "images" ) )
        uiData = fixDataImagePaths( uiData, compressed, images );

    d->configWidget = QPointer< AccountConfigWidget >( widgetFromData( uiData, 0 ) );

    emit changed();
}


AccountConfigWidget*
JSResolver::configUI() const
{
    Q_D( const JSResolver );

    if ( d->configWidget.isNull() )
        return 0;
    else
        return d->configWidget.data();
}


void
JSResolver::saveConfig()
{
    Q_D( JSResolver );

    QVariant saveData = loadDataFromWidgets();
//    qDebug() << Q_FUNC_INFO << saveData;

    d->resolverHelper->setResolverConfig( saveData.toMap() );
    callOnResolver( "saveUserConfig()" );
}


QVariant
JSResolver::widgetData( QWidget* widget, const QString& property )
{
    for ( int i = 0; i < widget->metaObject()->propertyCount(); i++ )
    {
        if ( widget->metaObject()->property( i ).name() == property )
        {
            return widget->property( property.toLatin1() );
        }
    }

    return QVariant();
}


void
JSResolver::setWidgetData( const QVariant& value, QWidget* widget, const QString& property )
{
    const QMetaObject *metaObject = widget->metaObject();
    for ( int i = 0; i < metaObject->propertyCount(); i++ )
    {
        const QMetaProperty &prop = metaObject->property( i );
        if ( prop.name() == property )
        {
            prop.write( widget, value );
            return;
        }
    }
}


QVariantMap
JSResolver::loadDataFromWidgets()
{
    Q_D( JSResolver );

    QVariantMap saveData;
    foreach( const QVariant& dataWidget, d->dataWidgets )
    {
        QVariantMap data = dataWidget.toMap();

        QString widgetName = data["widget"].toString();
        QWidget* widget= d->configWidget.data()->findChild<QWidget*>( widgetName );

        QVariant value = widgetData( widget, data["property"].toString() );

        saveData[ data["name"].toString() ] = value;
    }

    return saveData;
}


void
JSResolver::fillDataInWidgets( const QVariantMap& data )
{
    Q_D( JSResolver );

    foreach(const QVariant& dataWidget, d->dataWidgets)
    {
        const QVariantMap m = dataWidget.toMap();
        QString widgetName = m["widget"].toString();
        QWidget* widget= d->configWidget.data()->findChild<QWidget*>( widgetName );
        if ( !widget )
        {
            tLog() << Q_FUNC_INFO << "Widget specified in resolver was not found:" << widgetName;
            Q_ASSERT(false);
            return;
        }

        QString propertyName = m["property"].toString();
        QString name = m["name"].toString();

        setWidgetData( data[ name ], widget, propertyName );
    }
}


void
JSResolver::onCapabilitiesChanged( Tomahawk::ExternalResolver::Capabilities capabilities )
{
    Q_D( JSResolver );

    d->capabilities = capabilities;
    loadCollections();
}


void
JSResolver::loadCollections()
{
    Q_D( JSResolver );

    if ( d->capabilities.testFlag( Browsable ) )
    {
        const QVariantMap collectionInfo =  callOnResolver( "collection()" ).toMap();
        if ( collectionInfo.isEmpty() ||
             !collectionInfo.contains( "prettyname" ) ||
             !collectionInfo.contains( "description" ) )
            return;

        const QString prettyname = collectionInfo.value( "prettyname" ).toString();
        const QString desc = collectionInfo.value( "description" ).toString();

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
            QString iconPath = QFileInfo( filePath() ).path() + "/"
                               + collectionInfo.value( "iconfile" ).toString();

            QPixmap iconPixmap;
            bool ok = iconPixmap.load( iconPath );
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
                    QNetworkReply* reply = Tomahawk::Utils::nam()->get( req );
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
JSResolver::onCollectionIconFetched()
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
JSResolver::resolverSettings()
{
    return callOnResolver( "settings" ).toMap();
}


QVariantMap
JSResolver::resolverUserConfig()
{
    return callOnResolver( "getUserConfig()" ).toMap();
}


QVariantMap
JSResolver::resolverInit()
{
    return callOnResolver( "init()" ).toMap();
}


QVariantMap
JSResolver::resolverCollections()
{
    return QVariantMap(); //TODO: add a way to distinguish collections
    // the resolver should provide a unique ID string for each collection, and then be queriable
    // against this ID. doesn't matter what kind of ID string as long as it's unique.
    // Then when there's callbacks from a resolver, it sends source name, collection id
    // + data.
}


void
JSPlugin::loadScript( const QString& path )
{
    QFile file( path );

    if ( !file.open( QIODevice::ReadOnly ) )
    {
        qWarning() << "Failed to read contents of file:" << path << file.errorString();
        Q_ASSERT(false);
        return;
    }

    const QByteArray contents = file.readAll();

    m_engine->setScriptPath( path );
    m_engine->mainFrame()->evaluateJavaScript( contents );

    file.close();
}


void
JSPlugin::loadScripts( const QStringList& paths )
{
    foreach ( const QString& path, paths )
    {
        loadScript( path );
    }
}


QVariant
JSResolver::callOnResolver( const QString& scriptSource )
{
    Q_D( JSResolver );

    QString propertyName = scriptSource.split('(').first();

    tLog() << "JAVASCRIPT: run: " << scriptSource;

    return d->scriptPlugin->evaluateJavaScriptWithResult( QString(
        "if(Tomahawk.resolver.instance['_adapter_%1']) {"
        "    Tomahawk.resolver.instance._adapter_%2;"
        "} else {"
        "    Tomahawk.resolver.instance.%2"
        "}"
    ).arg( propertyName ).arg( scriptSource ) );
}


QString
JSPlugin::JSPlugin::escape( const QString& source )
{
    QString copy = source;
    return copy.replace( "\\", "\\\\" ).replace( "'", "\\'" );
}
