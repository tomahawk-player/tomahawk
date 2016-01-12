/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2013,      Teo Mrnjavac <teo@kde.org>
 *   Copyright 2013,      Uwe L. Korn <uwelk@xhochy.com>
 *   Copyright 2016,      Dominik Schmidt <domme@tomahawk-player.org>
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
#include "ScriptInfoPlugin.h"
#include "JSAccount.h"
#include "ScriptJob.h"
#include "database/Database.h"
#include "database/DatabaseImpl.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QMetaProperty>
#include <QWebFrame>

using namespace Tomahawk;

JSResolver::JSResolver( const QString& accountId, const QString& scriptPath, const QStringList& additionalScriptPaths )
    : Tomahawk::ExternalResolverGui( scriptPath )
    , ScriptPlugin( scriptobject_ptr() )
    , d_ptr( new JSResolverPrivate( this, accountId, scriptPath, additionalScriptPaths ) )
{
    Q_D( JSResolver );
    tLog() << Q_FUNC_INFO << "Loading JS resolver:" << scriptPath;

    d->name = QFileInfo( filePath() ).baseName();
    d->scriptAccount.reset( new JSAccount( d->name ) );
    d->scriptAccount->setResolver( this );
    d->scriptAccount->setFilePath( filePath() );
    d->scriptAccount->setIcon( icon( QSize( 0, 0 ) ) );

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
JSResolver::icon( const QSize& size ) const
{
    Q_D( const JSResolver );

    if ( !size.isEmpty() )
        return d->icon.scaled( size, Qt::KeepAspectRatio, Qt::SmoothTransformation );

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
        d->scriptAccount->addToJavaScriptWindowObject( "Tomahawk", d->resolverHelper );

        // load es6-promises shim
        d->scriptAccount->loadScript( RESPATH "js/rsvp-latest.min.js" );


        // Load CrytoJS core
        d->scriptAccount->loadScript( RESPATH "js/cryptojs-core.js" );

        // Load CryptoJS modules
        QStringList jsfiles;
        jsfiles << "*.js";
        QDir cryptojs( RESPATH "js/cryptojs" );
        foreach ( QString jsfile, cryptojs.entryList( jsfiles ) )
        {
            d->scriptAccount->loadScript( RESPATH "js/cryptojs/" +  jsfile );
        }

        // Load tomahawk.js
        d->scriptAccount->loadScript( RESPATH "js/tomahawk.js" );
    }

    // tomahawk-infosystem.js
    {
        // TODO: be smarter about this, only include this if the resolver supports infoplugins

        // add deps
        d->scriptAccount->loadScript( RESPATH "js/tomahawk-infosystem.js" );
    }

    // add resolver dependencies, if any
    d->scriptAccount->loadScripts( d->requiredScriptPaths );


    // add resolver
    d->scriptAccount->loadScript( filePath() );

    // HACK: register resolver object
    d->scriptAccount->evaluateJavaScript( "Tomahawk.PluginManager.registerPlugin('resolver', Tomahawk.resolver.instance);" );
    // init resolver
    scriptObject()->syncInvoke( "init" );

    QVariantMap m = scriptObject()->syncInvoke( "settings" ).toMap();
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
    if ( !d->configWidget.isNull() )
    {
        d->configWidget->fillDataInWidgets( resolverUserConfig() );
    }

    qDebug() << "JS" << filePath() << "READY," << "name" << d->name << "weight" << d->weight << "timeout" << d->timeout << "icon received" << success;

    d->ready = true;
}


void
JSResolver::start()
{
    Q_D( JSResolver );

    d->stopped = false;
    d->resolverHelper->start();

    if ( d->ready )
        Tomahawk::Pipeline::instance()->addResolver( this );
    else
        init();

    scriptAccount()->start();
}


void
JSResolver::tracksAdded( const QList<query_ptr>&, const ModelMode, const collection_ptr&)
{
    // Check if we still are actively waiting
    if ( m_pendingAlbum.isNull() || m_pendingUrl.isNull() )
        return;

    emit informationFound( m_pendingUrl, m_pendingAlbum.objectCast<QObject>() );
    m_pendingAlbum = album_ptr();
    m_pendingUrl = QString();
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
    ScriptJob* job = scriptAccount()->resolve( scriptObject(), query, "resolver" );

    connect( job, SIGNAL( done( QVariantMap ) ), SLOT( onResolveRequestDone( QVariantMap ) ) );

    job->start();
}

void
JSResolver::onResolveRequestDone( const QVariantMap& data )
{
    Q_ASSERT( QThread::currentThread() == thread() );
    Q_D( JSResolver );

    ScriptJob* job = qobject_cast< ScriptJob* >( sender() );

    QID qid = job->property( "qid" ).toString();

    if ( job->error() )
    {
        Tomahawk::Pipeline::instance()->reportError( qid, this );
    }
    else
    {

        QList< Tomahawk::result_ptr > results = scriptAccount()->parseResultVariantList( data.value( "tracks" ).toList() );

        foreach( const result_ptr& result, results )
        {
            result->setResolvedByResolver( this );
            result->setFriendlySource( name() );
        }

        Tomahawk::Pipeline::instance()->reportResults( qid, this, results );
    }

    sender()->deleteLater();
}

void
JSResolver::stop()
{
    Q_D( JSResolver );

    d->stopped = true;
    d->resolverHelper->stop();

    scriptAccount()->stop();

    Tomahawk::Pipeline::instance()->removeResolver( this );
    emit stopped();
}


void
JSResolver::loadUi()
{
    Q_D( JSResolver );

    QVariantMap m = scriptObject()->syncInvoke( "getConfigUi" ).toMap();

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
    if ( !d->configWidget.isNull() )
    {
        d->configWidget->setDataWidgets( m["fields"].toList() );
    }

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
    scriptObject()->syncInvoke( "saveUserConfig" );
}


QVariantMap
JSResolver::loadDataFromWidgets()
{
    Q_D( JSResolver );

    return d->configWidget->readData();
}


ScriptAccount*
JSResolver::scriptAccount() const
{
    Q_D( const JSResolver );

    return d->scriptAccount.get();
}


void
JSResolver::onCapabilitiesChanged( Tomahawk::ExternalResolver::Capabilities capabilities )
{
    Q_D( JSResolver );

    d->capabilities = capabilities;
}


QVariantMap
JSResolver::resolverUserConfig()
{
    return scriptObject()->syncInvoke( "getUserConfig" ).toMap();
}


ScriptJob*
JSResolver::getStreamUrl( const result_ptr& result )
{
    QVariantMap arguments;
    arguments["url"] = result->url();

    return scriptObject()->invoke( "getStreamUrl", arguments );
}
