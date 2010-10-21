#include "trackmodel.h"

#include <QDebug>
#include <QMimeData>
#include <QTreeView>

#include "tomahawk/tomahawkapp.h"

using namespace Tomahawk;


TrackModel::TrackModel( QObject* parent )
    : QAbstractItemModel( parent )
{
    qDebug() << Q_FUNC_INFO;
}


TrackModel::~TrackModel()
{
    delete m_rootItem;
}


QModelIndex
TrackModel::index( int row, int column, const QModelIndex& parent ) const
{
    if ( !m_rootItem || row < 0 || column < 0 )
        return QModelIndex();

    PlItem* parentItem = itemFromIndex( parent );
    PlItem* childItem = parentItem->children.value( row );
    if ( !childItem )
        return QModelIndex();

    return createIndex( row, column, childItem );
}


int
TrackModel::rowCount( const QModelIndex& parent ) const
{
    if ( parent.column() > 0 )
        return 0;

    PlItem* parentItem = itemFromIndex( parent );
    if ( !parentItem )
        return 0;

    return parentItem->children.count();
}


int
TrackModel::columnCount( const QModelIndex& parent ) const
{
    return 0;
}


QModelIndex
TrackModel::parent( const QModelIndex& child ) const
{
    PlItem* entry = itemFromIndex( child );
    if ( !entry )
        return QModelIndex();

    PlItem* parentEntry = entry->parent;
    if ( !parentEntry )
        return QModelIndex();

    PlItem* grandparentEntry = parentEntry->parent;
    if ( !grandparentEntry )
        return QModelIndex();

    int row = grandparentEntry->children.indexOf( parentEntry );
    return createIndex( row, 0, parentEntry );
}


QVariant
TrackModel::data( const QModelIndex& index, int role ) const
{
    return QVariant();
}


QVariant
TrackModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    return QVariant();
}


void
TrackModel::setCurrentItem( const QModelIndex& index )
{
    qDebug() << Q_FUNC_INFO;
    PlItem* oldEntry = itemFromIndex( m_currentIndex );
    if ( oldEntry )
    {
        oldEntry->setIsPlaying( false );
    }

    PlItem* entry = itemFromIndex( index );
    if ( entry )
    {
        m_currentIndex = index;
        entry->setIsPlaying( true );
    }
    else
    {
        m_currentIndex = QModelIndex();
    }
}


Qt::DropActions
TrackModel::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}


Qt::ItemFlags
TrackModel::flags( const QModelIndex& index ) const
{
    Qt::ItemFlags defaultFlags = QAbstractItemModel::flags( index );

    if ( index.isValid() )
        return Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | defaultFlags;
    else
        return Qt::ItemIsDropEnabled | defaultFlags;
}


QStringList
TrackModel::mimeTypes() const
{
    QStringList types;
    types << "application/tomahawk.query.list";
    return types;
}


QMimeData*
TrackModel::mimeData( const QModelIndexList &indexes ) const
{
    qDebug() << Q_FUNC_INFO;

    QByteArray queryData;
    QDataStream queryStream( &queryData, QIODevice::WriteOnly );

    foreach ( const QModelIndex& i, indexes )
    {
        if ( i.column() > 0 )
            continue;

        QModelIndex idx = index( i.row(), 0, i.parent() );
        PlItem* item = itemFromIndex( idx );
        if ( item )
        {
            const query_ptr& query = item->query();
            queryStream << qlonglong( &query );
        }
    }

    QMimeData* mimeData = new QMimeData();
    mimeData->setData( "application/tomahawk.query.list", queryData );

    return mimeData;
}
