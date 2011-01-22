#include "sourcesmodel.h"

#include <QMimeData>
#include <QTimer>
#include <QTreeView>
#include <QStandardItemModel>

#include "tomahawk/tomahawkapp.h"
#include "query.h"
#include "sourcelist.h"
#include "sourcetreeitem.h"
#include "utils/imagebutton.h"

using namespace Tomahawk;


SourcesModel::SourcesModel( QObject* parent )
    : QStandardItemModel( parent )
{
    setColumnCount( 1 );

    connect( SourceList::instance(), SIGNAL( sourceAdded( Tomahawk::source_ptr ) ), SLOT( onSourceAdded( Tomahawk::source_ptr ) ) );
    connect( SourceList::instance(), SIGNAL( sourceRemoved( Tomahawk::source_ptr ) ), SLOT( onSourceRemoved( Tomahawk::source_ptr ) ) );

    connect( parent, SIGNAL( onOnline( QModelIndex ) ), SLOT( onItemOnline( QModelIndex ) ) );
    connect( parent, SIGNAL( onOffline( QModelIndex ) ), SLOT( onItemOffline( QModelIndex ) ) );
}


QStringList
SourcesModel::mimeTypes() const
{
    QStringList types;
    types << "application/tomahawk.query.list";
    return types;
}


Qt::DropActions
SourcesModel::supportedDropActions() const
{
    return Qt::CopyAction;
}


Qt::ItemFlags
SourcesModel::flags( const QModelIndex& index ) const
{
    Qt::ItemFlags defaultFlags = QStandardItemModel::flags( index );

    if ( index.isValid() )
    {
        if ( indexType( index ) == 1 )
        {
            playlist_ptr playlist = indexToPlaylist( index );
            if ( !playlist.isNull() && playlist->author()->isLocal() )
                defaultFlags |= Qt::ItemIsEditable;
        }

        return Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | defaultFlags;
    }
    else
        return defaultFlags;
}


QVariant
SourcesModel::data( const QModelIndex& index, int role ) const
{
    if ( role == Qt::SizeHintRole )
    {
        return QSize( 0, 18 );
    }

    return QStandardItemModel::data( index, role );
}


void
SourcesModel::loadSources()
{
    QList<source_ptr> sources = SourceList::instance()->sources();

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


bool
SourcesModel::appendItem( const source_ptr& source )
{
    SourceTreeItem* item = new SourceTreeItem( source, this );
    connect( item, SIGNAL( clicked( QModelIndex ) ), this, SIGNAL( clicked( QModelIndex ) ) );

//    qDebug() << "Appending source item:" << item->source()->username();
    invisibleRootItem()->appendRow( item->columns() );

    ((QTreeView*)parent())->setIndexWidget( index( rowCount() - 1, 0 ), item->widget() );
    return true; // FIXME
}


bool
SourcesModel::removeItem( const source_ptr& source )
{
//    qDebug() << "Removing source item from SourceTree:" << source->username();

    QModelIndex idx;
    int rows = rowCount();
    for ( int row = 0; row < rows; row++ )
    {
        QModelIndex idx = index( row, 0 );
        SourceTreeItem* item = indexToTreeItem( idx );
        if ( item )
        {
            if ( item->source() == source )
            {
                qDebug() << "Found removed source item:" << item->source()->userName();
                invisibleRootItem()->removeRow( row );

                onItemOffline( idx );

                delete item;
                return true;
            }
        }
    }

    return false;
}


void
SourcesModel::onItemOnline( const QModelIndex& idx )
{
    qDebug() << Q_FUNC_INFO;

    SourceTreeItem* item = indexToTreeItem( idx );
    if ( item )
        item->onOnline();
}


void
SourcesModel::onItemOffline( const QModelIndex& idx )
{
    qDebug() << Q_FUNC_INFO;

    SourceTreeItem* item = indexToTreeItem( idx );
    if ( item )
        item->onOffline();
}


int
SourcesModel::indexType( const QModelIndex& index )
{
    if ( !index.isValid() )
        return -1;

    QModelIndex idx = index.model()->index( index.row(), 0, index.parent() );
    return idx.data( Qt::UserRole + 1 ).toInt();
}


playlist_ptr
SourcesModel::indexToPlaylist( const QModelIndex& index )
{
    playlist_ptr res;
    if ( !index.isValid() )
        return res;

    if ( indexType( index ) == 1 )
    {
        QModelIndex idx = index.model()->index( index.row(), 0, index.parent() );
        qlonglong pptr = idx.data( Qt::UserRole + 3 ).toLongLong();
        playlist_ptr* playlist = reinterpret_cast<playlist_ptr*>(pptr);
        if ( playlist )
            return *playlist;
    }

    return res;
}


SourceTreeItem*
SourcesModel::indexToTreeItem( const QModelIndex& index )
{
    if ( !index.isValid() )
        return 0;

    int type = indexType( index );
    if ( type == 0 || type == 1 )
    {
        QModelIndex idx = index.model()->index( index.row(), 0, index.parent() );
        qlonglong pptr = idx.data( Qt::UserRole + 2 ).toLongLong();
        SourceTreeItem* item = reinterpret_cast<SourceTreeItem*>(pptr);
        if ( item )
            return item;
    }

    return 0;
}


bool
SourcesModel::setData( const QModelIndex& index, const QVariant& value, int role )
{
    qDebug() << Q_FUNC_INFO;

    if ( !index.isValid() )
        return false;

    if ( indexType( index ) == 1 )
    {
        playlist_ptr playlist = indexToPlaylist( index );
        if ( !playlist.isNull() )
        {
            playlist->rename( value.toString() );
            QStandardItemModel::setData( index, value, Qt::DisplayRole );
        }

        return true;
    }

    return false;
}
