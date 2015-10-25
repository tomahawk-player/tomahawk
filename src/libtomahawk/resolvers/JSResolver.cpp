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
#include "ScriptInfoPlugin.h"
#include "JSAccount.h"

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

    d->scriptAccount->addToJavaScriptWindowObject( "Tomahawk", d->resolverHelper );

    // add resolver
    d->scriptAccount->loadScript( filePath() );

    // HACK: register resolver object
    d->scriptAccount->evaluateJavaScript(
        "var resolverInstance = new (require('main').default);"
        "Tomahawk.PluginManager.registerPlugin('resolver', resolverInstance);"
    );

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
    if ( d->ready )
        Tomahawk::Pipeline::instance()->addResolver( this );
    else
        init();

    scriptAccount()->start();
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
                       .arg( JSAccount::escape( QString( url ) ) )
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
                   .arg( JSAccount::escape( QString( url ) ) );

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
                  .arg( JSAccount::escape( query->id() ) )
                  .arg( JSAccount::escape( query->queryTrack()->artist() ) )
                  .arg( JSAccount::escape( query->queryTrack()->album() ) )
                  .arg( JSAccount::escape( query->queryTrack()->track() ) );
    }
    else
    {
        eval = QString( "search( '%1', '%2' )" )
                  .arg( JSAccount::escape( query->id() ) )
                  .arg( JSAccount::escape( query->fullTextQuery() ) );
    }

    QVariantMap m = callOnResolver( eval ).toMap();
}


void
JSResolver::stop()
{
    Q_D( JSResolver );

    d->stopped = true;

    scriptAccount()->stop();

    Tomahawk::Pipeline::instance()->removeResolver( this );
    emit stopped();
}


void
JSResolver::loadUi()
{
    Q_D( JSResolver );

    QVariantMap m = callOnResolver( "getConfigUi()" ).toMap();

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
    callOnResolver( "saveUserConfig()" );
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


QVariant
JSResolver::callOnResolver( const QString& scriptSource )
{
    Q_D( JSResolver );

    QString propertyName = scriptSource.split('(').first();

    return d->scriptAccount->evaluateJavaScriptWithResult( QString(
        "if(resolverInstance['_adapter_%1']) {"
        "    resolverInstance._adapter_%2;"
        "} else {"
        "    resolverInstance.%2;"
        "}"
    ).arg( propertyName ).arg( scriptSource ) );
}
