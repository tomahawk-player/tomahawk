/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "qtscriptresolver.h"

#include "artist.h"
#include "album.h"
#include "pipeline.h"
#include "sourcelist.h"
#include "utils/tomahawkutils.h"

#include <QMetaProperty>


QtScriptResolverHelper::QtScriptResolverHelper( const QString& scriptPath, QObject* parent )
    : QObject( parent )
{
    m_scriptPath = scriptPath;
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
QtScriptResolverHelper::readCompressed(const QString& fileName)
{
    return compress( readRaw( fileName ) );
}


QString
QtScriptResolverHelper::readBase64(const QString& fileName)
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
QtScriptResolverHelper::setResolverConfig( QVariantMap config )
{
    m_resolverConfig = config;
}


QtScriptResolver::QtScriptResolver( const QString& scriptPath )
    : Tomahawk::ExternalResolver( scriptPath )
    , m_ready( false )
    , m_stopped( false )
    , m_resolverHelper( new QtScriptResolverHelper( scriptPath, this ) )
{
    qDebug() << Q_FUNC_INFO << scriptPath;

    m_engine = new ScriptEngine( this );
    QFile scriptFile( scriptPath );
    if ( !scriptFile.open( QIODevice::ReadOnly ) )
    {
        qDebug() << Q_FUNC_INFO << "Failed loading JavaScript resolver:" << scriptPath;
        deleteLater();
        return;
    }

    m_engine->mainFrame()->setHtml( "<html><body></body></html>" );
    m_engine->mainFrame()->addToJavaScriptWindowObject( "Tomahawk", m_resolverHelper );
    m_engine->mainFrame()->evaluateJavaScript( scriptFile.readAll() );
    scriptFile.close();

    QVariantMap m = resolverSettings();
    m_name       = m.value( "name" ).toString();
    m_weight     = m.value( "weight", 0 ).toUInt();
    m_timeout    = m.value( "timeout", 25 ).toUInt() * 1000;

    // load config widget and apply settings
    loadUi();
    QVariantMap config =  resolverUserConfig();
    fillDataInWidgets( config );

    qDebug() << Q_FUNC_INFO << m_name << m_weight << m_timeout;

    m_ready = true;
    Tomahawk::Pipeline::instance()->addResolver( this );
}


QtScriptResolver::~QtScriptResolver()
{
    Tomahawk::Pipeline::instance()->removeResolver( this );
    delete m_engine;
}


void
QtScriptResolver::resolve( const Tomahawk::query_ptr& query )
{
    if ( QThread::currentThread() != thread() )
    {
//        qDebug() << "Reinvoking in correct thread:" << Q_FUNC_INFO;
        QMetaObject::invokeMethod( this, "resolve", Qt::QueuedConnection, Q_ARG(Tomahawk::query_ptr, query) );
        return;
    }

//    qDebug() << Q_FUNC_INFO << query->toString();
    QString eval;

    if ( !query->isFullTextQuery() )
    {
        eval = QString( "Tomahawk.resolver.instance.resolve( '%1', '%2', '%3', '%4' );" )
            .arg( query->id().replace( "'", "\\'" ) )
            .arg( query->artist().replace( "'", "\\'" ) )
            .arg( query->album().replace( "'", "\\'" ) )
            .arg( query->track().replace( "'", "\\'" ) );
    }
    else
    {
        eval = QString( "Tomahawk.resolver.instance.resolve( '%1', '%2', '%3', '%4' );" )
            .arg( query->id().replace( "'", "\\'" ) )
            .arg( query->fullTextQuery().replace( "'", "\\'" ) )
            .arg( QString() )
            .arg( QString() );
    }

    QList< Tomahawk::result_ptr > results;

    QVariantMap m = m_engine->mainFrame()->evaluateJavaScript( eval ).toMap();
    qDebug() << "JavaScript Result:" << m;

    const QString qid = query->id();
    const QVariantList reslist = m.value( "results" ).toList();

    foreach( const QVariant& rv, reslist )
    {
        QVariantMap m = rv.toMap();

        Tomahawk::result_ptr rp( new Tomahawk::Result() );
        Tomahawk::artist_ptr ap = Tomahawk::Artist::get( 0, m.value( "artist" ).toString() );
        rp->setArtist( ap );
        rp->setAlbum( Tomahawk::Album::get( 0, m.value( "album" ).toString(), ap ) );
        rp->setTrack( m.value( "track" ).toString() );
        rp->setBitrate( m.value( "bitrate" ).toUInt() );
        rp->setUrl( m.value( "url" ).toString() );
        rp->setSize( m.value( "size" ).toUInt() );
        rp->setScore( m.value( "score" ).toFloat() * ( (float)weight() / 100.0 ) );
        rp->setRID( uuid() );
        rp->setFriendlySource( name() );

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

        results << rp;
    }

    Tomahawk::Pipeline::instance()->reportResults( qid, results );
}


void
QtScriptResolver::stop()
{
    m_stopped = true;
    emit finished();
}


void
QtScriptResolver::loadUi()
{
    qDebug() << Q_FUNC_INFO;

    QVariantMap m = m_engine->mainFrame()->evaluateJavaScript( "Tomahawk.resolver.instance.getConfigUi();" ).toMap();
    m_dataWidgets = m["fields"].toList();


    bool compressed = m.value( "compressed", "false" ).toBool();
    qDebug() << "Resolver has a preferences widget! compressed?" << compressed << m;

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



    m_configWidget = QWeakPointer< QWidget >( widgetFromData( uiData, 0 ) );

    emit changed();
}


QWidget*
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
    qDebug() << Q_FUNC_INFO << saveData;

    m_resolverHelper->setResolverConfig( saveData.toMap() );
    m_engine->mainFrame()->evaluateJavaScript( "Tomahawk.resolver.instance.saveUserConfig();" );
}


QWidget*
QtScriptResolver::findWidget(QWidget* widget, const QStringList& widgetPath)
{
    if( !widget || !widget->isWidgetType() )
        return 0;

    qDebug() << Q_FUNC_INFO << widget->objectName() << widgetPath;

    if( widgetPath.isEmpty() )
        return widget;

    QString searchName = widgetPath.first();

    foreach( QObject* child, widget->children() )
    {
        if( child->isWidgetType() && child->objectName() == searchName )
        {
            QStringList newWidgetPath = widgetPath;
            newWidgetPath.removeFirst();
            return findWidget(qobject_cast< QWidget* >( child ), newWidgetPath);
        }
    }

    return 0;
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

        QStringList widgetPath;
        foreach(const QVariant& pathItem, data["widget"].toList())
        {
            widgetPath << pathItem.toString();
        }

        QWidget* widget= findWidget( m_configWidget.data(), widgetPath );

        QString value = widgetData( widget, data["property"].toString() ).toString();

        saveData[ data["name"].toString() ] = value;
    }

    qDebug() << saveData;

    return saveData;
}


void
QtScriptResolver::fillDataInWidgets( const QVariantMap& data )
{
    qDebug() << Q_FUNC_INFO << data;
    foreach(const QVariant& dataWidget, m_dataWidgets)
    {
        QStringList widgetPath;
        foreach(const QVariant& pathItem, dataWidget.toMap()["widget"].toList())
        {
            widgetPath << pathItem.toString();
        }

        QWidget* widget= findWidget( m_configWidget.data(), widgetPath );
        if( !widget )
        {
            qDebug() << Q_FUNC_INFO << "widget specified in resolver was not found:" << widgetPath;
            Q_ASSERT(false);
            return;
        }

        QString propertyName = dataWidget.toMap()["property"].toString();
        QString name = dataWidget.toMap()["name"].toString();

        setWidgetData( data[ name ], widget, propertyName );
    }
}


QVariantMap
QtScriptResolver::resolverSettings()
{
    return m_engine->mainFrame()->evaluateJavaScript( "Tomahawk.resolver.instance.getSettings();" ).toMap();
}


QVariantMap
QtScriptResolver::resolverUserConfig()
{
    return m_engine->mainFrame()->evaluateJavaScript( "Tomahawk.resolver.instance.getUserConfig();" ).toMap();
}
