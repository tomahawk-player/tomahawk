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

    virtual bool showStatsBar() const { return false; }
    virtual bool showFilter() const { return true; }

//    virtual void setShowModes( bool b ) { m_showModes = b; }
    virtual bool showModes() const { return false; }

    virtual bool setFilter( const QString& filter );
    virtual bool jumpToCurrentTrack();

    void setTrackView( TrackView* view );
    void setDetailedView( TrackView* view );
    void setGridView( GridView* view );

    void setPlayableModel( PlayableModel* model );

public slots:
    void setCurrentMode( FlexibleViewMode mode );

signals:
    void modeChanged( FlexibleViewMode mode );

private:
    TrackView* m_trackView;
    TrackView* m_detailedView;
    GridView* m_gridView;

    PlayableModel* m_model;
    QStackedWidget* m_stack;

    FlexibleViewMode m_mode;
};

Q_DECLARE_METATYPE( FlexibleView::FlexibleViewMode );

#endif // FLEXIBLEVIEW_H
