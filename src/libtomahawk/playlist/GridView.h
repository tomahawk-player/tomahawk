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

#ifndef GRIDVIEW_H
#define GRIDVIEW_H

#include <QListView>
#include <QSortFilterProxyModel>
#include <QTimer>

#include "ViewPage.h"
#include "PlayableProxyModel.h"
#include "widgets/OverlayWidget.h"
#include "DllMacro.h"

namespace Tomahawk
{
    class ContextMenu;
};

class AnimatedSpinner;
class GridItemDelegate;
class PlayableModel;
class GridPlaylistInterface;

class DLLEXPORT GridView : public QListView, public Tomahawk::ViewPage
{
Q_OBJECT

public:
    explicit GridView( QWidget* parent = 0 );
    ~GridView();

    void setProxyModel( PlayableProxyModel* model );

    PlayableModel* model() const { return m_model; }
    PlayableProxyModel* proxyModel() const { return m_proxyModel; }
    GridItemDelegate* delegate() const { return m_delegate; }

    bool autoFitItems() const { return m_autoFitItems; }
    void setAutoFitItems( bool b ) { m_autoFitItems = b; }

    bool autoResize() const { return m_autoResize; }
    void setAutoResize( bool b ) { m_autoResize = b; }

    void setPlayableModel( PlayableModel* model );
    void setModel( QAbstractItemModel* model );

    void setEmptyTip( const QString& tip );

    virtual QWidget* widget() { return this; }
    virtual Tomahawk::playlistinterface_ptr playlistInterface() const;
    void setPlaylistInterface( const Tomahawk::playlistinterface_ptr& playlistInterface );

    virtual QString title() const { return m_model->title(); }
    virtual QString description() const { return m_model->description(); }

    virtual bool setFilter( const QString& filter );
    virtual bool jumpToCurrentTrack();
    QRect currentTrackRect() const;

    virtual bool isBeingPlayed() const { return m_playing.isValid(); }

public slots:
    void onItemActivated( const QModelIndex& index );

signals:
    void modelChanged();
    void scrolledContents( int dx, int dy );
    void resized();

protected:
    virtual void startDrag( Qt::DropActions supportedActions );
    virtual void scrollContentsBy( int dx, int dy );

    void paintEvent( QPaintEvent* event );
    void resizeEvent( QResizeEvent* event );
    void wheelEvent( QWheelEvent* );

protected slots:
    virtual void currentChanged( const QModelIndex& current, const QModelIndex& previous );

private slots:
    void onFilterChanged( const QString& filter );
    void onCustomContextMenu( const QPoint& pos );

    void onDelegatePlaying( const QPersistentModelIndex& idx );
    void onDelegateStopped( const QPersistentModelIndex& idx );

    void layoutItems();
    void verifySize();

private:
    PlayableModel* m_model;
    PlayableProxyModel* m_proxyModel;
    GridItemDelegate* m_delegate;
    AnimatedSpinner* m_loadingSpinner;
    OverlayWidget* m_overlay;

    QModelIndex m_contextMenuIndex;
    QPersistentModelIndex m_playing;

    Tomahawk::ContextMenu* m_contextMenu;

    QString m_emptyTip;
    bool m_inited;
    bool m_autoFitItems;
    bool m_autoResize;

    QRect m_paintRect;

    friend class ::GridPlaylistInterface;
};

#endif // GRIDVIEW_H
