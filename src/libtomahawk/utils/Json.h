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

#pragma once
#ifndef TOMAHAWKUTILS_JSON_H
#define TOMAHAWKUTILS_JSON_H

#include "DllMacro.h"

#include <QVariant>

namespace TomahawkUtils
{
     /**
     * Convert a QObject instance to a QVariantMap by adding its properties
     * as key-value pairs.
     *
     * @param object Object that shall be "serialised"
     * @return All properties of the object stored as QVariantMap
     */
    DLLEXPORT QVariantMap qobject2qvariant( const QObject* object );

    /**
     * Write out all key-value pairs into the respective properties of the
     * given object.
     *
     * @param variant The key-value pairs that shall be stored in the object.
     * @param object The destiation object where we store the key-value pairs of the map as properties.
     */
    DLLEXPORT void qvariant2qobject( const QVariantMap& variant, QObject* object );

    /**
     * Parse the JSON string and return the result as a QVariant.
     *
     * @param jsonData The string containing the data as JSON.
     * @param ok Set to true if the conversion was successful, otherwise false.
     * @return After a successful conversion the parsed data either as QVariantMap or QVariantList.
     */
    DLLEXPORT QVariant parseJson( const QByteArray& jsonData, bool* ok = 0 );

   /**
     * Convert a QVariant to a JSON representation.
     *
     * This function will accept Strings, Number, QVariantList and QVariantMaps
     * as input types. Although Qt5's JSON implementation itself does not
     * support the serialisation of QVariantHash, we will convert a QVariantHash
     * to a QVariantMap but it is suggest to convert all QVariantHash to
     * QVariantMap in your code than passing them here.
     *
     * @param variant The data to be serialised.
     * @param ok Set to true if the conversion was successful, otherwise false.
     * @return After a successful serialisation the data of the QVariant represented as JSON.
     */
    DLLEXPORT QByteArray toJson( const QVariant &variant, bool* ok = 0 );
}

#endif // TOMAHAWKUTILS_JSON_H
