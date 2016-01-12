/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright (C) 2016  Dominik Schmidt <domme@tomahawk-player.org>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
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
#ifndef TOMAHAWK_UTILS_LINKPARSERPLUGIN_H
#define TOMAHAWK_UTILS_LINKPARSERPLUGIN_H

#include "../DllMacro.h"
#include "../Typedefs.h"
#include "UrlType.h"

namespace Tomahawk {

class ScriptJob;

namespace Utils {

class DLLEXPORT LinkParserPlugin : public QObject
{
Q_OBJECT

public:
    virtual ~LinkParserPlugin();

    virtual bool canParseUrl( const QString& url, Tomahawk::Utils::UrlType type ) const = 0;
    virtual void lookupUrl( const QString& url ) const = 0;

signals:
    void informationFound( const QString&, const QSharedPointer<QObject>& );
};

} // namespace Utils
} // namespace Tomahawk

#endif // TOMAHAWK_UTILS_LINKPARSERPLUGIN_H
