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

#include "albummodel.h"

#include <QListView>
#include <QMimeData>
#include <QNetworkReply>

#include "artist.h"
#include "albumitem.h"
#include "source.h"
#include "sourcelist.h"
#include "database/database.h"
#include "utils/tomahawkutils.h"
#include "utils/logger.h"

static QString s_tmInfoIdentifier = QString( "ALBUMMODEL" );

using namespace Tomahawk;


AlbumModel::AlbumModel( QObject* parent )
    : QAbstractItemModel( parent )
    , m_rootItem( new AlbumItem( 0, this ) )
    , m_overwriteOnAdd( false )
{
    qDebug() << Q_FUNC_INFO;

    connect( Tomahawk::InfoSystem::InfoSystem::instance(),
             SIGNAL( info( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ),
               SLOT( infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ) );

    connect( Tomahawk::InfoSystem::InfoSystem::instance(), SIGNAL( finished( QString ) ), SLOT( infoSystemFinished( QString ) ) );
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

    if ( role == Qt::DecorationRole )
    {
        return entry->cover;
    }

    if ( role != Qt::DisplayRole ) // && role != Qt::ToolTipRole )
        return QVariant();

    const album_ptr& album = entry->album();
    switch( index.column() )
    {
        case 0:
            return album->name();
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
    qDebug() << Q_FUNC_INFO;

    QByteArray queryData;
    QDataStream queryStream( &queryData, QIODevice::WriteOnly );

    foreach ( const QModelIndex& i, indexes )
    {
        if ( i.column() > 0 )
            continue;

        QModelIndex idx = index( i.row(), 0, i.parent() );
        AlbumItem* item = itemFromIndex( idx );
        if ( item )
        {
            const album_ptr& album = item->album();
            queryStream << album->artist()->name();
            queryStream << album->name();
        }
    }

    QMimeData* mimeData = new QMimeData();
    mimeData->setData( "application/tomahawk.metadata.album", queryData );

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

    if ( !albums.count() )
    {
        emit itemCountChanged( rowCount( QModelIndex() ) );
        return;
    }

    int c = rowCount( QModelIndex() );
    QPair< int, int > crows;
    crows.first = c;
    crows.second = c + albums.count() - 1;

    emit beginInsertRows( QModelIndex(), crows.first, crows.second );

    AlbumItem* albumitem;
    foreach( const album_ptr& album, albums )
    {
        albumitem = new AlbumItem( album, m_rootItem );
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


bool
AlbumModel::getCover( const QModelIndex& index )
{
    AlbumItem* item = itemFromIndex( index );
    if ( !item || !item->cover.isNull() )
        return false;

    Tomahawk::InfoSystem::InfoStringHash trackInfo;
    if ( !item->album()->artist().isNull() )
        trackInfo["artist"] = item->album()->artist()->name();
    trackInfo["album"] = item->album()->name();
    trackInfo["pptr"] = QString::number( (qlonglong)item );
    m_coverHash.insert( (qlonglong)item, index );

    Tomahawk::InfoSystem::InfoRequestData requestData;
    requestData.caller = s_tmInfoIdentifier;
    requestData.type = Tomahawk::InfoSystem::InfoAlbumCoverArt;
    requestData.input = QVariant::fromValue< Tomahawk::InfoSystem::InfoStringHash >( trackInfo );
    requestData.customData = QVariantMap();

    Tomahawk::InfoSystem::InfoSystem::instance()->getInfo( requestData );
    return true;
}


void
AlbumModel::infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData requestData, QVariant output )
{
//    qDebug() << Q_FUNC_INFO << " with caller " << requestData.caller;

    if ( requestData.caller != s_tmInfoIdentifier ||
       ( requestData.type != Tomahawk::InfoSystem::InfoAlbumCoverArt && requestData.type != Tomahawk::InfoSystem::InfoArtistImages ) )
    {
        return;
    }

    if ( !output.canConvert< QVariantMap >() )
    {
        qDebug() << "Cannot convert fetched art from a QByteArray";
        return;
    }

    Tomahawk::InfoSystem::InfoStringHash pptr = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash >();
    QVariantMap returnedData = output.value< QVariantMap >();
    const QByteArray ba = returnedData["imgbytes"].toByteArray();
    if ( ba.length() )
    {
        QPixmap pm;
        pm.loadFromData( ba );

        bool ok;
        qlonglong p = pptr["pptr"].toLongLong( &ok );
        AlbumItem* ai = itemFromIndex( m_coverHash.take( p ) );
        if ( !ai )
            return;

        if ( !pm.isNull() )
            ai->cover = pm;

        if ( ai->index.isValid() )
            emit dataChanged( ai->index, ai->index.sibling( ai->index.row(), columnCount( QModelIndex() ) - 1 ) );
    }
}


void
AlbumModel::infoSystemFinished( QString target )
{
    Q_UNUSED( target );
}


void
AlbumModel::onDataChanged()
{
    AlbumItem* p = (AlbumItem*)sender();
    emit dataChanged( p->index, p->index.sibling( p->index.row(), columnCount( QModelIndex() ) - 1 ) );
}
