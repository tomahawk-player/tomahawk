/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
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

#ifndef TREEMODEL_H
#define TREEMODEL_H

#include <QAbstractItemModel>
#include <QPixmap>

#include "Artist.h"
#include "Album.h"
#include "Query.h"
#include "Result.h"
#include "PlayableModel.h"
#include "PlaylistInterface.h"
#include "database/DatabaseCommand_AllArtists.h"

#include "DllMacro.h"
#include "Typedefs.h"

class QMetaData;

class PlayableItem;

class DLLEXPORT TreeModel : public PlayableModel
{
Q_OBJECT

public:
    explicit TreeModel( QObject* parent = 0 );
    virtual ~TreeModel();

    virtual Tomahawk::ModelMode mode() const { return m_mode; }
    virtual void setMode( Tomahawk::ModelMode mode );

    Tomahawk::collection_ptr collection() const;

    void addAllCollections();
    void addCollection( const Tomahawk::collection_ptr& collection );
    //TODO: Unused, but will be useful for supporting filtered queries. - Teo 1/2013
    //void addFilteredCollection( const Tomahawk::collection_ptr& collection, unsigned int amount, DatabaseCommand_AllArtists::SortOrder order );

    void addArtists( const Tomahawk::artist_ptr& artist );
    void addTracks( const Tomahawk::album_ptr& album, const QModelIndex& parent, bool autoRefetch = false );
    void fetchAlbums( const Tomahawk::artist_ptr& artist );

    void getCover( const QModelIndex& index );

    virtual PlayableItem* itemFromResult( const Tomahawk::result_ptr& result ) const;

    virtual QModelIndex indexFromArtist( const Tomahawk::artist_ptr& artist ) const;
    virtual QModelIndex indexFromAlbum( const Tomahawk::album_ptr& album ) const;
    virtual QModelIndex indexFromResult( const Tomahawk::result_ptr& result ) const;
    virtual QModelIndex indexFromQuery( const Tomahawk::query_ptr& query ) const;

public slots:
    void addAlbums( const QModelIndex& parent, const QList<Tomahawk::album_ptr>& albums );
    void reloadCollection();

signals:
    void modeChanged( Tomahawk::ModelMode mode );

protected:
    bool canFetchMore( const QModelIndex& parent ) const;
    void fetchMore( const QModelIndex& parent );

private slots:
    void onArtistsAdded( const QList<Tomahawk::artist_ptr>& artists );
    void onAlbumsFound( const QList<Tomahawk::album_ptr>& albums, Tomahawk::ModelMode mode );
    void onTracksAdded( const QList<Tomahawk::query_ptr>& tracks, const QModelIndex& index );
    void onTracksFound( const QList<Tomahawk::query_ptr>& tracks, Tomahawk::ModelMode mode, Tomahawk::collection_ptr collection );

    void onSourceAdded( const Tomahawk::source_ptr& source );
    void onCollectionChanged();

private:
    Tomahawk::ModelMode m_mode;
    Tomahawk::collection_ptr m_collection;

    QList<Tomahawk::artist_ptr> m_artistsFilter;
};

#endif // ALBUMMODEL_H
