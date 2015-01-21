/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2014, Dominik Schmidt <domme@tomahawk-player.org>
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

#include "JSAccount.h"

#include "../utils/Json.h"
#include "../utils/Logger.h"
#include "ScriptEngine.h"
#include "ScriptJob.h"
#include "ScriptObject.h"
#include "JSResolver.h"

#include <QWebFrame>
#include <QFile>
#include <QThread>

using namespace Tomahawk;

JSAccount::JSAccount( const QString& name )
    : ScriptAccount( name )
    , m_engine( new ScriptEngine( this ) )
{
}


void
JSAccount::addToJavaScriptWindowObject( const QString& name, QObject* object )
{
    m_engine->mainFrame()->addToJavaScriptWindowObject( name, object );
}


void
JSAccount::setResolver( JSResolver* resolver )
{
    m_resolver = resolver;
}


void
JSAccount::scriptPluginFactory( const QString& type, const scriptobject_ptr& object )
{
    if ( type == "resolver" )
    {
        Q_ASSERT( m_resolver );
        m_resolver->m_scriptObject = object;
    }
    else
    {
        Tomahawk::ScriptAccount::scriptPluginFactory(type, object);
    }
}


QString
JSAccount::serializeQVariantMap( const QVariantMap& map )
{
    QVariantMap localMap = map;

    foreach( const QString& key, localMap.keys() )
    {
        QVariant currentVariant = localMap[ key ];

        // strip unserializable types - at least QJson needs this, check with QtJson
        if( currentVariant.canConvert<QImage>() )
        {
            localMap.remove( key );
        }
    }

    QByteArray serialized = TomahawkUtils::toJson( localMap );

    return QString( "JSON.parse('%1')" ).arg( JSAccount::escape( QString::fromUtf8( serialized ) ) );
}


QString
JSAccount::JSAccount::escape( const QString& source )
{
    QString copy = source;
    return copy.replace( "\\", "\\\\" ).replace( "'", "\\'" );
}


void
JSAccount::loadScript( const QString& path )
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
JSAccount::loadScripts( const QStringList& paths )
{
    foreach ( const QString& path, paths )
    {
        loadScript( path );
    }
}


void
JSAccount::startJob( ScriptJob* scriptJob )
{
    QString eval = QString(
        "Tomahawk.PluginManager.invoke("
        "'%1'," // requestId
        "'%2'," // objectId
        "'%3'," // methodName
        "%4"    // arguments
        ");"
    ).arg( scriptJob->id() )
    .arg( scriptJob->scriptObject()->id() )
    .arg( scriptJob->methodName() )
    .arg( serializeQVariantMap( scriptJob->arguments() ) );

    // Remove when new scripting api turned out to work reliably
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << eval;

    evaluateJavaScript( eval );
}


QVariant
JSAccount::syncInvoke( const scriptobject_ptr& scriptObject, const QString& methodName, const QVariantMap& arguments )
{
    QString eval = QString(
        "Tomahawk.PluginManager.invokeSync("
        "0, " // requestId
        "'%1'," // objectId
        "'%2'," // methodName
        "%3"    // arguments
        ");"
    ).arg( scriptObject->id() )
    .arg( methodName )
    .arg( serializeQVariantMap( arguments ) );

    // Remove when new scripting api turned out to work reliably
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << eval;

    return evaluateJavaScriptWithResult( eval );
}


QVariant
JSAccount::evaluateJavaScriptInternal( const QString& scriptSource )
{
    return m_engine->mainFrame()->evaluateJavaScript( scriptSource );
}


void
JSAccount::evaluateJavaScript( const QString& scriptSource )
{
    if ( QThread::currentThread() != thread() )
    {
        QMetaObject::invokeMethod( this, "evaluateJavaScript", Qt::QueuedConnection, Q_ARG( QString, scriptSource ) );
        return;
    }

    evaluateJavaScriptInternal( scriptSource );
}


QVariant
JSAccount::evaluateJavaScriptWithResult( const QString& scriptSource )
{
    Q_ASSERT( QThread::currentThread() == thread() );

    return evaluateJavaScriptInternal( scriptSource );
}
