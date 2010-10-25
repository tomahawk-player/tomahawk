#include "collectionflatmodel.h"

#include <QDebug>
#include <QMimeData>
#include <QTreeView>

using namespace Tomahawk;


CollectionFlatModel::CollectionFlatModel( QObject* parent )
    : TrackModel( parent )
{
    qDebug() << Q_FUNC_INFO;
    m_rootItem = new PlItem( 0, this );

    connect( &APP->sourcelist(), SIGNAL( sourceRemoved( Tomahawk::source_ptr ) ), SLOT( onSourceOffline( Tomahawk::source_ptr ) ) );
}


CollectionFlatModel::~CollectionFlatModel()
{
}


int
CollectionFlatModel::columnCount( const QModelIndex& parent ) const
{
    return TrackModel::columnCount( parent );
}


QVariant
CollectionFlatModel::data( const QModelIndex& index, int role ) const
{
    return TrackModel::data( index, role );
}


QVariant
CollectionFlatModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    return TrackModel::headerData( section, orientation, role );
}


void
CollectionFlatModel::addCollection( const collection_ptr& collection )
{
    qDebug() << Q_FUNC_INFO << collection->name()
                            << collection->source()->id()
                            << collection->source()->userName();

    emit loadingStarts();

    connect( collection.data(), SIGNAL( tracksAdded( QList<QVariant>, Tomahawk::collection_ptr ) ),
             SLOT( onTracksAdded( QList<QVariant>, Tomahawk::collection_ptr ) ) );
    connect( collection.data(), SIGNAL( tracksFinished( Tomahawk::collection_ptr ) ),
             SLOT( onTracksAddingFinished( Tomahawk::collection_ptr ) ) );
}


void
CollectionFlatModel::removeCollection( const collection_ptr& collection )
{
    disconnect( collection.data(), SIGNAL( tracksAdded( QList<QVariant>, Tomahawk::collection_ptr ) ),
                this, SLOT( onTracksAdded( QList<QVariant>, Tomahawk::collection_ptr ) ) );
    disconnect( collection.data(), SIGNAL( tracksFinished( Tomahawk::collection_ptr ) ),
                this, SLOT( onTracksAddingFinished( Tomahawk::collection_ptr ) ) );

    QList<PlItem*> plitems = m_collectionIndex.values( collection );
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
        emit endRemoveRows();
    }

    qDeleteAll( plitems );
    m_collectionIndex.remove( collection );
}


void
CollectionFlatModel::onTracksAdded( const QList<QVariant>& tracks, const collection_ptr& collection )
{
    int c = rowCount( QModelIndex() );
    QPair< int, int > crows;
    crows.first = c;
    crows.second = c + tracks.count() - 1;

    emit beginInsertRows( QModelIndex(), crows.first, crows.second );

    PlItem* plitem;
    foreach( const QVariant& v, tracks )
    {
        Tomahawk::query_ptr query = query_ptr( new Query( v ) );

        // FIXME: needs merging
        // Manually add a result, since it's coming from the local collection
        QVariantMap t = query->toVariant().toMap();
        t["score"] = 1.0;
        QList<result_ptr> results;
        result_ptr result = result_ptr( new Result( t, collection ) );
        results << result;
        query->addResults( results );

        plitem = new PlItem( query, m_rootItem );
        plitem->index = createIndex( m_rootItem->children.count() - 1, 0, plitem );

        connect( plitem, SIGNAL( dataChanged() ), SLOT( onDataChanged() ) );

        m_collectionIndex.insertMulti( collection, plitem );
    }

    m_collectionRows.insertMulti( collection, crows );
    emit endInsertRows();

    emit trackCountChanged( rowCount( QModelIndex() ) );
    qDebug() << rowCount( QModelIndex() );
}


void
CollectionFlatModel::onTracksAddingFinished( const Tomahawk::collection_ptr& /* collection */ )
{
    qDebug() << "Finished loading tracks";
    emit loadingFinished();
}


void
CollectionFlatModel::onDataChanged()
{
    PlItem* p = (PlItem*)sender();
//    emit itemSizeChanged( p->index );
    emit dataChanged( p->index, p->index.sibling( p->index.row(), columnCount() - 1 ) );
}


void
CollectionFlatModel::onSourceOffline( const Tomahawk::source_ptr& src )
{
    qDebug() << Q_FUNC_INFO;

    if ( m_collectionIndex.contains( src->collection() ) )
    {
        removeCollection( src->collection() );
    }
}
