/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Teo Mrnjavac <teo@kde.org>
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

#ifndef TRACKSREQUEST_H
#define TRACKSREQUEST_H

#include "Typedefs.h"
#include "DllMacro.h"

#include <QList>

namespace Tomahawk
{

class DLLEXPORT TracksRequest
{
public:
    virtual ~TracksRequest();

    virtual void enqueue() = 0;

protected: //signals
    virtual void tracks( const QList< Tomahawk::query_ptr >& ) = 0;
};

} //ns

#endif // TRACKSREQUEST_H
