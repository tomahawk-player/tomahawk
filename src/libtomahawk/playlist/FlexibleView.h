/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#ifndef FLEXIBLEVIEW_H
#define FLEXIBLEVIEW_H

#include "ViewPage.h"
#include "PlaylistInterface.h"
#include "DllMacro.h"

class QStackedWidget;

class GridView;
class TrackView;
class PlayableModel;
class PlaylistModel;
class FlexibleHeader;

class DLLEXPORT FlexibleView : public QWidget, public Tomahawk::ViewPage
{
Q_OBJECT

public:
    enum FlexibleViewMode
    { Flat = 0, Detailed = 1, Grid = 2 };

    explicit FlexibleView( QWidget* parent = 0 );
    ~FlexibleView();

    virtual QWidget* widget() { return this; }
    virtual Tomahawk::playlistinterface_ptr playlistInterface() const;

    virtual QString title() const;
    virtual QString description() const;
    virtual QPixmap pixmap() const;

    virtual bool showInfoBar() const { return false; }
    virtual bool jumpToCurrentTrack();

    TrackView* trackView() const { return m_trackView; }
    TrackView* detailedView() const { return m_detailedView; }
    GridView* gridView() const { return m_gridView; }

    void setGuid( const QString& guid );

    void setTrackView( TrackView* view );
    void setDetailedView( TrackView* view );
    void setGridView( GridView* view );

    void setPlayableModel( PlayableModel* model );
    void setPlaylistModel( PlaylistModel* model );

    void setPixmap( const QPixmap& pixmap );
    void setEmptyTip( const QString& tip );

public slots:
    void setCurrentMode( FlexibleViewMode mode );
    virtual bool setFilter( const QString& pattern );

signals:
    void modeChanged( FlexibleViewMode mode );
    void destroyed( QWidget* widget );

private slots:
    void onModelChanged();
    void onWidgetDestroyed( QWidget* widget );

private:
    FlexibleHeader* m_header;
    QPixmap m_pixmap;

    TrackView* m_trackView;
    TrackView* m_detailedView;
    GridView* m_gridView;

    PlayableModel* m_model;
    QStackedWidget* m_stack;

    FlexibleViewMode m_mode;
};

Q_DECLARE_METATYPE( FlexibleView::FlexibleViewMode );

#endif // FLEXIBLEVIEW_H
