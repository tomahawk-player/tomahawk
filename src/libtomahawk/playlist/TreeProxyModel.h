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

class DatabaseCommand_AllArtists;

namespace Tomahawk
{
    class TreeProxyModelPlaylistInterface;
}

class DLLEXPORT TreeProxyModel : public PlayableProxyModel
{
Q_OBJECT

public:
    explicit TreeProxyModel( QObject* parent = 0 );
    virtual ~TreeProxyModel() {}

    virtual void setSourcePlayableModel( TreeModel* model );
    virtual Tomahawk::playlistinterface_ptr playlistInterface();

    virtual void setFilter( const QString& pattern );

signals:

protected:
    bool filterAcceptsRow( int sourceRow, const QModelIndex& sourceParent ) const;
    bool lessThan( const QModelIndex& left, const QModelIndex& right ) const;

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
    DatabaseCommand_AllArtists* m_artistsFilterCmd;

    QString m_filter;
    TreeModel* m_model;

    Tomahawk::playlistinterface_ptr m_playlistInterface;
};

#endif // TREEPROXYMODEL_H
