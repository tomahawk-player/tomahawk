/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#ifndef ALBUMPROXYMODEL_H
#define ALBUMPROXYMODEL_H

#include <QSortFilterProxyModel>

#include "playlistinterface.h"
#include "playlist/albummodel.h"

#include "dllmacro.h"

class DLLEXPORT AlbumProxyModel : public QSortFilterProxyModel
{
Q_OBJECT

public:
    explicit AlbumProxyModel( QObject* parent = 0 );
    virtual ~AlbumProxyModel() {}

    virtual AlbumModel* sourceModel() const { return m_model; }
    virtual void setSourceAlbumModel( AlbumModel* sourceModel );
    virtual void setSourceModel( QAbstractItemModel* sourceModel );

    virtual int albumCount() const { return rowCount( QModelIndex() ); }

    virtual void removeIndex( const QModelIndex& index );
    virtual void removeIndexes( const QList<QModelIndex>& indexes );

    virtual void emitFilterChanged( const QString &pattern ) { emit filterChanged( pattern ); }

    virtual Tomahawk::playlistinterface_ptr getPlaylistInterface();

signals:
    void filterChanged( const QString& filter );

protected:
    bool filterAcceptsRow( int sourceRow, const QModelIndex& sourceParent ) const;
    bool lessThan( const QModelIndex& left, const QModelIndex& right ) const;

private:
    AlbumModel* m_model;

    Tomahawk::playlistinterface_ptr m_playlistInterface;
};

#endif // ALBUMPROXYMODEL_H
