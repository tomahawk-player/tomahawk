/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2013, Uwe L. Korn <uwelk@xhochy.com>
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
#ifndef DATABASECOMMAND_LOADALLPLAYLISTS_P_H
#define DATABASECOMMAND_LOADALLPLAYLISTS_P_H

#include "DatabaseCommand_LoadAllPlaylists.h"

#include "DatabaseCommand_p.h"

namespace Tomahawk
{

class DatabaseCommand_LoadAllPlaylistsPrivate: public DatabaseCommandPrivate
{
public:
    DatabaseCommand_LoadAllPlaylistsPrivate( DatabaseCommand_LoadAllPlaylists* q, const source_ptr& s )
        : DatabaseCommandPrivate( q, s )
        , limitAmount( 0 )
        , returnPlEntryIds( false )
        , sortOrder( DatabaseCommand_LoadAllPlaylists::None )
        , sortDescending( false )
    {
    }

    Q_DECLARE_PUBLIC( DatabaseCommand_LoadAllPlaylists )

private:
    unsigned int limitAmount;
    bool returnPlEntryIds;
    DatabaseCommand_LoadAllPlaylists::SortOrder sortOrder;
    bool sortDescending;
};

}

#endif // DATABASECOMMAND_LOADALLPLAYLISTS_P_H
