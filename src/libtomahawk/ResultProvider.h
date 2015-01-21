/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright (C) 2015  Dominik Schmidt <domme@tomahawk-player.org>
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
#ifndef TOMAHAWK_RESULTPROVIDER_H
#define TOMAHAWK_RESULTPROVIDER_H

#include "DllMacro.h"

class QPixmap;
class QString;
class QSize;

namespace Tomahawk
{

class DLLEXPORT ResultProvider
{
public:
    virtual ~ResultProvider();

    virtual QString name() const = 0;
    virtual QPixmap icon( const QSize& size ) const = 0;
};

}

#endif // TOMAHAWK_RESULTPROVIDER_H
