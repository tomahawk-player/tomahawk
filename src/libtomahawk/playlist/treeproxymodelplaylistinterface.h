/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "playlistinterface.h"
#include "treemodel.h"

#include "dllmacro.h"

class DatabaseCommand_AllArtists;
class TreeProxyModel;

namespace Tomahawk
{

class DLLEXPORT TreeProxyModelPlaylistInterface : public Tomahawk::PlaylistInterface
{
Q_OBJECT

public:
    explicit TreeProxyModelPlaylistInterface( TreeProxyModel *proxyModel );
    virtual ~TreeProxyModelPlaylistInterface();

    virtual QList< Tomahawk::query_ptr > tracks() { Q_ASSERT( FALSE ); QList< Tomahawk::query_ptr > queries; return queries; }

    virtual int unfilteredTrackCount() const;
    virtual int trackCount() const;

    virtual bool hasNextItem();
    virtual Tomahawk::result_ptr currentItem() const;
    virtual Tomahawk::result_ptr siblingItem( int direction );
    virtual Tomahawk::result_ptr siblingItem( int direction, bool readOnly );

    virtual QString filter() const;
    virtual void setFilter( const QString& pattern );

    virtual QString vanillaFilter() const { return PlaylistInterface::filter(); }
    virtual void setVanillaFilter( const QString &filter ) { PlaylistInterface::setFilter( filter ); }

    virtual void sendTrackCount() { emit trackCountChanged( trackCount() ); }

    virtual PlaylistInterface::RepeatMode repeatMode() const { return m_repeatMode; }
    virtual bool shuffled() const { return m_shuffled; }
    virtual PlaylistInterface::ViewMode viewMode() const { return PlaylistInterface::Tree; }

signals:
    void repeatModeChanged( Tomahawk::PlaylistInterface::RepeatMode mode );
    void shuffleModeChanged( bool enabled );

    void trackCountChanged( unsigned int tracks );
    void sourceTrackCountChanged( unsigned int tracks );

    void filterChanged( const QString& filter );
    void filteringStarted();
    void filteringFinished();

    void nextTrackReady();

public slots:
    virtual void setRepeatMode( RepeatMode mode ) { m_repeatMode = mode; emit repeatModeChanged( mode ); }
    virtual void setShuffled( bool enabled ) { m_shuffled = enabled; emit shuffleModeChanged( enabled ); }

private:
    QWeakPointer< TreeProxyModel > m_proxyModel;

    RepeatMode m_repeatMode;
    bool m_shuffled;
};

} //ns

#endif // TREEPROXYMODELPLAYLISTINTERFACE_H
