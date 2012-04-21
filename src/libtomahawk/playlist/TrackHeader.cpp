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

#include "TrackHeader.h"

#include "playlist/TrackView.h"
#include "utils/logger.h"


TrackHeader::TrackHeader( TrackView* parent )
    : ViewHeader( parent )
    , m_parent( parent )
{
    QList< double > columnWeights;
    columnWeights << 0.16 << 0.16 << 0.14 << 0.12 << 0.05 << 0.05 << 0.05 << 0.05 << 0.05 << 0.05 << 0.09; // << 0.03;

    setDefaultColumnWeights( columnWeights );
}


TrackHeader::~TrackHeader()
{
}
