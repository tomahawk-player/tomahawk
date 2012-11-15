/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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

#include "DynamicPlaylistRevision.h"

#include "playlist/dynamic/DynamicControl.h"
#include "Source.h"

using namespace Tomahawk;

DynamicPlaylistRevision::DynamicPlaylistRevision(const PlaylistRevision &other)
{
    revisionguid = other.revisionguid;
    oldrevisionguid = other.oldrevisionguid;
    newlist = other.newlist;
    added = other.added;
    removed = other.removed;
    applied = other.applied;
}

DynamicPlaylistRevision::DynamicPlaylistRevision()
{}
