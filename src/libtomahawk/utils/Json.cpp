/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2014, Uwe L. Korn <uwelk@xhochy.com>
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

#include "Json.h"

// Qt version specific includes
#if QT_VERSION >= QT_VERSION_CHECK( 5, 0, 0 )
    #include <QUrlQuery>
    #include <QJsonDocument>
    #include <QMetaProperty>
#else
    #include <qjson/parser.h>
    #include <qjson/qobjecthelper.h>
    #include <qjson/serializer.h>
#endif

namespace TomahawkUtils
{

QVariantMap
qobject2qvariant( const QObject* object )
{
#if QT_VERSION >= QT_VERSION_CHECK( 5, 0, 0 )
    QVariantMap map;
    if ( object == NULL )
    {
        return map;
    }

    const QMetaObject* metaObject = object->metaObject();
    for ( int i = 0; i < metaObject->propertyCount(); ++i )
    {
        QMetaProperty metaproperty = metaObject->property( i );
        if ( metaproperty.isReadable() )
        {
            map[ QLatin1String( metaproperty.name() ) ] = object->property( metaproperty.name() );
        }
    }
    return map;
#else
    return QJson::QObjectHelper::qobject2qvariant( object );
#endif
}


void
qvariant2qobject( const QVariantMap& variant, QObject* object )
{
#if QT_VERSION >= QT_VERSION_CHECK( 5, 0, 0 )
    for ( QVariantMap::const_iterator iter = variant.begin(); iter != variant.end(); ++iter )
    {
        QVariant property = object->property( iter.key().toLatin1() );
        Q_ASSERT( property.isValid() );
        if ( property.isValid() )
        {
            QVariant value = iter.value();
            if ( value.canConvert( property.type() ) )
            {
                value.convert( property.type() );
                object->setProperty( iter.key().toLatin1(), value );
            } else if ( QString( QLatin1String("QVariant") ).compare( QLatin1String( property.typeName() ) ) == 0 ) {
                object->setProperty( iter.key().toLatin1(), value );
            }
        }
    }
#else
    QJson::QObjectHelper::qvariant2qobject( variant, object );
#endif
}


QVariant
parseJson( const QByteArray& jsonData, bool* ok )
{
#if QT_VERSION >= QT_VERSION_CHECK( 5, 0, 0 )
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson( jsonData, &error );
    if ( ok != NULL )
    {
        *ok = ( error.error == QJsonParseError::NoError );
    }
    return doc.toVariant();
#else
    QJson::Parser p;
    return p.parse( jsonData, ok );
#endif
}


QByteArray
toJson( const QVariant &variant, bool* ok )
{
#if QT_VERSION >= QT_VERSION_CHECK( 5, 0, 0 )
    QJsonDocument doc = QJsonDocument::fromVariant( variant );
    if ( ok != NULL )
    {
        *ok = true;
    }
    return doc.toJson();
#else
    QJson::Serializer serializer;
    return serializer.serialize( variant, ok );
#endif
}

}
