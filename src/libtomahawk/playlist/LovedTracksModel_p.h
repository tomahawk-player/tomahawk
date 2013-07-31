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
#ifndef LOVEDTRACKSMODEL_P_H
#define LOVEDTRACKSMODEL_P_H

#include "LovedTracksModel.h"
#include "PlaylistModel_p.h"

#include <QTimer>

class LovedTracksModelPrivate : public PlaylistModelPrivate
{
public:
    LovedTracksModelPrivate( LovedTracksModel* q )
        : PlaylistModelPrivate( q )
        , limit( defaultNumberOfLovedTracks )
    {
    }

    Q_DECLARE_PUBLIC( LovedTracksModel )
    static const uint defaultNumberOfLovedTracks = 25;

protected:
    uint limit;
    Tomahawk::source_ptr source;
    QTimer smoothingTimer;
};

#endif // LOVEDTRACKSMODEL_P_H
