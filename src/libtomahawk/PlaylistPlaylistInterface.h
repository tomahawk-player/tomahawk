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

    virtual QList<Tomahawk::query_ptr> tracks() const;

    virtual int trackCount() const;

    virtual void setCurrentIndex( qint64 index ) { Q_UNUSED( index ); }
    virtual Tomahawk::result_ptr resultAt( qint64 index ) const { Q_UNUSED( index ); Q_ASSERT( false ); return Tomahawk::result_ptr(); }
    virtual Tomahawk::query_ptr queryAt( qint64 index ) const { Q_UNUSED( index ); Q_ASSERT( false ); return Tomahawk::query_ptr(); }
    virtual qint64 indexOfResult( const Tomahawk::result_ptr& result ) const { Q_UNUSED( result ); Q_ASSERT( false ); return -1; }
    virtual qint64 indexOfQuery( const Tomahawk::query_ptr& query ) const { Q_UNUSED( query ); Q_ASSERT( false ); return -1; }

    virtual Tomahawk::result_ptr currentItem() const { return m_currentItem; }
    virtual qint64 siblingIndex( int /*itemsAway*/, qint64 rootIndex = -1 ) const { Q_UNUSED( rootIndex ); return -1; }

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
