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

#include "JSPlugin.h"

#include "../utils/Json.h"
#include "../utils/Logger.h"
#include "ScriptEngine.h"

#include <QWebFrame>
#include <QFile>


using namespace Tomahawk;

JSPlugin::JSPlugin()
    : m_engine( new ScriptEngine( this ) )
{
}


void
JSPlugin::addToJavaScriptWindowObject( const QString& name, QObject* object )
{
    m_engine->mainFrame()->addToJavaScriptWindowObject( name, object );
}


QString
JSPlugin::serializeQVariantMap( const QVariantMap& map )
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

    return QString( "JSON.parse('%1')" ).arg( JSPlugin::escape( QString::fromUtf8( serialized ) ) );
}


QString
JSPlugin::JSPlugin::escape( const QString& source )
{
    QString copy = source;
    return copy.replace( "\\", "\\\\" ).replace( "'", "\\'" );
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
