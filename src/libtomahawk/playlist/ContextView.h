/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2014, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#ifndef CONTEXTVIEW_H
#define CONTEXTVIEW_H

#include "ViewPage.h"
#include "PlaylistInterface.h"
#include "DllMacro.h"

class QStackedWidget;

class CaptionLabel;
class GridView;
class TrackView;
class PlayableModel;
class PlaylistModel;
class FilterHeader;
class ModeHeader;

class DLLEXPORT ContextView : public QWidget, public Tomahawk::ViewPage
{
Q_OBJECT

public:
    explicit ContextView( QWidget* parent = 0, const QString& caption = QString(), QWidget* extraHeader = 0 );
    ~ContextView();

    virtual QWidget* widget() { return this; }
    virtual Tomahawk::playlistinterface_ptr playlistInterface() const;

    virtual QString title() const;
    virtual QString description() const;
    virtual QPixmap pixmap() const;

    virtual bool showInfoBar() const { return false; }
    virtual bool jumpToCurrentTrack();
    virtual bool isTemporaryPage() const;
    virtual bool isBeingPlayed() const;
    void setTemporaryPage( bool b );

    TrackView* trackView() const { return m_trackView; }

    void setCaption( const QString& caption );

    void setGuid( const QString& guid );

    void setTrackView( TrackView* view );

    void setPlayableModel( PlayableModel* model );
    void setPlaylistModel( PlaylistModel* model );

    void setPixmap( const QPixmap& pixmap );
    void setEmptyTip( const QString& tip );

public slots:
    virtual bool setFilter( const QString& pattern );
    void setShowCloseButton( bool b );

signals:
    void closeClicked();
    void destroyed( QWidget* widget );
    void pixmapChanged( const QPixmap& pixmap );

private slots:
    void onModelChanged();
    void onWidgetDestroyed( QWidget* widget );

    void onQuerySelected( const Tomahawk::query_ptr& query );

private:
    FilterHeader* m_header;
    ModeHeader* m_modeHeader;
    QPixmap m_pixmap;
    CaptionLabel* m_captionLabel;

    TrackView* m_trackView;

    PlayableModel* m_model;
    QStackedWidget* m_stack;

    bool m_temporary;
};

#endif // CONTEXTVIEW_H
