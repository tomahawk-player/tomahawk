/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include "DatabaseCommand_ModifyPlaylist.h"

#include "utils/Logger.h"
#include "Playlist.h"
#include "Source.h"

using namespace Tomahawk;


DatabaseCommand_ModifyPlaylist::DatabaseCommand_ModifyPlaylist( Playlist* playlist, const QList< plentry_ptr >& entries, Mode mode )
    : DatabaseCommand()
    , m_playlist( playlist )
    , m_entries( entries )
    , m_mode( mode )
{
}

DatabaseCommand_ModifyPlaylist::~DatabaseCommand_ModifyPlaylist()
{}

void
DatabaseCommand_ModifyPlaylist::exec( DatabaseImpl* lib )
{
    Q_UNUSED( lib );
}
