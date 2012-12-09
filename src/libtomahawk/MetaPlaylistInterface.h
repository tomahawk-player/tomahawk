/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#ifndef METAPLAYLISTINTERFACE_H
#define METAPLAYLISTINTERFACE_H

#include <QtCore/QModelIndex>

#include "PlaylistInterface.h"
#include "playlist/PlayableItem.h"
#include "Typedefs.h"
#include "DllMacro.h"

namespace Tomahawk
{

class DLLEXPORT MetaPlaylistInterface : public PlaylistInterface
{
Q_OBJECT

public:
    explicit MetaPlaylistInterface();
    virtual ~MetaPlaylistInterface();

    void addChildInterface( const Tomahawk::playlistinterface_ptr& interface );

    virtual QList< Tomahawk::query_ptr > tracks() const;
    virtual int trackCount() const;
    virtual Tomahawk::result_ptr currentItem() const;
    virtual qint64 siblingIndex( int itemsAway, qint64 rootIndex = -1 ) const;
    virtual Tomahawk::result_ptr resultAt( qint64 index ) const;
    virtual Tomahawk::query_ptr queryAt( qint64 index ) const;
    virtual qint64 indexOfResult( const Tomahawk::result_ptr& result ) const;
    virtual qint64 indexOfQuery( const Tomahawk::query_ptr& query ) const;

    virtual PlaylistModes::RepeatMode repeatMode() const;
    virtual bool shuffled() const;

    virtual bool hasChildInterface( const Tomahawk::playlistinterface_ptr& interface );

public slots:
    virtual void setRepeatMode( PlaylistModes::RepeatMode mode );
    virtual void setShuffled( bool enabled );

private:
    QList< Tomahawk::playlistinterface_ptr > m_childInterfaces;
};

}

#endif // METAPLAYLISTINTERFACE_H
