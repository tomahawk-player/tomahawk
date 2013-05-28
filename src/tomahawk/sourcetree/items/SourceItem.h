/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright 2010-2013, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2013,      Teo Mrnjavac <teo@kde.org>
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

#ifndef SOURCE_ITEM_H
#define SOURCE_ITEM_H

#include "SourceTreeItem.h"

#include "playlist_ptr.h"
#include "dynplaylist_ptr.h"

class TemporaryPageItem;
class GenericPageItem;
class CategoryItem;

namespace Tomahawk
{
    class ViewPage;
}

class SourceItem : public SourceTreeItem
{
    Q_OBJECT
public:
    SourceItem( SourcesModel* model, SourceTreeItem* parent, const Tomahawk::source_ptr& source );

    virtual QString text() const;
    virtual QString tooltip() const;
    virtual QIcon icon() const;
    virtual QPixmap pixmap( const QSize& size = QSize( 0, 0 ) ) const;
    virtual int peerSortValue() const;
    virtual int IDValue() const;

    virtual bool localLatchedOn() const;
    virtual Tomahawk::PlaylistModes::LatchMode localLatchMode() const;

    Tomahawk::source_ptr source() const;

    CategoryItem* stationsCategory() const;
    CategoryItem* playlistsCategory() const;
    void setStationsCategory( CategoryItem* item );
    void setPlaylistsCategory( CategoryItem* item );

    virtual bool willAcceptDrag( const QMimeData* data ) const;
    virtual bool dropMimeData( const QMimeData* data, Qt::DropAction action );
    virtual DropTypes supportedDropTypes( const QMimeData* data ) const;
    virtual Qt::ItemFlags flags() const;

public slots:
    virtual void activate();

private slots:
    void onPlaylistsAdded( const QList<Tomahawk::playlist_ptr>& playlists );
    void onPlaylistDeleted( const Tomahawk::playlist_ptr& playlists );
    void onAutoPlaylistsAdded( const QList<Tomahawk::dynplaylist_ptr>& playlists );
    void onAutoPlaylistDeleted( const Tomahawk::dynplaylist_ptr& playlists );
    void onStationsAdded( const QList<Tomahawk::dynplaylist_ptr>& stations );
    void onStationDeleted( const Tomahawk::dynplaylist_ptr& stations );

    void latchedOn( const Tomahawk::source_ptr&, const Tomahawk::source_ptr& );
    void latchedOff( const Tomahawk::source_ptr&, const Tomahawk::source_ptr& );
    void latchModeChanged( Tomahawk::PlaylistModes::LatchMode mode );

    void onCollectionAdded( const Tomahawk::collection_ptr& ); //never call from ctor because of begin/endRowsAdded!
    void onCollectionRemoved( const Tomahawk::collection_ptr& );

    void requestExpanding();

    Tomahawk::ViewPage* sourceInfoClicked();
    Tomahawk::ViewPage* getSourceInfoPage() const;

    Tomahawk::ViewPage* collectionClicked( const Tomahawk::collection_ptr& collection );
    Tomahawk::ViewPage* getCollectionPage( const Tomahawk::collection_ptr& collection ) const;

    Tomahawk::ViewPage* coolPlaylistsClicked();
    Tomahawk::ViewPage* getCoolPlaylistsPage() const;

    Tomahawk::ViewPage* latestAdditionsClicked();
    Tomahawk::ViewPage* getLatestAdditionsPage() const;

    Tomahawk::ViewPage* recentPlaysClicked();
    Tomahawk::ViewPage* getRecentPlaysPage() const;

    void onTracksDropped( const QList< Tomahawk::query_ptr >& queries );

private:
    void playlistsAddedInternal( SourceTreeItem* parent, const QList< Tomahawk::dynplaylist_ptr >& playlists );
    template< typename T >
    void playlistDeletedInternal( SourceTreeItem* parent, const T& playlists );
    void performAddCollectionItem( const Tomahawk::collection_ptr& collection );

    Tomahawk::source_ptr m_source;
    CategoryItem* m_playlists;
    CategoryItem* m_stations;

    bool m_latchedOn;
    Tomahawk::source_ptr m_latchedOnTo;

    QMap< Tomahawk::collection_ptr, GenericPageItem* > m_collectionItems;
    QMap< Tomahawk::collection_ptr, Tomahawk::ViewPage* > m_collectionPages;

    GenericPageItem* m_sourceInfoItem;
    GenericPageItem* m_coolPlaylistsItem;
    GenericPageItem* m_latestAdditionsItem;
    GenericPageItem* m_recentPlaysItem;

    Tomahawk::ViewPage* m_sourceInfoPage;
    Tomahawk::ViewPage* m_coolPlaylistsPage;
    Tomahawk::ViewPage* m_latestAdditionsPage;
    Tomahawk::ViewPage* m_recentPlaysPage;
    Tomahawk::ViewPage* m_whatsHotPage;
};


#endif
