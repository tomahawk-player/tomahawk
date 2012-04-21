/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include "PlaylistInterface.h"
#include "utils/logger.h"
#include "Result.h"

using namespace Tomahawk;

PlaylistInterface::PlaylistInterface ()
    : QObject()
    , m_latchMode( StayOnSong )
{
    m_id = uuid();
    qRegisterMetaType<Tomahawk::PlaylistInterface::RepeatMode>( "Tomahawk::PlaylistInterface::RepeatMode" );
}

PlaylistInterface::~PlaylistInterface()
{
}

result_ptr
PlaylistInterface::previousItem()
{
     return siblingItem( -1 );
}

result_ptr
PlaylistInterface::nextItem()
{
     return siblingItem( 1 );
}
