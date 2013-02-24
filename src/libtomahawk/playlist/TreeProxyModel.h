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

#ifndef TREEPROXYMODEL_H
#define TREEPROXYMODEL_H

#include "PlaylistInterface.h"
#include "TreeModel.h"
#include "PlayableProxyModel.h"

#include "DllMacro.h"

namespace Tomahawk
{
    class ArtistsRequest;
    class TreeProxyModelPlaylistInterface;
}

class DLLEXPORT TreeProxyModel : public PlayableProxyModel
{
Q_OBJECT

public:
    explicit TreeProxyModel( QObject* parent = 0 );
    virtual ~TreeProxyModel() {}

    virtual void setSourcePlayableModel( TreeModel* model );
    // workaround overloaded-virtual warning
    virtual void setSourcePlayableModel( PlayableModel* ) { Q_ASSERT( false ); }

    virtual void setFilter( const QString& pattern );
    virtual QString filter() const;

    QModelIndex indexFromArtist( const Tomahawk::artist_ptr& artist ) const;
    QModelIndex indexFromAlbum( const Tomahawk::album_ptr& album ) const;
    QModelIndex indexFromResult( const Tomahawk::result_ptr& result ) const;
    QModelIndex indexFromQuery( const Tomahawk::query_ptr& query ) const;

protected:
    bool filterAcceptsRow( int sourceRow, const QModelIndex& sourceParent ) const;

    bool lessThan( const QModelIndex& left, const QModelIndex& right ) const;
    // workaround overloaded-virtual warning: using this would lead to serious weirdness in release mode, sometimes an assert is simply not enough
    bool lessThan( int, const Tomahawk::query_ptr&, const Tomahawk::query_ptr& ) const {  Q_ASSERT( false ); TomahawkUtils::crash(); return false; }

private slots:
    void onRowsInserted( const QModelIndex& parent, int start, int end );

    void onFilterArtists( const QList<Tomahawk::artist_ptr>& artists );
    void onFilterAlbums( const QList<Tomahawk::album_ptr>& albums );

    void onModelReset();

private:
    void filterFinished();
    QString textForItem( PlayableItem* item ) const;

    mutable QMap< QPersistentModelIndex, Tomahawk::query_ptr > m_cache;

    QList<Tomahawk::artist_ptr> m_artistsFilter;
    QList<int> m_albumsFilter;
    Tomahawk::ArtistsRequest* m_artistsFilterCmd;

    QString m_filter;
    TreeModel* m_model;
};

#endif // TREEPROXYMODEL_H
