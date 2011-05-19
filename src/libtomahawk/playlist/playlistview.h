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

#ifndef PLAYLISTVIEW_H
#define PLAYLISTVIEW_H

#include <QMenu>

#include "playlist/trackproxymodel.h"
#include "playlist/playlistmodel.h"
#include "trackview.h"
#include "viewpage.h"

#include "dllmacro.h"

class PlaylistModel;

class DLLEXPORT PlaylistView : public TrackView, public Tomahawk::ViewPage
{
Q_OBJECT

public:
    explicit PlaylistView( QWidget* parent = 0 );
    ~PlaylistView();

    PlaylistModel* playlistModel() const { return m_model; }
    virtual void setPlaylistModel( PlaylistModel* model );
    virtual void setModel( QAbstractItemModel* model );

    virtual QWidget* widget() { return this; }
    virtual PlaylistInterface* playlistInterface() const { return proxyModel(); }

    virtual bool showFilter() const { return true; }

    virtual QString title() const { return playlistModel()->title(); }
    virtual QString description() const { return m_model->description(); }
    virtual QPixmap pixmap() const { return QPixmap( RESPATH "images/playlist-icon.png" ); }

    virtual bool jumpToCurrentTrack();

signals:
    void nameChanged( const QString& title );
    void destroyed( QWidget* widget );

protected:
    void keyPressEvent( QKeyEvent* event );

private slots:
    void onCustomContextMenu( const QPoint& pos );
    void onTrackCountChanged( unsigned int tracks );

    void addItemsToPlaylist();
    void deleteItems();

    void onDeleted();
    void onChanged();
private:
    void setupMenus();

    PlaylistModel* m_model;

    QMenu m_itemMenu;

    QAction* m_playItemAction;
    QAction* m_addItemsToQueueAction;
    QAction* m_addItemsToPlaylistAction;
    QAction* m_deleteItemsAction;
};

#endif // PLAYLISTVIEW_H
