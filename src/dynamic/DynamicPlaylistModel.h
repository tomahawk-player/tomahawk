/****************************************************************************************
 * Copyright (c) 2010 Leo Franchi <lfranchi@kde.org>                                    *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/
#ifndef DYNAMIC_PLAYLIST_MODEL_H
#define DYNAMIC_PLAYLIST_MODEL_H

#include "typedefs.h"
#include "playlist/playlistmodel.h"
    
/**
 * Simple model that extends PlaylistModel with support for adding/removing tracks from top and bottom.
 */
class DynamicPlaylistModel : public PlaylistModel
{
Q_OBJECT
public:
    explicit DynamicPlaylistModel( QObject* parent = 0 );
    ~DynamicPlaylistModel();
    
    void loadPlaylist( const Tomahawk::dynplaylist_ptr& playlist );
    
private:
    Tomahawk::dynplaylist_ptr m_playlist;
};


#endif
