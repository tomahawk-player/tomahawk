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

#include "typedefs.h"

#include <QTreeView>
#include <QMenu>

namespace Tomahawk {
    class PlaylistInterface;
}

class CollectionModel;
class PlaylistModel;
class SourcesModel;
class SourcesProxyModel;
class SourceDelegate;

class SourceTreeView : public QTreeView
{
Q_OBJECT

public:
    explicit SourceTreeView( QWidget* parent = 0 );

public slots:
    void showOfflineSources( bool offlineSourcesShown );

    void renamePlaylist();

    void update( const QModelIndex &index );

signals:
    void onOnline( const QModelIndex& index );
    void onOffline( const QModelIndex& index );

private slots:
    void onItemExpanded( const QModelIndex& idx );
    void onItemActivated( const QModelIndex& index );
    void selectRequest( const QPersistentModelIndex& idx );
    void expandRequest( const QPersistentModelIndex& idx );

    void loadPlaylist();
    void deletePlaylist( const QModelIndex& = QModelIndex() );
    void copyPlaylistLink();
    void addToLocal();

    void latchOn();
    void latchOff();
    void playlistChanged( Tomahawk::PlaylistInterface* );

    void onCustomContextMenu( const QPoint& pos );

protected:
    void drawRow( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const;

    virtual void paintEvent( QPaintEvent* event );

    virtual void dragEnterEvent( QDragEnterEvent* event );
    virtual void dragLeaveEvent( QDragLeaveEvent* event );
    virtual void dragMoveEvent( QDragMoveEvent* event );
    virtual void dropEvent( QDropEvent* event );
    virtual void keyPressEvent( QKeyEvent* event );

private:
    void setupMenus();

    template< typename T >
    T* itemFromIndex( const QModelIndex& index ) const;

    SourcesModel* m_model;
    SourcesProxyModel* m_proxyModel;
    QModelIndex m_contextMenuIndex;
    SourceDelegate* m_delegate;

    QMenu m_playlistMenu;
    QMenu m_roPlaylistMenu;
    QMenu m_latchMenu;
    QAction* m_loadPlaylistAction;
    QAction* m_renamePlaylistAction;
    QAction* m_deletePlaylistAction;
    QAction* m_copyPlaylistAction;
    QAction* m_addToLocalAction;
    QAction* m_latchOnAction;
    QAction* m_latchOffAction;
    Tomahawk::playlistinterface_ptr m_latch;

    bool m_dragging;
    QRect m_dropRect;
    QPersistentModelIndex m_dropIndex;
};

#endif // SOURCETREEVIEW_H
