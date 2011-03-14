
/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/


#include "sourcetree/sourcesmodel.h"

#include "sourcetree/sourcetreeitem.h"
#include "sourcelist.h"
#include "playlist.h"
#include "collection.h"
#include "source.h"

#include <QMimeData>
#include <QSize>

using namespace Tomahawk;

SourcesModel::SourcesModel( QObject* parent )
    : QAbstractItemModel( parent )
{
    m_rootItem = new SourceTreeItem( this, 0, Invalid );

    onSourcesAdded( SourceList::instance()->sources() );

    connect( SourceList::instance(), SIGNAL( sourceAdded( Tomahawk::source_ptr ) ), SLOT( onSourceAdded( Tomahawk::source_ptr ) ) );
    connect( SourceList::instance(), SIGNAL( sourceRemoved( Tomahawk::source_ptr ) ), SLOT( onSourceRemoved( Tomahawk::source_ptr ) ) );
}

SourcesModel::~SourcesModel()
{
    delete m_rootItem;
}


QVariant
SourcesModel::data( const QModelIndex& index, int role ) const
{
    if( !index.isValid() )
        return QVariant();

    switch( role )
    {
    case Qt::SizeHintRole:
        return QSize( 0, 18 );
    case SourceTreeItemRole:
        return QVariant::fromValue< SourceTreeItem* >( itemFromIndex( index ) );
    case SourceTreeItemTypeRole:
        return itemFromIndex( index )->type();
    case Qt::DisplayRole:
        return itemFromIndex( index )->text();
    }
    return QVariant();
}

int
SourcesModel::columnCount( const QModelIndex& parent ) const
{
    return 1;
}

int
SourcesModel::rowCount( const QModelIndex& parent ) const
{
    if( !parent.isValid() ) {
        return m_rootItem->children().count();
    }
//     qDebug() << "ASKING FOR AND RETURNING ROWCOUNT:" << parent.row() << parent.column() << parent.internalPointer() << itemFromIndex( parent )->children().count() << itemFromIndex( parent )->text();
    return itemFromIndex( parent )->children().count();
}

bool
SourcesModel::hasChildren( const QModelIndex& parent ) const
{
    return rowCount( parent ) > 0;
}


QModelIndex
SourcesModel::parent( const QModelIndex& child ) const
{
//     qDebug() << Q_FUNC_INFO << child;
    if( !child.isValid() ) {
        return QModelIndex();
    }

    SourceTreeItem* node = itemFromIndex( child );
    SourceTreeItem* parent = node->parent();
    if( parent == m_rootItem )
        return QModelIndex();

    return createIndex( rowForItem( node ), 0, parent );
}

QModelIndex
SourcesModel::index( int row, int column, const QModelIndex& parent ) const
{
//     qDebug() << "INDEX:" << row << column << parent;
    if( row < 0 || column < 0 )
        return QModelIndex();

    if( hasIndex( row, column, parent ) ) {
        SourceTreeItem *parentNode = itemFromIndex( parent );
        SourceTreeItem *childNode = parentNode->children().at( row );
//         qDebug() << "Making index with parent:" << parentNode->text() << "and index:" << childNode->text();
        return createIndex( row, column, childNode );
    }

    return QModelIndex();

}

bool
SourcesModel::setData( const QModelIndex& index, const QVariant& value, int role )
{
    // TODO
    return false;
}

QStringList
SourcesModel::mimeTypes() const
{
    QStringList types;
    types << "application/tomahawk.query.list";
    return types;
}

QMimeData*
SourcesModel::mimeData( const QModelIndexList& indexes ) const
{
    // TODO
    return new QMimeData();
}

bool
SourcesModel::dropMimeData( const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent )
{
    // TODO
    return false;
}

Qt::DropActions
SourcesModel::supportedDropActions() const
{
    return Qt::CopyAction;
}

Qt::ItemFlags
SourcesModel::flags( const QModelIndex& index ) const
{
    if ( index.isValid() ) {
        return itemFromIndex( index )->flags();
    } else {
        return 0;
    }
}


void
SourcesModel::appendItem( const Tomahawk::source_ptr& source )
{

    beginInsertRows( QModelIndex(), rowCount(), rowCount() );
    // append to end
    CollectionItem* item = new CollectionItem( this, m_rootItem, source );
    connect( item, SIGNAL( updated() ), this, SLOT( collectionUpdated() ) );

    m_rootItem->appendChild( item );
    endInsertRows();

    qDebug() << "Appending source item:" << item->source()->friendlyName();

    if ( !source.isNull() )
    {
        connect( source.data(), SIGNAL( stats( QVariantMap ) ), SLOT( onSourceChanged() ) );
        connect( source.data(), SIGNAL( playbackStarted( Tomahawk::query_ptr ) ), SLOT( onSourceChanged() ) );
        connect( source.data(), SIGNAL( stateChanged() ), SLOT( onSourceChanged() ) );

    }
}

bool
SourcesModel::removeItem( const Tomahawk::source_ptr& source )
{
    qDebug() << "Removing source item from SourceTree:" << source->friendlyName();

    QModelIndex idx;
    int rows = rowCount();
    for ( int row = 0; row < rows; row++ )
    {
        QModelIndex idx = index( row, 0, QModelIndex() );
        CollectionItem* item = static_cast< CollectionItem* >( idx.internalPointer() );
        if ( item && item->source() == source )
        {
            qDebug() << "Found removed source item:" << item->source()->userName();
            beginRemoveRows( QModelIndex(), row, row );
            m_rootItem->removeChild( item );
            endRemoveRows();

//             onItemOffline( idx );

            delete item;
            return true;
        }
    }

    return false;
}

void
SourcesModel::onSourceChanged() {

    Source* src = qobject_cast< Source* >( sender() );
    Q_ASSERT( src );

    qDebug() << "Searching for source item:" << src->friendlyName();

    for( int i = 0; i < rowCount(); i++ )
    {
        QModelIndex idx = index( i, 0 , QModelIndex() );

        if( idx.isValid() && itemFromIndex( idx ) && itemFromIndex( idx )->type() == Collection && // this is a source
            static_cast< CollectionItem* >( itemFromIndex( idx ) )->source().data() == src )       // and it is the one we want
        {
            qDebug() << "Found changed source, emitting dataChanged:" << src->friendlyName();
            emit dataChanged( idx, idx );
            return;
        }
    }
}

void
SourcesModel::loadSources()
{
    QList<source_ptr> sources = SourceList::instance()->sources();

    foreach( const source_ptr& source, sources )
        appendItem( source );
}


void
SourcesModel::onSourcesAdded( const QList<source_ptr>& sources )
{
    foreach( const source_ptr& source, sources )
        appendItem( source );
}


void
SourcesModel::onSourceAdded( const source_ptr& source )
{
    appendItem( source );
}


void
SourcesModel::onSourceRemoved( const source_ptr& source )
{
    removeItem( source );
}

void
SourcesModel::collectionUpdated()
{

}

void
SourcesModel::onItemRowsAddedBegin( int first, int last )
{

    Q_ASSERT( qobject_cast< SourceTreeItem* >( sender() ) );
    SourceTreeItem* item = qobject_cast< SourceTreeItem* >( sender() );

    if( !item )
        return;

    QModelIndex idx = indexFromItem( item );
    beginInsertRows( idx, first, last );
}

void
SourcesModel::onItemRowsAddedDone()
{
    Q_ASSERT( qobject_cast< SourceTreeItem* >( sender() ) );

    endInsertRows();
}

void
SourcesModel::onItemRowsRemovedBegin( int first, int last )
{
    Q_ASSERT( qobject_cast< SourceTreeItem* >( sender() ) );
    SourceTreeItem* item = qobject_cast< SourceTreeItem* >( sender() );

    if( !item )
        return;

    QModelIndex idx = indexFromItem( item );
    beginRemoveRows( idx, first, last );
}

void
SourcesModel::onItemRowsRemovedDone()
{
    Q_ASSERT( qobject_cast< SourceTreeItem* >( sender() ) );

    endRemoveRows();
}

SourceTreeItem*
SourcesModel::itemFromIndex( const QModelIndex& idx ) const
{
    if( !idx.isValid() )
        return m_rootItem;

    Q_ASSERT( idx.internalPointer() );

    return reinterpret_cast< SourceTreeItem* >( idx.internalPointer() );
}

QModelIndex
SourcesModel::indexFromItem( SourceTreeItem* item ) const
{
    if( !item || !item->parent() ) // should never happen..
        return QModelIndex();

    // reconstructs a modelindex from a sourcetreeitem that is somewhere in the tree
    // traverses the item to the root node, then rebuilds the qmodeindices from there back down
    // each int is the row of that item in the parent.
    /**
     * In this diagram, if the \param item is G, childIndexList will contain [0, 2, 0]
     *
     *    A
     *      D
     *      E
     *      F
     *        G
     *        H
     *    B
     *    C
     *
     **/
    QList< int > childIndexList;
    SourceTreeItem* curItem = item;
    while( curItem != m_rootItem ) {
        childIndexList << rowForItem( curItem );

        curItem = curItem->parent();
    }
    qDebug() << "build child index list:" << childIndexList;
    // now rebuild the qmodelindex we need
    QModelIndex idx;
    for( int i = childIndexList.size() - 1; i >= 0 ; i-- ) {
        idx = index( childIndexList[ i ], 0, idx );
    }
    qDebug() << "Got index from item:" << idx << idx.data( Qt::DisplayRole ).toString();

    return idx;
}


playlist_ptr
SourcesModel::playlistFromItem( SourceTreeItem* item ) const
{
    Q_ASSERT( item );
    Q_ASSERT( item->type() == StaticPlaylist );

    return dynamic_cast< PlaylistItem* >( item )->playlist();
}

int
SourcesModel::rowForItem( SourceTreeItem* item ) const
{
    return item->parent()->children().indexOf( item );
}
