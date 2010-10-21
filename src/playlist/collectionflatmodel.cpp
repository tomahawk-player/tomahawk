#include "collectionflatmodel.h"

#include <QDebug>
#include <QMimeData>
#include <QTreeView>

using namespace Tomahawk;


CollectionFlatModel::CollectionFlatModel( QObject* parent )
    : TrackModel( parent )
{
    qDebug() << Q_FUNC_INFO;
}


CollectionFlatModel::~CollectionFlatModel()
{
    delete m_rootItem;
}


int
CollectionFlatModel::columnCount( const QModelIndex& parent ) const
{
    return 6;
}


QVariant
CollectionFlatModel::data( const QModelIndex& index, int role ) const
{
    PlItem* entry = itemFromIndex( index );
    if ( !entry )
        return QVariant();

    if ( role == Qt::DecorationRole )
    {
        if ( index.column() == 0 && entry->isPlaying() )
            return QString( RESPATH "images/now-playing-speaker.png" );

        return QVariant();
    }

    if ( role == Qt::SizeHintRole )
    {
        return QSize( 0, 18 );
    }

    if ( role != Qt::DisplayRole )
        return QVariant();

    const query_ptr& query = entry->query();
    if ( query.isNull() )
    {
        if ( !index.column() )
        {
            return entry->caption.isEmpty() ? "Unknown" : entry->caption;
        }

        if ( index.column() == 1 )
        {
            return entry->childCount;
        }

        return QVariant( "" );
    }

    if ( !query->numResults() )
    {
        switch( index.column() )
        {
            case 0:
                return query->artist();
                break;

            case 1:
                return query->track();
                break;

            case 2:
                return query->album();
                break;
        }
    }
    else
    {
        switch( index.column() )
        {
            case 0:
                return query->results().first()->artist();
                break;

            case 1:
                return query->results().first()->track();
                break;

            case 2:
                return query->results().first()->album();
                break;

            case 3:
                return TomahawkUtils::timeToString( query->results().first()->duration() );
                break;

            case 4:
                return query->results().first()->bitrate();
                break;

            case 5:
                return query->results().first()->collection()->source()->friendlyName();
                break;
        }
    }

    return QVariant();
}


QVariant
CollectionFlatModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    QStringList headers;
    headers << tr( "Artist" ) << tr( "Track" ) << tr( "Album" ) << tr( "Duration" ) << tr( "Bitrate" ) << tr( "Origin" );
    if ( orientation == Qt::Horizontal && role == Qt::DisplayRole && section >= 0 )
    {
        return headers.at( section );
    }

    return QVariant();
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
    QList<int> rows;

    if ( plitems.count() )
    {
        foreach( PlItem* item, plitems )
        {
            rows << item->index.row();
        }

        qSort( rows );
        int start = rows.first();
        int end = rows.first();

        foreach( int i, rows )
        {
            if ( i > end + 1 )
            {
                qDebug() << start << end;
                emit beginRemoveRows( plitems.first()->parent->index, start, end );
                emit endRemoveRows();

                start = i;
            }

            end = i;
        }

        qDebug() << start << end;

        emit beginRemoveRows( plitems.first()->parent->index, start, end );
        emit endRemoveRows();

        qDeleteAll( plitems );
    }

    m_collectionIndex.remove( collection );
}


void
CollectionFlatModel::onTracksAdded( const QList<QVariant>& tracks, const collection_ptr& collection )
{
    int c = rowCount( QModelIndex() );
    emit beginInsertRows( QModelIndex(), c, c + tracks.count() - 1 );

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
