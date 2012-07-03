/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#ifndef PLAYLISTPLAYLISTINTERFACE_H
#define PLAYLISTPLAYLISTINTERFACE_H

#include <QObject>
#include <QList>
#include <QSharedPointer>

#include "Typedefs.h"
#include "Result.h"
#include "PlaylistInterface.h"
#include "Query.h"

#include "DllMacro.h"

namespace Tomahawk
{

class DLLEXPORT PlaylistPlaylistInterface : public Tomahawk::PlaylistInterface
{
Q_OBJECT

public:
    PlaylistPlaylistInterface( Tomahawk::Playlist* playlist );
    virtual ~PlaylistPlaylistInterface();

    virtual QList<Tomahawk::query_ptr> tracks();

    virtual int trackCount() const;

    virtual bool hasNextItem() { return false; }
    virtual Tomahawk::result_ptr currentItem() const { return m_currentItem; }

    virtual Tomahawk::result_ptr siblingItem( int /*itemsAway*/ ) { return result_ptr(); }

    virtual PlaylistModes::RepeatMode repeatMode() const { return PlaylistModes::NoRepeat; }
    virtual bool shuffled() const { return false; }

public slots:
    virtual void setRepeatMode( PlaylistModes::RepeatMode ) {}
    virtual void setShuffled( bool ) {}

private:
    PlaylistPlaylistInterface();
    Q_DISABLE_COPY( PlaylistPlaylistInterface )

    QWeakPointer< Tomahawk::Playlist > m_playlist;

    result_ptr m_currentItem;
};

}

#endif // PLAYLISTPLAYLISTINTERFACE_H
