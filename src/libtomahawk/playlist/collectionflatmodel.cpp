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

#include "collectionflatmodel.h"

#include <QMimeData>
#include <QTreeView>

#include "database/database.h"
#include "sourcelist.h"
#include "utils/logger.h"

using namespace Tomahawk;


CollectionFlatModel::CollectionFlatModel( QObject* parent )
    : TrackModel( parent )
{
    qDebug() << Q_FUNC_INFO;

    connect( SourceList::instance(), SIGNAL( sourceRemoved( Tomahawk::source_ptr ) ), SLOT( onSourceOffline( Tomahawk::source_ptr ) ) );
}


CollectionFlatModel::~CollectionFlatModel()
{
}


void
CollectionFlatModel::addCollections( const QList< collection_ptr >& collections )
{
    qDebug() << Q_FUNC_INFO << "Adding collections!";
    foreach( const collection_ptr& col, collections )
    {
        addCollection( col );
    }

    // we are waiting for some to load
    if( !m_loadingCollections.isEmpty() )
        emit loadingStarted();
}


void
CollectionFlatModel::addCollection( const collection_ptr& collection, bool sendNotifications )
{
    qDebug() << Q_FUNC_INFO << collection->name()
                            << collection->source()->id()
                            << collection->source()->userName();

    connect( collection.data(), SIGNAL( tracksAdded( QList<Tomahawk::query_ptr> ) ),
                                  SLOT( onTracksAdded( QList<Tomahawk::query_ptr> ) ) );
    connect( collection.data(), SIGNAL( tracksRemoved( QList<Tomahawk::query_ptr> ) ),
                                  SLOT( onTracksRemoved( QList<Tomahawk::query_ptr> ) ) );

    if( sendNotifications )
        emit loadingStarted();

    if ( collection->isLoaded() )
    {
        onTracksAdded( collection->tracks() );
    }
    else
    {
        collection->tracks(); // data will arrive via signals
        m_loadingCollections << collection.data();
    }

    if ( collection->source()->isLocal() )
        setTitle( tr( "Your Collection" ) );
    else
        setTitle( tr( "Collection of %1" ).arg( collection->source()->friendlyName() ) );
}


void
CollectionFlatModel::addFilteredCollection( const collection_ptr& collection, unsigned int amount, DatabaseCommand_AllTracks::SortOrder order )
{
    qDebug() << Q_FUNC_INFO << collection->name()
                            << collection->source()->id()
                            << collection->source()->userName()
                            << amount << order;

    DatabaseCommand_AllTracks* cmd = new DatabaseCommand_AllTracks( collection );
    cmd->setLimit( amount );
    cmd->setSortOrder( order );
    cmd->setSortDescending( true );

    connect( cmd, SIGNAL( tracks( QList<Tomahawk::query_ptr>, QVariant ) ),
                    SLOT( onTracksAdded( QList<Tomahawk::query_ptr> ) ), Qt::QueuedConnection );

    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
}


void
CollectionFlatModel::removeCollection( const collection_ptr& collection )
{
    return; // FIXME

    disconnect( collection.data(), SIGNAL( tracksAdded( QList<Tomahawk::query_ptr> ) ),
                this, SLOT( onTracksAdded( QList<Tomahawk::query_ptr> ) ) );

    QTime timer;
    timer.start();

//    QList<TrackModelItem*> plitems = m_collectionIndex.values( collection );
    QList< QPair< int, int > > rows;
    QList< QPair< int, int > > sortrows;
    QPair< int, int > row;
    QPair< int, int > rowf;
    rows = m_collectionRows.values( collection );

    while ( rows.count() )
    {
        int x = -1;
        int j = 0;
        foreach( row, rows )
        {
            if ( x < 0 || row.first > rows.at( x ).first )
                x = j;

            j++;
        }

        sortrows.append( rows.at( x ) );
        rows.removeAt( x );
    }

    foreach( row, sortrows )
    {
        QMap< Tomahawk::collection_ptr, QPair< int, int > > newrows;
        foreach ( const collection_ptr& col, m_collectionRows.uniqueKeys() )
        {
            if ( col.data() == collection.data() )
                continue;

            foreach ( rowf, m_collectionRows.values( col ) )
            {
                if ( rowf.first > row.first )
                {
                    rowf.first -= ( row.second - row.first ) + 1;
                    rowf.second -= ( row.second - row.first ) + 1;
                }
                newrows.insertMulti( col, rowf );
            }
        }
        m_collectionRows = newrows;

        qDebug() << "Removing rows:" << row.first << row.second;
        emit beginRemoveRows( QModelIndex(), row.first, row.second );
        for ( int i = row.second; i >= row.first; i-- )
        {
            TrackModelItem* item = itemFromIndex( index( i, 0, QModelIndex() ) );
            delete item;
        }
        emit endRemoveRows();
    }

    qDebug() << "Collection removed, time elapsed:" << timer.elapsed();

//    emit trackCountChanged( rowCount( QModelIndex() ) );
}


void
CollectionFlatModel::onTracksAdded( const QList<Tomahawk::query_ptr>& tracks )
{
    qDebug() << Q_FUNC_INFO << tracks.count() << rowCount( QModelIndex() );

    if( !m_loadingCollections.isEmpty() && sender() && qobject_cast< Collection* >( sender() ) )
    {
        // we are keeping track and are called as a slot
        m_loadingCollections.removeAll( qobject_cast< Collection* >( sender() ) );
    }

    bool kickOff = m_tracksToAdd.isEmpty();
    m_tracksToAdd << tracks;

    emit trackCountChanged( trackCount() );

    if ( m_tracksToAdd.count() && kickOff )
        processTracksToAdd();
    else if ( m_tracksToAdd.isEmpty() && m_loadingCollections.isEmpty() )
        emit loadingFinished();
}


void
CollectionFlatModel::processTracksToAdd()
{
    int chunkSize = 5000;
    int maxc = qMin( chunkSize, m_tracksToAdd.count() );
    int c = rowCount( QModelIndex() );

    emit beginInsertRows( QModelIndex(), c, c + maxc - 1 );
    //beginResetModel();

    TrackModelItem* plitem;
    QList< Tomahawk::query_ptr >::iterator iter = m_tracksToAdd.begin();

    for( int i = 0; i < maxc; ++i )
    {
        plitem = new TrackModelItem( *iter, m_rootItem );
        plitem->index = createIndex( m_rootItem->children.count() - 1, 0, plitem );

        connect( plitem, SIGNAL( dataChanged() ), SLOT( onDataChanged() ) );

        ++iter;
    }

    m_tracksToAdd.erase( m_tracksToAdd.begin(), iter );

    if ( m_tracksToAdd.isEmpty() && m_loadingCollections.isEmpty() )
        emit loadingFinished();

    //endResetModel();
    emit endInsertRows();
    qDebug() << Q_FUNC_INFO << rowCount( QModelIndex() );

    if ( m_tracksToAdd.count() )
        QTimer::singleShot( 250, this, SLOT( processTracksToAdd() ) );
}


void
CollectionFlatModel::onTracksRemoved( const QList<Tomahawk::query_ptr>& tracks )
{
    QList<Tomahawk::query_ptr> t = tracks;
    for ( int i = rowCount( QModelIndex() ); i >= 0 && t.count(); i-- )
    {
        TrackModelItem* item = itemFromIndex( index( i, 0, QModelIndex() ) );
        if ( !item )
            continue;

        int j = 0;
        foreach ( const query_ptr& query, t )
        {
            if ( item->query().data() == query.data() )
            {
                qDebug() << "Removing row:" << i << query->toString();
                emit beginRemoveRows( QModelIndex(), i, i );
                delete item;
                emit endRemoveRows();

                t.removeAt( j );
                break;
            }

            j++;
        }
    }

//    emit trackCountChanged( rowCount( QModelIndex() ) );
    qDebug() << Q_FUNC_INFO << rowCount( QModelIndex() );
}


void
CollectionFlatModel::onDataChanged()
{
    TrackModelItem* p = (TrackModelItem*)sender();
//    emit itemSizeChanged( p->index );

    if ( p )
        emit dataChanged( p->index, p->index.sibling( p->index.row(), columnCount( QModelIndex() ) - 1 ) );
}


void
CollectionFlatModel::onSourceOffline( const Tomahawk::source_ptr& src )
{
    qDebug() << Q_FUNC_INFO;

    if ( m_collectionRows.contains( src->collection() ) )
    {
        removeCollection( src->collection() );
    }
}
