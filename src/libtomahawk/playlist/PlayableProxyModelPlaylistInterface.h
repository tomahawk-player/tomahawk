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

#ifndef TRACKPROXYMODELPLAYLISTINTERFACE_H
#define TRACKPROXYMODELPLAYLISTINTERFACE_H

#include <QtGui/QSortFilterProxyModel>

#include "PlaylistInterface.h"
#include "playlist/PlayableModel.h"

#include "DllMacro.h"

class PlayableProxyModel;

namespace Tomahawk {

class DLLEXPORT PlayableProxyModelPlaylistInterface : public Tomahawk::PlaylistInterface
{
Q_OBJECT

public:
    explicit PlayableProxyModelPlaylistInterface( PlayableProxyModel* proxyModel );
    virtual ~PlayableProxyModelPlaylistInterface();

    virtual QList<Tomahawk::query_ptr> tracks();

    virtual int trackCount() const;

    virtual Tomahawk::result_ptr currentItem() const;
    virtual Tomahawk::result_ptr siblingItem( int itemsAway );
    virtual Tomahawk::result_ptr siblingItem( int itemsAway, bool readOnly );
    virtual bool hasNextItem();

    virtual QString filter() const;

    virtual PlaylistModes::RepeatMode repeatMode() const { return m_repeatMode; }
    virtual bool shuffled() const { return m_shuffled; }

public slots:
    virtual void setRepeatMode( Tomahawk::PlaylistModes::RepeatMode mode ) { m_repeatMode = mode; emit repeatModeChanged( mode ); }
    virtual void setShuffled( bool enabled ) { m_shuffled = enabled; emit shuffleModeChanged( enabled ); }

protected:
    QWeakPointer< PlayableProxyModel > m_proxyModel;
    PlaylistModes::RepeatMode m_repeatMode;
    bool m_shuffled;
};

} //ns

#endif // TRACKPROXYMODELPLAYLISTINTERFACE_H
