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

#ifndef RESOLVER_H
#define RESOLVER_H

#include "../ResultProvider.h"
#include "Typedefs.h"
#include "DllMacro.h"

#include <QObject>

// implement this if you can resolve queries to content

/*
    Weight: 1-100, 100 being the best
    Timeout: some millisecond value, after which we try the next highest
             weighted resolver

*/

namespace Tomahawk
{

class DLLEXPORT Resolver : public QObject, public ResultProvider
{
Q_OBJECT

public:
    Resolver() {}

    virtual unsigned int weight() const = 0;
    virtual unsigned int timeout() const = 0;

    virtual QPixmap icon( const QSize& size ) const override;

public slots:
    virtual void resolve( const Tomahawk::query_ptr& query ) = 0;
};

} //ns

#endif // RESOLVER_H
