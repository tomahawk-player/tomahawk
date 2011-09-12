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

#ifndef ARTISTVIEW_H
#define ARTISTVIEW_H

#include <QSortFilterProxyModel>
#include <QTreeView>
#include <QTimer>

#include "contextmenu.h"
#include "treeproxymodel.h"
#include "viewpage.h"

#include "dllmacro.h"

class TreeHeader;
class LoadingSpinner;
class OverlayWidget;
class TreeModel;

class DLLEXPORT ArtistView : public QTreeView, public Tomahawk::ViewPage
{
Q_OBJECT

public:
    explicit ArtistView( QWidget* parent = 0 );
    ~ArtistView();

    virtual QString guid() const;
    virtual void setGuid( const QString& guid ) { m_guid = guid; }

    void setProxyModel( TreeProxyModel* model );

    TreeModel* model() const { return m_model; }
    TreeProxyModel* proxyModel() const { return m_proxyModel; }
    OverlayWidget* overlay() const { return m_overlay; }
    //    PlaylistItemDelegate* delegate() { return m_delegate; }

    void setModel( QAbstractItemModel* model );
    void setTreeModel( TreeModel* model );

    virtual QWidget* widget() { return this; }
    virtual Tomahawk::PlaylistInterface* playlistInterface() const { return proxyModel(); }

    virtual QString title() const { return m_model->title(); }
    virtual QString description() const { return m_model->description(); }
    virtual QPixmap pixmap() const { return m_model->icon(); }

    virtual bool showStatsBar() const { return false; }
    virtual bool showFilter() const { return true; }

    virtual void setShowModes( bool b ) { m_showModes = b; }
    virtual bool showModes() const { return m_showModes; }

    virtual bool jumpToCurrentTrack();

public slots:
    void onItemActivated( const QModelIndex& index );

protected:
    virtual void startDrag( Qt::DropActions supportedActions );
    virtual void resizeEvent( QResizeEvent* event );

    void paintEvent( QPaintEvent* event );
    void keyPressEvent( QKeyEvent* event );

private slots:
    void onFilterChanged( const QString& filter );
    void onFilteringStarted();
    void onViewChanged();
    void onScrollTimeout();

    void onCustomContextMenu( const QPoint& pos );
    void onMenuTriggered( int action );

private:
    TreeHeader* m_header;
    OverlayWidget* m_overlay;
    TreeModel* m_model;
    TreeProxyModel* m_proxyModel;
//    PlaylistItemDelegate* m_delegate;
    LoadingSpinner* m_loadingSpinner;

    QModelIndex m_contextMenuIndex;
    Tomahawk::ContextMenu* m_contextMenu;

    bool m_showModes;
    QTimer m_timer;
    mutable QString m_guid;
};

#endif // ARTISTVIEW_H
