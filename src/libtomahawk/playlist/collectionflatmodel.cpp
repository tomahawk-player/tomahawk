#include "collectionflatmodel.h"

#include <QDebug>
#include <QMimeData>
#include <QTreeView>

#include "database/database.h"
#include "sourcelist.h"

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

    onTracksAdded( collection->tracks(), collection );

    connect( collection.data(), SIGNAL( tracksAdded( QList<Tomahawk::query_ptr>, Tomahawk::collection_ptr ) ),
                                  SLOT( onTracksAdded( QList<Tomahawk::query_ptr>, Tomahawk::collection_ptr ) ) );
    connect( collection.data(), SIGNAL( tracksFinished( Tomahawk::collection_ptr ) ),
                                  SLOT( onTracksAddingFinished( Tomahawk::collection_ptr ) ) );

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

    emit loadingStarts();

    DatabaseCommand_AllTracks* cmd = new DatabaseCommand_AllTracks( collection );
    cmd->setLimit( amount );
    cmd->setSortOrder( order );
    cmd->setSortDescending( true );

    connect( cmd, SIGNAL( tracks( QList<Tomahawk::query_ptr>, Tomahawk::collection_ptr ) ),
                    SLOT( onTracksAdded( QList<Tomahawk::query_ptr>, Tomahawk::collection_ptr ) ), Qt::QueuedConnection );

    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
}


void
CollectionFlatModel::removeCollection( const collection_ptr& collection )
{
    disconnect( collection.data(), SIGNAL( tracksAdded( QList<Tomahawk::query_ptr>, Tomahawk::collection_ptr ) ),
                this, SLOT( onTracksAdded( QList<Tomahawk::query_ptr>, Tomahawk::collection_ptr ) ) );
    disconnect( collection.data(), SIGNAL( tracksFinished( Tomahawk::collection_ptr ) ),
                this, SLOT( onTracksAddingFinished( Tomahawk::collection_ptr ) ) );

    QTime timer;
    timer.start();

//    QList<PlItem*> plitems = m_collectionIndex.values( collection );
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
            PlItem* item = itemFromIndex( index( i, 0, QModelIndex() ) );
            delete item;
        }
        emit endRemoveRows();
    }

    qDebug() << "Collection removed, time elapsed:" << timer.elapsed();

    emit trackCountChanged( rowCount( QModelIndex() ) );
}


void
CollectionFlatModel::onTracksAdded( const QList<Tomahawk::query_ptr>& tracks, const Tomahawk::collection_ptr& collection )
{
    if ( !tracks.count() )
    {
        emit trackCountChanged( rowCount( QModelIndex() ) );
        return;
    }

    int c = rowCount( QModelIndex() );
    QPair< int, int > crows;
    crows.first = c;
    crows.second = c + tracks.count() - 1;

    emit beginInsertRows( QModelIndex(), crows.first, crows.second );

    PlItem* plitem;
    foreach( const query_ptr& query, tracks )
    {
        plitem = new PlItem( query, m_rootItem );
        plitem->index = createIndex( m_rootItem->children.count() - 1, 0, plitem );

        connect( plitem, SIGNAL( dataChanged() ), SLOT( onDataChanged() ) );
    }

    m_collectionRows.insertMulti( collection, crows );
    emit endInsertRows();

    emit trackCountChanged( rowCount( QModelIndex() ) );
    qDebug() << Q_FUNC_INFO << rowCount( QModelIndex() );
}


void
CollectionFlatModel::onTracksAddingFinished( const Tomahawk::collection_ptr& collection )
{
    qDebug() << "Finished loading tracks" << collection->source()->friendlyName();

    emit loadingFinished();
}


void
CollectionFlatModel::onDataChanged()
{
    PlItem* p = (PlItem*)sender();
//    emit itemSizeChanged( p->index );

    if ( p )
        emit dataChanged( p->index, p->index.sibling( p->index.row(), columnCount() - 1 ) );
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
