/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include "albummodel.h"

#include <QListView>
#include <QMimeData>
#include <QNetworkReply>

#include "artist.h"
#include "albumitem.h"
#include "Source.h"
#include "sourcelist.h"
#include "database/database.h"
#include "utils/tomahawkutils.h"
#include "utils/logger.h"

using namespace Tomahawk;


AlbumModel::AlbumModel( QObject* parent )
    : QAbstractItemModel( parent )
    , m_rootItem( new AlbumItem( 0, this ) )
    , m_overwriteOnAdd( false )
{
}


AlbumModel::~AlbumModel()
{
    delete m_rootItem;
}


QModelIndex
AlbumModel::index( int row, int column, const QModelIndex& parent ) const
{
    if ( !m_rootItem || row < 0 || column < 0 )
        return QModelIndex();

    AlbumItem* parentItem = itemFromIndex( parent );
    AlbumItem* childItem = parentItem->children.value( row );
    if ( !childItem )
        return QModelIndex();

    return createIndex( row, column, childItem );
}


int
AlbumModel::rowCount( const QModelIndex& parent ) const
{
    if ( parent.column() > 0 )
        return 0;

    AlbumItem* parentItem = itemFromIndex( parent );
    if ( !parentItem )
        return 0;

    return parentItem->children.count();
}


int
AlbumModel::columnCount( const QModelIndex& parent ) const
{
    Q_UNUSED( parent );
    return 1;
}


QModelIndex
AlbumModel::parent( const QModelIndex& child ) const
{
    AlbumItem* entry = itemFromIndex( child );
    if ( !entry )
        return QModelIndex();

    AlbumItem* parentEntry = entry->parent;
    if ( !parentEntry )
        return QModelIndex();

    AlbumItem* grandparentEntry = parentEntry->parent;
    if ( !grandparentEntry )
        return QModelIndex();

    int row = grandparentEntry->children.indexOf( parentEntry );
    return createIndex( row, 0, parentEntry );
}


QVariant
AlbumModel::data( const QModelIndex& index, int role ) const
{
    if ( role == Qt::SizeHintRole )
    {
        return QSize( 116, 150 );
    }

    AlbumItem* entry = itemFromIndex( index );
    if ( !entry )
        return QVariant();

    if ( role != Qt::DisplayRole ) // && role != Qt::ToolTipRole )
        return QVariant();

    QString name;
    if ( !entry->album().isNull() )
        name = entry->album()->name();
    else if ( !entry->artist().isNull() )
        name = entry->artist()->name();

    switch( index.column() )
    {
        case 0:
            return name;
            break;

    }

    return QVariant();
}


QVariant
AlbumModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    QStringList headers;
    headers << tr( "Album" );
    if ( orientation == Qt::Horizontal && role == Qt::DisplayRole && section >= 0 )
    {
        return headers.at( section );
    }

    return QVariant();
}


Qt::ItemFlags
AlbumModel::flags( const QModelIndex& index ) const
{
    Qt::ItemFlags defaultFlags = QAbstractItemModel::flags( index );

    if ( index.isValid() && index.column() == 0 )
        return Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | defaultFlags;
    else
        return defaultFlags;
}


QStringList
AlbumModel::mimeTypes() const
{
    QStringList types;
    types << "application/tomahawk.query.list";
    return types;
}


QMimeData*
AlbumModel::mimeData( const QModelIndexList &indexes ) const
{
    QByteArray queryData;
    QDataStream queryStream( &queryData, QIODevice::WriteOnly );

    bool isAlbumData = true;
    foreach ( const QModelIndex& i, indexes )
    {
        if ( i.column() > 0 )
            continue;

        QModelIndex idx = index( i.row(), 0, i.parent() );
        AlbumItem* item = itemFromIndex( idx );
        if ( item && !item->album().isNull() )
        {
            const album_ptr& album = item->album();
            queryStream << album->artist()->name();
            queryStream << album->name();

            isAlbumData = true;
        }
        else if ( item && !item->artist().isNull() )
        {
            const artist_ptr& artist = item->artist();
            queryStream << artist->name();

            isAlbumData = false;
        }
    }

    QMimeData* mimeData = new QMimeData;
    mimeData->setData( isAlbumData ? "application/tomahawk.metadata.album" : "application/tomahawk.metadata.artist", queryData );

    return mimeData;
}


void
AlbumModel::removeIndex( const QModelIndex& index )
{
    qDebug() << Q_FUNC_INFO;

    if ( index.column() > 0 )
        return;

    AlbumItem* item = itemFromIndex( index );
    if ( item )
    {
        emit beginRemoveRows( index.parent(), index.row(), index.row() );
        delete item;
        emit endRemoveRows();
    }

    emit itemCountChanged( rowCount( QModelIndex() ) );
}


void
AlbumModel::removeIndexes( const QList<QModelIndex>& indexes )
{
    foreach( const QModelIndex& idx, indexes )
    {
        removeIndex( idx );
    }
}


void
AlbumModel::addCollection( const collection_ptr& collection, bool overwrite )
{
    qDebug() << Q_FUNC_INFO << collection->name()
                            << collection->source()->id()
                            << collection->source()->userName();

    DatabaseCommand_AllAlbums* cmd = new DatabaseCommand_AllAlbums( collection );
    m_overwriteOnAdd = overwrite;
    m_collection = collection;

    connect( cmd, SIGNAL( albums( QList<Tomahawk::album_ptr>, QVariant ) ),
                    SLOT( addAlbums( QList<Tomahawk::album_ptr> ) ) );

    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );

    m_title = tr( "All albums from %1" ).arg( collection->source()->friendlyName() );

    if ( collection.isNull() )
    {
        connect( SourceList::instance(), SIGNAL( sourceAdded( Tomahawk::source_ptr ) ), SLOT( onSourceAdded( Tomahawk::source_ptr ) ), Qt::UniqueConnection );

        QList<Tomahawk::source_ptr> sources = SourceList::instance()->sources();
        foreach ( const source_ptr& source, sources )
        {
            connect( source->collection().data(), SIGNAL( changed() ), SLOT( onCollectionChanged() ), Qt::UniqueConnection );
        }
    }
    else
    {
        connect( collection.data(), SIGNAL( changed() ), SLOT( onCollectionChanged() ), Qt::UniqueConnection );
    }

    emit loadingStarted();
}


void
AlbumModel::addFilteredCollection( const collection_ptr& collection, unsigned int amount, DatabaseCommand_AllAlbums::SortOrder order, bool overwrite )
{
/*    qDebug() << Q_FUNC_INFO << collection->name()
                            << collection->source()->id()
                            << collection->source()->userName()
                            << amount << order;*/

    DatabaseCommand_AllAlbums* cmd = new DatabaseCommand_AllAlbums( collection );
    cmd->setLimit( amount );
    cmd->setSortOrder( order );
    cmd->setSortDescending( true );
    m_overwriteOnAdd = overwrite;
    m_collection = collection;

    connect( cmd, SIGNAL( albums( QList<Tomahawk::album_ptr>, QVariant ) ),
                    SLOT( addAlbums( QList<Tomahawk::album_ptr> ) ) );

    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );

    if ( !collection.isNull() )
        m_title = tr( "All albums from %1" ).arg( collection->source()->friendlyName() );
    else
        m_title = tr( "All albums" );

    emit loadingStarted();
}


void
AlbumModel::addAlbums( const QList<Tomahawk::album_ptr>& albums )
{
    emit loadingFinished();

    if ( m_overwriteOnAdd )
        clear();

    QList<Tomahawk::album_ptr> trimmedAlbums;
    foreach ( const album_ptr& album, albums )
    {
        if ( !album.isNull() && album->name().length() )
        {
            if ( findItem( album ) || trimmedAlbums.contains( album ) )
                continue;
            trimmedAlbums << album;
        }
    }

    if ( !trimmedAlbums.count() )
    {
        emit itemCountChanged( rowCount( QModelIndex() ) );
        return;
    }

    int c = rowCount( QModelIndex() );
    QPair< int, int > crows;
    crows.first = c;
    crows.second = c + trimmedAlbums.count() - 1;

    emit beginInsertRows( QModelIndex(), crows.first, crows.second );

    AlbumItem* albumitem;
    foreach( const album_ptr& album, trimmedAlbums )
    {
        albumitem = new AlbumItem( album, m_rootItem );
        albumitem->index = createIndex( m_rootItem->children.count() - 1, 0, albumitem );

        connect( albumitem, SIGNAL( dataChanged() ), SLOT( onDataChanged() ) );
    }

    emit endInsertRows();
    emit itemCountChanged( rowCount( QModelIndex() ) );
}


void
AlbumModel::addArtists( const QList<Tomahawk::artist_ptr>& artists )
{
    emit loadingFinished();

    if ( m_overwriteOnAdd )
        clear();

    QList<Tomahawk::artist_ptr> trimmedArtists;
    foreach ( const artist_ptr& artist, artists )
    {
        if ( !artist.isNull() && artist->name().length() )
        {
            if ( findItem( artist ) || trimmedArtists.contains( artist ) )
                continue;
            trimmedArtists << artist;
        }
    }

    if ( !trimmedArtists.count() )
    {
        emit itemCountChanged( rowCount( QModelIndex() ) );
        return;
    }

    int c = rowCount( QModelIndex() );
    QPair< int, int > crows;
    crows.first = c;
    crows.second = c + trimmedArtists.count() - 1;

    emit beginInsertRows( QModelIndex(), crows.first, crows.second );

    AlbumItem* albumitem;
    foreach ( const artist_ptr& artist, trimmedArtists )
    {
        albumitem = new AlbumItem( artist, m_rootItem );
        albumitem->index = createIndex( m_rootItem->children.count() - 1, 0, albumitem );

        connect( albumitem, SIGNAL( dataChanged() ), SLOT( onDataChanged() ) );
    }

    emit endInsertRows();
    emit itemCountChanged( rowCount( QModelIndex() ) );
}


void
AlbumModel::onSourceAdded( const Tomahawk::source_ptr& source )
{
    connect( source->collection().data(), SIGNAL( changed() ), SLOT( onCollectionChanged() ), Qt::UniqueConnection );
}


void
AlbumModel::onCollectionChanged()
{
    addCollection( m_collection, true );
}


void
AlbumModel::clear()
{
    beginResetModel();
    delete m_rootItem;
    m_rootItem = new AlbumItem( 0, this );
    endResetModel();
}


void
AlbumModel::onDataChanged()
{
    AlbumItem* p = (AlbumItem*)sender();
    emit dataChanged( p->index, p->index.sibling( p->index.row(), columnCount( QModelIndex() ) - 1 ) );
}


AlbumItem*
AlbumModel::findItem( const artist_ptr& artist ) const
{
    for ( int i = 0; i < rowCount( QModelIndex() ); i++ )
    {
        AlbumItem* item = itemFromIndex( index( i, 0, QModelIndex() ) );
        if ( !item->artist().isNull() && item->artist() == artist )
        {
            return item;
        }
    }

    return 0;
}


AlbumItem*
AlbumModel::findItem( const album_ptr& album ) const
{
    for ( int i = 0; i < rowCount( QModelIndex() ); i++ )
    {
        AlbumItem* item = itemFromIndex( index( i, 0, QModelIndex() ) );
        if ( !item->album().isNull() && item->album() == album )
        {
            return item;
        }
    }

    return 0;
}
