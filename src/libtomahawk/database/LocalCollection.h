/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2011, Leo Franchi <lfranchi@kde.org>
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

#ifndef LOCALCOLLECTION_H
#define LOCALCOLLECTION_H

#include "DatabaseCollection.h"
#include "DllMacro.h"
#include "Playlist.h"

namespace Tomahawk
{

class DLLEXPORT LocalCollection : public DatabaseCollection
{
    Q_OBJECT
public:
    explicit LocalCollection( const Tomahawk::source_ptr& source, QObject* parent = 0 );

    virtual QString prettyName() const;
};

}

#endif // LOCALCOLLECTION_H
