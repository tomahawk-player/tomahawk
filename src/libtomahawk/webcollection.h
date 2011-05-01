/*
    Copyright (C) 2011  Leo Franchi <lfranchi@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/


#ifndef WEBCOLLECTION_H
#define WEBCOLLECTION_H

#include "collection.h"
#include "dllmacro.h"

namespace Tomahawk
{

class DLLEXPORT WebCollection : public Collection
{
    Q_OBJECT
public:
    WebCollection( const source_ptr& source, const QString& name = QString(), QObject* parent = 0 );
    virtual ~WebCollection();

    virtual void removeTracks( const QDir& ) {}
    virtual void addTracks( const QList< QVariant >& ) {}
};


}

#endif // WEBCOLLECTION_H
