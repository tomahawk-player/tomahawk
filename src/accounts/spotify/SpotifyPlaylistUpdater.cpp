/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2012, Leo Franchi <lfranchi@kde.org>
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

#include "SpotifyPlaylistUpdater.h"

SpotifyPlaylistUpdater::SpotifyPlaylistUpdater(const Tomahawk::playlist_ptr& pl): PlaylistUpdaterInterface(pl)
{

}


SpotifyPlaylistUpdater::~SpotifyPlaylistUpdater()
{

}

void
SpotifyPlaylistUpdater::loadFromSettings( const QString& group )
{

}

void
SpotifyPlaylistUpdater::removeFromSettings( const QString& group ) const
{

}


void
SpotifyPlaylistUpdater::saveToSettings( const QString& group ) const
{

}


QString
SpotifyPlaylistUpdater::type() const
{
    return QString();
}

void
SpotifyPlaylistUpdater::updateNow()
{
}
