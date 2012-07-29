/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012 Leo Franchi <lfranchi@kde.org>
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

#ifndef SINGLE_TRACK_PLAYLIST_INTERFACE
#define SINGLE_TRACK_PLAYLIST_INTERFACE

#include "PlaylistInterface.h"
#include "DllMacro.h"
#include "Query.h"
#include "Typedefs.h"
#include "Result.h"

namespace Tomahawk
{

class DLLEXPORT SingleTrackPlaylistInterface : public PlaylistInterface
{
    Q_OBJECT
public:
    explicit SingleTrackPlaylistInterface( const query_ptr& query )
        : PlaylistInterface()
        , m_track( query )
    {
    }

    query_ptr track() const { return m_track; }
    void setQuery( const query_ptr& track ) { m_track = track; }

    virtual result_ptr currentItem() const { return result_ptr(); }
    virtual PlaylistModes::RepeatMode repeatMode() const { return PlaylistModes::NoRepeat; }
    virtual void setRepeatMode( PlaylistModes::RepeatMode ) {}
    virtual void setShuffled( bool ) {}
    virtual bool shuffled() const { return false; }
    virtual result_ptr siblingItem( int itemsAway ) { return result_ptr(); }
    virtual int trackCount() const { return 1; }
    virtual QList< query_ptr > tracks() { return QList< query_ptr >(); }

private:
    query_ptr m_track;
};

}

#endif
