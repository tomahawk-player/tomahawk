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

#ifndef DYNAMIC_PLAYLIST_REVISION_H
#define DYNAMIC_PLAYLIST_REVISION_H

#include "playlist.h"
#include "dllmacro.h"

namespace Tomahawk
{

struct DLLEXPORT DynamicPlaylistRevision : PlaylistRevision
{
public:

    QList< dyncontrol_ptr > controls;
    Tomahawk::GeneratorMode mode;
    QString type;

    DynamicPlaylistRevision( const PlaylistRevision& other )
    {
        revisionguid = other.revisionguid;
        oldrevisionguid = other.oldrevisionguid;
        newlist = other.newlist;
        added = other.added;
        removed = other.removed;
        applied = other.applied;
    }

    DynamicPlaylistRevision() {}
};

}

#endif
