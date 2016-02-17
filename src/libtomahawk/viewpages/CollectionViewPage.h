/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012-2014, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2014,      Teo Mrnjavac <teo@kde.org>
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

#ifndef COLLECTIONVIEWPAGE_H
#define COLLECTIONVIEWPAGE_H

#include "ViewPage.h"
#include "PlaylistInterface.h"
#include "DownloadManager.h"
#include "DllMacro.h"

class QPushButton;
class QStackedWidget;

class GridView;
class TrackView;
class ColumnView;
class TreeModel;
class PlayableModel;
class PlaylistModel;
class FilterHeader;

namespace Tomahawk {
    class MetaPlaylistInterface;
}

class DLLEXPORT CollectionViewPage : public QWidget, public Tomahawk::ViewPage
{
Q_OBJECT

public:
    enum CollectionViewPageMode
    { Columns = 0, Albums = 1, Flat = 2 };

    explicit CollectionViewPage( const Tomahawk::collection_ptr& collection, QWidget* parent = 0 );
    ~CollectionViewPage();

    virtual QWidget* widget() { return this; }
    virtual Tomahawk::playlistinterface_ptr playlistInterface() const;

    virtual QString title() const;
    virtual QString description() const;
    virtual QPixmap pixmap() const;

    virtual bool jumpToCurrentTrack();
    virtual bool isTemporaryPage() const;
    virtual bool isBeingPlayed() const;

    void setTreeModel( TreeModel* model );
    void setFlatModel( PlayableModel* model );
    void setAlbumModel( PlayableModel* model );

    void setPixmap( const QPixmap& pixmap, bool tinted = true );
    void setEmptyTip( const QString& tip );

public slots:
    void setCurrentMode( CollectionViewPageMode mode );
    virtual bool setFilter( const QString& pattern );
    void restoreViewMode(); //ViewManager calls this on every show

    void loadCollection( const Tomahawk::collection_ptr& collection );

signals:
    void modeChanged( CollectionViewPageMode mode );
    void destroyed( QWidget* widget );

private slots:
    void onModelChanged();
    void onCollectionChanged();

    void onDownloadAll();
    void onDownloadManagerStateChanged( DownloadManager::DownloadManagerState newState, DownloadManager::DownloadManagerState oldState );

private:
    FilterHeader* m_header;
    QPixmap m_pixmap;
    QPushButton* m_downloadAllButton;

    ColumnView* m_columnView;
    TrackView* m_trackView;
    GridView* m_albumView;

    TreeModel* m_model;
    PlayableModel* m_flatModel;
    PlayableModel* m_albumModel;
    QStackedWidget* m_stack;
    QSharedPointer<Tomahawk::MetaPlaylistInterface> m_playlistInterface;

    Tomahawk::collection_ptr m_collection;

    CollectionViewPageMode m_mode;
};

Q_DECLARE_METATYPE( CollectionViewPage::CollectionViewPageMode );

#endif // COLLECTIONVIEWPAGE_H
