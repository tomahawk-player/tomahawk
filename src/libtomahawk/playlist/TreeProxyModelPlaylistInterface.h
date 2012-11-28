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

#ifndef TREEPROXYMODELPLAYLISTINTERFACE_H
#define TREEPROXYMODELPLAYLISTINTERFACE_H

#include "PlaylistInterface.h"
#include "TreeModel.h"

#include "DllMacro.h"

class DatabaseCommand_AllArtists;
class TreeProxyModel;

namespace Tomahawk
{

class DLLEXPORT TreeProxyModelPlaylistInterface : public Tomahawk::PlaylistInterface
{
Q_OBJECT

public:
    explicit TreeProxyModelPlaylistInterface( TreeProxyModel* proxyModel );
    virtual ~TreeProxyModelPlaylistInterface();

    virtual QList< Tomahawk::query_ptr > tracks() const;
    virtual int trackCount() const;

    virtual Tomahawk::result_ptr resultAt( qint64 index ) const;
    virtual Tomahawk::query_ptr queryAt( qint64 index ) const;
    virtual qint64 indexOfResult( const Tomahawk::result_ptr& result ) const;
    virtual qint64 indexOfQuery( const Tomahawk::query_ptr& query ) const;

    virtual void setCurrentIndex( qint64 index );
    virtual Tomahawk::result_ptr currentItem() const;
    virtual qint64 siblingIndex( int itemsAway, qint64 rootIndex = -1 ) const;

    virtual QString filter() const;

    virtual PlaylistModes::RepeatMode repeatMode() const { return m_repeatMode; }
    virtual bool shuffled() const { return m_shuffled; }
    virtual PlaylistModes::ViewMode viewMode() const { return PlaylistModes::Tree; }

signals:
    void filterChanged( const QString& filter );
    void filteringStarted();
    void filteringFinished();

public slots:
    virtual void setRepeatMode( Tomahawk::PlaylistModes::RepeatMode mode ) { m_repeatMode = mode; emit repeatModeChanged( mode ); }
    virtual void setShuffled( bool enabled ) { m_shuffled = enabled; emit shuffleModeChanged( enabled ); }

private:
    QWeakPointer< TreeProxyModel > m_proxyModel;

    PlaylistModes::RepeatMode m_repeatMode;
    bool m_shuffled;
};

} //ns

#endif // TREEPROXYMODELPLAYLISTINTERFACE_H
