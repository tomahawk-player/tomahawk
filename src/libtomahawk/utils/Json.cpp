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

#include <QJsonDocument>
#include <QMetaProperty>
#include <QVariantHash>

namespace TomahawkUtils
{

QVariantMap
qobject2qvariant( const QObject* object )
{
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
}


void
qvariant2qobject( const QVariantMap& variant, QObject* object )
{
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
}


QVariant
parseJson( const QByteArray& jsonData, bool* ok )
{
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson( jsonData, &error );
    if ( ok != NULL )
    {
        *ok = ( error.error == QJsonParseError::NoError );
    }
    return doc.toVariant();
}


QByteArray
toJson( const QVariant &variant, bool* ok )
{
    QVariant _variant = variant;
    if ( variant.type() == QVariant::Hash )
    {
        // QJsonDocument cannot deal with QVariantHash, so convert.
        const QVariantHash hash = variant.toHash();
        QVariantMap map;
        QHashIterator<QString, QVariant> it(hash);
        while ( it.hasNext() )
        {
            it.next();
            map.insert( it.key(), it.value() );
        }
        _variant = map;
    }

    QJsonDocument doc = QJsonDocument::fromVariant( _variant );
    if ( ok != NULL )
    {
        *ok = !doc.isNull();
    }
    return doc.toJson( QJsonDocument::Compact );
}

}
