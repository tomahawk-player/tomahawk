/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2013,      Uwe L. Korn <uwelk@xhochy.com>
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
#ifndef PLAYLISTMODEL_P_H
#define PLAYLISTMODEL_P_H

#include "PlaylistModel.h"
#include "PlayableModel_p.h"

class PlaylistModelPrivate : public PlayableModelPrivate
{
public:
    PlaylistModelPrivate( PlaylistModel* q )
        : PlayableModelPrivate( q, false )
        , isTemporary( false )
        , changesOngoing( false )
        , isLoading( false )
        , acceptPlayableQueriesOnly( false )
        , savedInsertPos( -1 )
    {
    }

    Q_DECLARE_PUBLIC( PlaylistModel )

private:
    Tomahawk::playlist_ptr playlist;
    bool isTemporary;
    bool changesOngoing;
    bool isLoading;
    bool acceptPlayableQueriesOnly;
    QList< Tomahawk::Query* > waitingForResolved;
    QStringList waitForRevision;

    int savedInsertPos;
    QList< Tomahawk::plentry_ptr > savedInsertTracks;
    QList< Tomahawk::query_ptr > savedRemoveTracks;

    PlaylistModel::DropStorageData dropStorage;
};

#endif // PLAYLISTMODEL_P_H
