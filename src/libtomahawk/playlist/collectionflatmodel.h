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

#ifndef COLLECTIONFLATMODEL_H
#define COLLECTIONFLATMODEL_H

#include <QAbstractItemModel>
#include <QList>
#include <QHash>

#include "trackmodelitem.h"
#include "trackmodel.h"
#include "collection.h"
#include "query.h"
#include "typedefs.h"
#include "playlist.h"
#include "playlistinterface.h"

#include "database/databasecommand_alltracks.h"

#include "dllmacro.h"

class QMetaData;

class DLLEXPORT CollectionFlatModel : public TrackModel
{
Q_OBJECT

public:
    explicit CollectionFlatModel( QObject* parent = 0 );
    ~CollectionFlatModel();

    virtual int trackCount() const { return rowCount( QModelIndex() ) + m_tracksToAdd.count(); }

    void addCollections( const QList< Tomahawk::collection_ptr >& collections );

    void addCollection( const Tomahawk::collection_ptr& collection, bool sendNotifications = true );
    void removeCollection( const Tomahawk::collection_ptr& collection );

    void addFilteredCollection( const Tomahawk::collection_ptr& collection, unsigned int amount, DatabaseCommand_AllTracks::SortOrder order );

    virtual void append( const Tomahawk::query_ptr& query ) {}

signals:
    void repeatModeChanged( PlaylistInterface::RepeatMode mode );
    void shuffleModeChanged( bool enabled );

    void itemSizeChanged( const QModelIndex& index );

private slots:
    void onDataChanged();

    void onTracksAdded( const QList<Tomahawk::query_ptr>& tracks );
    void onTracksRemoved( const QList<Tomahawk::query_ptr>& tracks );

    void onSourceOffline( const Tomahawk::source_ptr& src );

    void processTracksToAdd();

private:
    QMap< Tomahawk::collection_ptr, QPair< int, int > > m_collectionRows;
    QList<Tomahawk::query_ptr> m_tracksToAdd;
    // just to keep track of what we are waiting to be loaded
    QList<Tomahawk::Collection*> m_loadingCollections;
};

#endif // COLLECTIONFLATMODEL_H
