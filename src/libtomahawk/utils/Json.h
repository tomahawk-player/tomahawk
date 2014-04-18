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
    /* QJson */
    DLLEXPORT QVariantMap qobject2qvariant( const QObject* object );
    DLLEXPORT void qvariant2qobject( const QVariantMap& variant, QObject* object );
    DLLEXPORT QVariant parseJson( const QByteArray& jsonData, bool* ok = 0 );
    DLLEXPORT QByteArray toJson( const QVariant &variant, bool* ok = 0 );
}

#endif // TOMAHAWKUTILS_JSON_H
