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

#include "PlaylistPlaylistInterface.h"

#include "Playlist.h"
#include "SourceList.h"

#include "utils/Logger.h"

using namespace Tomahawk;


PlaylistPlaylistInterface::PlaylistPlaylistInterface( Tomahawk::Playlist* playlist )
    : Tomahawk::PlaylistInterface()
    , m_playlist( playlist )
{
}


PlaylistPlaylistInterface::~PlaylistPlaylistInterface()
{
    m_playlist.clear();
}


int
PlaylistPlaylistInterface::trackCount() const
{
    return ( m_playlist.isNull() ? 0 : m_playlist.data()->entries().count() );
}


QList< Tomahawk::query_ptr >
PlaylistPlaylistInterface::tracks() const
{
    QList<Tomahawk::query_ptr> queries;
    foreach( const plentry_ptr& p, ( m_playlist.isNull() ? QList< Tomahawk::plentry_ptr >() : m_playlist.data()->entries() ) )
        queries << p->query();

    return queries;
}
