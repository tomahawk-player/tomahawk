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

#ifndef TREEVIEW_H
#define TREEVIEW_H

#include <QSortFilterProxyModel>
#include <QTreeView>
#include <QTimer>

#include "TreeProxyModel.h"
#include "ViewPage.h"

#include "PlaylistInterface.h"

#include "DllMacro.h"

namespace Tomahawk
{
    class ContextMenu;
};

class ViewHeader;
class AnimatedSpinner;
class OverlayWidget;
class TreeModel;

class DLLEXPORT TreeView : public QTreeView
{
Q_OBJECT

public:
    explicit TreeView( QWidget* parent = 0 );
    ~TreeView();

    virtual QString guid() const;
    virtual void setGuid( const QString& guid ) { m_guid = guid; }

    void setProxyModel( TreeProxyModel* model );

    TreeModel* model() const { return m_model; }
    TreeProxyModel* proxyModel() const { return m_proxyModel; }
    OverlayWidget* overlay() const { return m_overlay; }

    void setModel( QAbstractItemModel* model );
    void setTreeModel( TreeModel* model );

    void setEmptyTip( const QString& tip );

    virtual bool jumpToCurrentTrack();

    QModelIndex hoveredIndex() const { return m_hoveredIndex; }

    bool updatesContextView() const { return m_updateContextView; }
    void setUpdatesContextView( bool b ) { m_updateContextView = b; }

public slots:
    void onItemActivated( const QModelIndex& index );

signals:
    void modelChanged();

protected:
    virtual void startDrag( Qt::DropActions supportedActions );
    virtual void resizeEvent( QResizeEvent* event );

    virtual void keyPressEvent( QKeyEvent* event );
    void wheelEvent( QWheelEvent* event );
    void mouseMoveEvent( QMouseEvent* event );
    void mousePressEvent( QMouseEvent* event );
    void leaveEvent( QEvent* event );

protected slots:
    virtual void currentChanged( const QModelIndex& current, const QModelIndex& previous );

private slots:
    void onFilterChangeFinished();
    void onFilteringStarted();
    void onViewChanged();
    void onScrollTimeout();

    void onCustomContextMenu( const QPoint& pos );
    void onMenuTriggered( int action );

private:
    void updateHoverIndex( const QPoint& pos );

    ViewHeader* m_header;
    OverlayWidget* m_overlay;
    TreeModel* m_model;
    TreeProxyModel* m_proxyModel;
    AnimatedSpinner* m_loadingSpinner;

    bool m_updateContextView;

    QModelIndex m_hoveredIndex;
    QModelIndex m_contextMenuIndex;
    Tomahawk::ContextMenu* m_contextMenu;

    QString m_emptyTip;
    QTimer m_timer;
    mutable QString m_guid;
};

#endif // TREEVIEW_H
