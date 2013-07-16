/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
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
#ifndef PLAYLISTSMODEL_P_H
#define PLAYLISTSMODEL_P_H

#include "PlaylistsModel.h"

namespace Tomahawk
{

class PlaylistsModelPrivate
{
public:
    PlaylistsModelPrivate( PlaylistsModel* q, const QList<playlist_ptr>& _playlists )
        : q_ptr( q )
        , playlists( _playlists )
    {
    }
    virtual ~PlaylistsModelPrivate() {}

    PlaylistsModel* q_ptr;
    Q_DECLARE_PUBLIC( PlaylistsModel )
private:
    QList<playlist_ptr> playlists;
    QMap<playlist_ptr, QString> artists;
};

}

#endif // PLAYLISTSMODEL_P_H
