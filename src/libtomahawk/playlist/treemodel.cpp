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

#include "treemodel.h"

#include <QDebug>
#include <QListView>
#include <QMimeData>
#include <QNetworkReply>

#include "database/databasecommand_allalbums.h"
#include "database/databasecommand_alltracks.h"
#include "database/database.h"
#include "utils/tomahawkutils.h"

static QString s_tmInfoIdentifier = QString( "TREEMODEL" );

using namespace Tomahawk;


TreeModel::TreeModel( QObject* parent )
    : QAbstractItemModel( parent )
    , m_rootItem( new TreeModelItem( 0, this ) )
{
    qDebug() << Q_FUNC_INFO;

    m_defaultCover = QPixmap( RESPATH "images/no-album-art-placeholder.png" )
                     .scaled( QSize( 120, 120 ), Qt::IgnoreAspectRatio, Qt::SmoothTransformation );

    connect( Tomahawk::InfoSystem::InfoSystem::instance(),
             SIGNAL( info( QString, Tomahawk::InfoSystem::InfoType, QVariant, QVariant, Tomahawk::InfoSystem::InfoCustomData ) ),
               SLOT( infoSystemInfo( QString, Tomahawk::InfoSystem::InfoType, QVariant, QVariant, Tomahawk::InfoSystem::InfoCustomData ) ) );

    connect( Tomahawk::InfoSystem::InfoSystem::instance(), SIGNAL( finished( QString ) ), SLOT( infoSystemFinished( QString ) ) );
}


TreeModel::~TreeModel()
{
}


QModelIndex
TreeModel::index( int row, int column, const QModelIndex& parent ) const
{
    if ( !m_rootItem || row < 0 || column < 0 )
        return QModelIndex();

    TreeModelItem* parentItem = itemFromIndex( parent );
    TreeModelItem* childItem = parentItem->children.value( row );
    if ( !childItem )
        return QModelIndex();

    return createIndex( row, column, childItem );
}


bool
TreeModel::canFetchMore( const QModelIndex& parent ) const
{
    TreeModelItem* parentItem = itemFromIndex( parent );

    if ( parentItem->fetchingMore )
        return false;

    if ( !parentItem->artist().isNull() )
    {
        return true;
    }
    else if ( !parentItem->album().isNull() )
    {
        return true;
    }

    return false;
}


void
TreeModel::fetchMore( const QModelIndex& parent )
{
    TreeModelItem* parentItem = itemFromIndex( parent );
    if ( !parentItem || parentItem->fetchingMore )
        return;

    parentItem->fetchingMore = true;
    if ( !parentItem->artist().isNull() )
    {
        qDebug() << Q_FUNC_INFO << "Loading Artist:" << parentItem->artist()->name();
        addAlbums( parentItem->artist(), parent );
    }
    else if ( !parentItem->album().isNull() )
    {
        qDebug() << Q_FUNC_INFO << "Loading Album:" << parentItem->album()->name();
        addTracks( parentItem->album(), parent );
    }
    else
        Q_ASSERT( false );
}


int
TreeModel::rowCount( const QModelIndex& parent ) const
{
    if ( parent.column() > 0 )
        return 0;

    TreeModelItem* parentItem = itemFromIndex( parent );
    if ( !parentItem )
        return 0;

    if ( !parentItem->artist().isNull() || !parentItem->album().isNull() )
    {
        if ( !parentItem->children.count() )
            return 1;
    }

    return parentItem->children.count();
}


int
TreeModel::columnCount( const QModelIndex& parent ) const
{
    Q_UNUSED( parent );
    return 7;
}


QModelIndex
TreeModel::parent( const QModelIndex& child ) const
{
    TreeModelItem* entry = itemFromIndex( child );
    if ( !entry )
        return QModelIndex();

    TreeModelItem* parentEntry = entry->parent;
    if ( !parentEntry )
        return QModelIndex();

    TreeModelItem* grandparentEntry = parentEntry->parent;
    if ( !grandparentEntry )
        return QModelIndex();

    int row = grandparentEntry->children.indexOf( parentEntry );
    return createIndex( row, 0, parentEntry );
}


QVariant
TreeModel::data( const QModelIndex& index, int role ) const
{
    TreeModelItem* entry = itemFromIndex( index );
    if ( !entry )
        return QVariant();

    if ( role == Qt::SizeHintRole )
    {
        if ( !entry->result().isNull() )
        {
            return QSize( 128, 20 );
        }
        else if ( !entry->album().isNull() )
        {
            return QSize( 128, 32 );
        }
        else if ( !entry->artist().isNull() )
        {
            return QSize( 128, 44 );
        }

        return QSize( 128, 0 );
    }

    if ( role != Qt::DisplayRole ) // && role != Qt::ToolTipRole )
        return QVariant();

    if ( !entry->artist().isNull() && index.column() == Name )
    {
        return entry->artist()->name();
    }
    else if ( !entry->album().isNull() && index.column() == Name )
    {
        return entry->album()->name();
    }
    else
    {
        const result_ptr& result = entry->result();
        switch( index.column() )
        {
            case Name:
                return QString( "%1%2" ).arg( result->albumpos() > 0 ? QString( "%1. ").arg( result->albumpos() ) : QString() )
                                        .arg( result->track() );

            case Duration:
                return TomahawkUtils::timeToString( result->duration() );

            case Bitrate:
                return result->bitrate();

            case Age:
                return TomahawkUtils::ageToString( QDateTime::fromTime_t( result->modificationTime() ) );

            case Year:
                return result->year();

            case Filesize:
                return TomahawkUtils::filesizeToString( result->size() );

            case Origin:
                return result->friendlySource();

            case AlbumPosition:
                return result->albumpos();

            default:
                return QVariant();
        }
    }

    return QVariant();
}


QVariant
TreeModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    QStringList headers;
    headers << tr( "Name" ) << tr( "Duration" ) << tr( "Bitrate" ) << tr( "Age" ) << tr( "Year" ) << tr( "Size" ) << tr( "Origin" );
    if ( orientation == Qt::Horizontal && role == Qt::DisplayRole && section >= 0 )
    {
        return headers.at( section );
    }

    return QVariant();
}


Qt::ItemFlags
TreeModel::flags( const QModelIndex& index ) const
{
    Qt::ItemFlags defaultFlags = QAbstractItemModel::flags( index );

    if ( index.isValid() && index.column() == 0 )
        return Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | defaultFlags;
    else
        return defaultFlags;
}


QStringList
TreeModel::mimeTypes() const
{
    QStringList types;
    types << "application/tomahawk.query.list";
    return types;
}


QMimeData*
TreeModel::mimeData( const QModelIndexList &indexes ) const
{
    qDebug() << Q_FUNC_INFO;

    QByteArray queryData;
    QDataStream queryStream( &queryData, QIODevice::WriteOnly );

    foreach ( const QModelIndex& i, indexes )
    {
        if ( i.column() > 0 )
            continue;

        QModelIndex idx = index( i.row(), 0, i.parent() );
        TreeModelItem* item = itemFromIndex( idx );
        if ( item )
        {
            const album_ptr& album = item->album();
            queryStream << qlonglong( &album );
        }
    }

    QMimeData* mimeData = new QMimeData();
    mimeData->setData( "application/tomahawk.query.list", queryData );

    return mimeData;
}


void
TreeModel::removeIndex( const QModelIndex& index )
{
    qDebug() << Q_FUNC_INFO;

    if ( index.column() > 0 )
        return;

    TreeModelItem* item = itemFromIndex( index );
    if ( item )
    {
        emit beginRemoveRows( index.parent(), index.row(), index.row() );
        delete item;
        emit endRemoveRows();
    }
}


void
TreeModel::removeIndexes( const QList<QModelIndex>& indexes )
{
    foreach( const QModelIndex& idx, indexes )
    {
        removeIndex( idx );
    }
}


void
TreeModel::addAllCollections()
{
    qDebug() << Q_FUNC_INFO;

    DatabaseCommand_AllArtists* cmd = new DatabaseCommand_AllArtists();

    connect( cmd, SIGNAL( artists( QList<Tomahawk::artist_ptr> ) ),
                    SLOT( onArtistsAdded( QList<Tomahawk::artist_ptr> ) ) );

    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );

    m_title = tr( "All Artists" );
}


void
TreeModel::addAlbums( const artist_ptr& artist, const QModelIndex& parent )
{
    qDebug() << Q_FUNC_INFO;

    DatabaseCommand_AllAlbums* cmd = new DatabaseCommand_AllAlbums( m_collection, artist );
    cmd->setData( parent.row() );

    connect( cmd, SIGNAL( albums( QList<Tomahawk::album_ptr>, QVariant ) ),
                    SLOT( onAlbumsAdded( QList<Tomahawk::album_ptr>, QVariant ) ) );

    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
}


void
TreeModel::addTracks( const album_ptr& album, const QModelIndex& parent )
{
    qDebug() << Q_FUNC_INFO;

    DatabaseCommand_AllTracks* cmd = new DatabaseCommand_AllTracks( m_collection );
    cmd->setAlbum( album.data() );
//    cmd->setArtist( album->artist().data() );

    QList< QVariant > rows;
    rows << parent.row();
    rows << parent.parent().row();
    cmd->setData( QVariant( rows ) );

    connect( cmd, SIGNAL( tracks( QList<Tomahawk::query_ptr>, QVariant ) ),
                    SLOT( onTracksAdded( QList<Tomahawk::query_ptr>, QVariant ) ) );

    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
}


void
TreeModel::addCollection( const collection_ptr& collection )
{
    qDebug() << Q_FUNC_INFO << collection->name()
                            << collection->source()->id()
                            << collection->source()->userName();

    m_collection = collection;
    DatabaseCommand_AllArtists* cmd = new DatabaseCommand_AllArtists( collection );

    connect( cmd, SIGNAL( artists( QList<Tomahawk::artist_ptr> ) ),
                    SLOT( onArtistsAdded( QList<Tomahawk::artist_ptr> ) ) );

    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );

    m_title = tr( "All Artists from %1" ).arg( collection->source()->friendlyName() );
}


void
TreeModel::addFilteredCollection( const collection_ptr& collection, unsigned int amount, DatabaseCommand_AllArtists::SortOrder order )
{
    qDebug() << Q_FUNC_INFO << collection->name()
                            << collection->source()->id()
                            << collection->source()->userName()
                            << amount << order;

    DatabaseCommand_AllArtists* cmd = new DatabaseCommand_AllArtists( collection );
    cmd->setLimit( amount );
    cmd->setSortOrder( order );
    cmd->setSortDescending( true );

    connect( cmd, SIGNAL( artists( QList<Tomahawk::artist_ptr>, Tomahawk::collection_ptr ) ),
                    SLOT( onArtistsAdded( QList<Tomahawk::artist_ptr>, Tomahawk::collection_ptr ) ) );

    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );

    m_title = tr( "All albums from %1" ).arg( collection->source()->friendlyName() );
}


void
TreeModel::onArtistsAdded( const QList<Tomahawk::artist_ptr>& artists )
{
    if ( !artists.count() )
        return;

    int c = rowCount( QModelIndex() );
    QPair< int, int > crows;
    crows.first = c;
    crows.second = c + artists.count() - 1;

    emit beginInsertRows( QModelIndex(), crows.first, crows.second );

    TreeModelItem* artistitem;
    foreach( const artist_ptr& artist, artists )
    {
        artistitem = new TreeModelItem( artist, m_rootItem );
        artistitem->cover = m_defaultCover;
        artistitem->index = createIndex( m_rootItem->children.count() - 1, 0, artistitem );
        connect( artistitem, SIGNAL( dataChanged() ), SLOT( onDataChanged() ) );
    }

    emit endInsertRows();
    qDebug() << rowCount( QModelIndex() );
}


void
TreeModel::onAlbumsAdded( const QList<Tomahawk::album_ptr>& albums, const QVariant& data )
{
    qDebug() << Q_FUNC_INFO << albums.count();
    if ( !albums.count() )
        return;

    QModelIndex parent = index( data.toUInt(), 0, QModelIndex() );
    TreeModelItem* parentItem = itemFromIndex( parent );

    // the -1 is because we fake a rowCount of 1 to trigger Qt calling fetchMore()
    int c = rowCount( parent ) - 1;
    QPair< int, int > crows;
    crows.first = c;
    crows.second = c + albums.count() - 1;

    if ( crows.second > 0 )
        emit beginInsertRows( parent, crows.first + 1, crows.second );

    TreeModelItem* albumitem = 0;
    foreach( const album_ptr& album, albums )
    {
        albumitem = new TreeModelItem( album, parentItem );
        albumitem->cover = m_defaultCover;
        albumitem->index = createIndex( parentItem->children.count() - 1, 0, albumitem );
        connect( albumitem, SIGNAL( dataChanged() ), SLOT( onDataChanged() ) );

        Tomahawk::InfoSystem::InfoCriteriaHash trackInfo;
        trackInfo["artist"] = album->artist()->name();
        trackInfo["album"] = album->name();
        trackInfo["pptr"] = QString::number( (qlonglong)albumitem );

        Tomahawk::InfoSystem::InfoSystem::instance()->getInfo(
            s_tmInfoIdentifier, Tomahawk::InfoSystem::InfoAlbumCoverArt,
            QVariant::fromValue< Tomahawk::InfoSystem::InfoCriteriaHash >( trackInfo ), Tomahawk::InfoSystem::InfoCustomData() );
    }

    if ( crows.second > 0 )
        emit endInsertRows();
    else
        emit dataChanged( albumitem->index, albumitem->index.sibling( albumitem->index.row(), columnCount( QModelIndex() ) - 1 ) );

    qDebug() << rowCount( parent );
}


void
TreeModel::onTracksAdded( const QList<Tomahawk::query_ptr>& tracks, const QVariant& data )
{
    qDebug() << Q_FUNC_INFO << tracks.count();
    if ( !tracks.count() )
        return;

    QList< QVariant > rows = data.toList();

    QModelIndex parent = index( rows.first().toUInt(), 0, index( rows.at( 1 ).toUInt(), 0, QModelIndex() ) );
    TreeModelItem* parentItem = itemFromIndex( parent );

    // the -1 is because we fake a rowCount of 1 to trigger Qt calling fetchMore()
    int c = rowCount( parent ) - 1;
    QPair< int, int > crows;
    crows.first = c;
    crows.second = c + tracks.count() - 1;

    if ( crows.second > 0 )
        emit beginInsertRows( parent, crows.first + 1, crows.second );

    TreeModelItem* item = 0;
    foreach( const query_ptr& query, tracks )
    {
        qDebug() << query->toString();
        item = new TreeModelItem( query->results().first(), parentItem );
        item->cover = m_defaultCover;
        item->index = createIndex( parentItem->children.count() - 1, 0, item );

        connect( item, SIGNAL( dataChanged() ), SLOT( onDataChanged() ) );
    }

    if ( crows.second > 0 )
        emit endInsertRows();
    else
        emit dataChanged( item->index, item->index.sibling( item->index.row(), columnCount( QModelIndex() ) - 1 ) );

    qDebug() << rowCount( parent );
}


void
TreeModel::infoSystemInfo( QString caller, Tomahawk::InfoSystem::InfoType type, QVariant input, QVariant output, Tomahawk::InfoSystem::InfoCustomData customData )
{
    Q_UNUSED( customData );
    qDebug() << Q_FUNC_INFO;

    if ( caller != s_tmInfoIdentifier ||
       ( type != Tomahawk::InfoSystem::InfoAlbumCoverArt && type != Tomahawk::InfoSystem::InfoArtistImages ) )
    {
        qDebug() << "Info of wrong type or not with our identifier";
        return;
    }

    if ( !output.canConvert< Tomahawk::InfoSystem::InfoCustomData >() )
    {
        qDebug() << "Cannot convert fetched art from a QByteArray";
        return;
    }

    Tomahawk::InfoSystem::InfoCriteriaHash pptr = input.value< Tomahawk::InfoSystem::InfoCriteriaHash >();
    Tomahawk::InfoSystem::InfoCustomData returnedData = output.value< Tomahawk::InfoSystem::InfoCustomData >();
    const QByteArray ba = returnedData["imgbytes"].toByteArray();
    if ( ba.length() )
    {
        QPixmap pm;
        pm.loadFromData( ba );

        bool ok;
        qlonglong p = pptr["pptr"].toLongLong( &ok );
        TreeModelItem* ai = reinterpret_cast<TreeModelItem*>(p);

        if ( pm.isNull() )
            ai->cover = m_defaultCover;
        else
            ai->cover = pm;

        emit dataChanged( ai->index, ai->index.sibling( ai->index.row(), columnCount( QModelIndex() ) - 1 ) );
    }
}


void
TreeModel::infoSystemFinished( QString target )
{
    Q_UNUSED( target );
    qDebug() << Q_FUNC_INFO;
}


void
TreeModel::onDataChanged()
{
    TreeModelItem* p = (TreeModelItem*)sender();
    emit dataChanged( p->index, p->index.sibling( p->index.row(), columnCount( QModelIndex() ) - 1 ) );
}
