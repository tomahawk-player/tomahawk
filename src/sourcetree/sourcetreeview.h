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

#ifndef SOURCETREEVIEW_H
#define SOURCETREEVIEW_H

#include <QTreeView>
#include <QMenu>

#include "source.h"
#include "sourcetree/sourcesmodel.h"

class CollectionModel;
class PlaylistModel;
class SourcesModel;
class SourcesProxyModel;

class SourceTreeView : public QTreeView
{
Q_OBJECT

public:
    explicit SourceTreeView( QWidget* parent = 0 );

public slots:
    void showOfflineSources();
    void hideOfflineSources();

    void renamePlaylist();
signals:
    void onOnline( const QModelIndex& index );
    void onOffline( const QModelIndex& index );

private slots:
    void onItemExpanded( const QModelIndex& idx );
    void onItemActivated( const QModelIndex& index );
    void selectRequest( const QModelIndex& idx );

    void loadPlaylist();
    void deletePlaylist();

    void onCustomContextMenu( const QPoint& pos );
protected:
//    void drawBranches( QPainter* painter, const QRect& rect, const QModelIndex& index ) const {}
    void drawRow( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const;

    virtual void paintEvent( QPaintEvent* event );

    virtual void dragEnterEvent( QDragEnterEvent* event );
    virtual void dragLeaveEvent( QDragLeaveEvent* event ) { Q_UNUSED( event ); m_dragging = false; setDirtyRegion( m_dropRect ); }
    virtual void dragMoveEvent( QDragMoveEvent* event );
    virtual void dropEvent( QDropEvent* event );

private:
    void setupMenus();

    template< typename T >
    T* itemFromIndex( const QModelIndex& index ) const;

    SourcesModel* m_model;
    SourcesProxyModel* m_proxyModel;
    QModelIndex m_contextMenuIndex;

    QMenu m_playlistMenu;
    QAction* m_loadPlaylistAction;
    QAction* m_renamePlaylistAction;
    QAction* m_deletePlaylistAction;

    bool m_dragging;
    QRect m_dropRect;
};

#endif // SOURCETREEVIEW_H
