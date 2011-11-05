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

#include <QListView>
#include <QMimeData>
#include <QNetworkReply>

#include "source.h"
#include "sourcelist.h"
#include "audio/audioengine.h"
#include "database/databasecommand_allalbums.h"
#include "database/databasecommand_alltracks.h"
#include "database/database.h"
#include "utils/tomahawkutils.h"
#include "utils/logger.h"

using namespace Tomahawk;


TreeModel::TreeModel( QObject* parent )
    : QAbstractItemModel( parent )
    , m_rootItem( new TreeModelItem( 0, this ) )
    , m_infoId( uuid() )
    , m_columnStyle( AllColumns )
    , m_mode( DatabaseMode )
{
    setIcon( QPixmap( RESPATH "images/music-icon.png" ) );

    connect( AudioEngine::instance(), SIGNAL( finished( Tomahawk::result_ptr ) ), SLOT( onPlaybackFinished( Tomahawk::result_ptr ) ), Qt::DirectConnection );
    connect( AudioEngine::instance(), SIGNAL( stopped() ), SLOT( onPlaybackStopped() ), Qt::DirectConnection );

    connect( Tomahawk::InfoSystem::InfoSystem::instance(),
             SIGNAL( info( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ),
               SLOT( infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ) );

    connect( Tomahawk::InfoSystem::InfoSystem::instance(), SIGNAL( finished( QString ) ), SLOT( infoSystemFinished( QString ) ) );
}


TreeModel::~TreeModel()
{
}


void
TreeModel::clear()
{
    if ( rowCount( QModelIndex() ) )
    {
        emit loadingFinished();

        emit beginResetModel();
        delete m_rootItem;
        m_rootItem = 0;
        m_rootItem = new TreeModelItem( 0, this );
        emit endResetModel();
    }
}


Tomahawk::collection_ptr
TreeModel::collection() const
{
    return m_collection;
}


void
TreeModel::getCover( const QModelIndex& index )
{
    TreeModelItem* item = itemFromIndex( index );
    if ( !item->cover.isNull() )
        return;

    Tomahawk::InfoSystem::InfoStringHash trackInfo;
    Tomahawk::InfoSystem::InfoRequestData requestData;

    if ( !item->artist().isNull() )
    {
        trackInfo["artist"] = item->artist()->name();
        requestData.type = Tomahawk::InfoSystem::InfoArtistImages;
    }
    else if ( !item->album().isNull() )
    {
        trackInfo["artist"] = item->album()->artist()->name();
        trackInfo["album"] = item->album()->name();
        requestData.type = Tomahawk::InfoSystem::InfoAlbumCoverArt;
    }
    else
        return;

    trackInfo["pptr"] = QString::number( (qlonglong)item );
    m_coverHash.insert( (qlonglong)item, index );

    requestData.caller = m_infoId;
    requestData.input = QVariant::fromValue< Tomahawk::InfoSystem::InfoStringHash >( trackInfo );
    requestData.customData = QVariantMap();
    Tomahawk::InfoSystem::InfoSystem::instance()->getInfo( requestData );
}


void
TreeModel::setCurrentItem( const QModelIndex& index )
{
    qDebug() << Q_FUNC_INFO;

    TreeModelItem* oldEntry = itemFromIndex( m_currentIndex );
    if ( oldEntry )
    {
        oldEntry->setIsPlaying( false );
    }

    TreeModelItem* entry = itemFromIndex( index );
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


bool
TreeModel::hasChildren( const QModelIndex& parent ) const
{
    TreeModelItem* parentItem = itemFromIndex( parent );
    if ( !parentItem )
        return false;

    if ( parentItem == m_rootItem )
        return true;

    return ( !parentItem->artist().isNull() || !parentItem->album().isNull() );
}


int
TreeModel::rowCount( const QModelIndex& parent ) const
{
    if ( parent.column() > 0 )
        return 0;

    TreeModelItem* parentItem = itemFromIndex( parent );
    if ( !parentItem )
        return 0;

    return parentItem->children.count();
}


int
TreeModel::columnCount( const QModelIndex& parent ) const
{
    Q_UNUSED( parent );
    if ( m_columnStyle == AllColumns )
        return 7;
    else if ( m_columnStyle == TrackOnly )
        return 1;

    // UH..
    return 0;
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
        if ( !entry->result().isNull() || !entry->query().isNull() )
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
    else if ( !entry->result().isNull() )
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
    else if ( !entry->query().isNull() && index.column() == Name )
    {
        return entry->query()->track();
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
    {
        TreeModelItem* item = itemFromIndex( index );
        if ( item && !item->result().isNull() )
            return Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | defaultFlags;
        if ( item && ( !item->album().isNull() || !item->artist().isNull() ) )
            return Qt::ItemIsDragEnabled | defaultFlags;
    }

    return defaultFlags;
}


QStringList
TreeModel::mimeTypes() const
{
    QStringList types;
    types << "application/tomahawk.mixed";
    return types;
}


QMimeData*
TreeModel::mimeData( const QModelIndexList &indexes ) const
{
    qDebug() << Q_FUNC_INFO;

    QByteArray resultData;
    QDataStream resultStream( &resultData, QIODevice::WriteOnly );

    // lets try with artist only
    bool fail = false;
    foreach ( const QModelIndex& i, indexes)
    {
        if ( i.column() > 0 || indexes.contains( i.parent() ) )
            continue;

        TreeModelItem* item = itemFromIndex( i );
        if ( !item )
            continue;

        if ( !item->artist().isNull() )
        {
            const artist_ptr& artist = item->artist();
            resultStream << artist->name();
        }
        else
        {
            fail = true;
            break;
        }
    }
    if ( !fail )
    {
        QMimeData* mimeData = new QMimeData();
        mimeData->setData( "application/tomahawk.metadata.artist", resultData );
        return mimeData;
    }

    // lets try with album only
    fail = false;
    resultData.clear();
    foreach ( const QModelIndex& i, indexes )
    {
        if ( i.column() > 0 || indexes.contains( i.parent() ) )
            continue;

        TreeModelItem* item = itemFromIndex( i );
        if ( !item )
            continue;

        if ( !item->album().isNull() )
        {
            const album_ptr& album = item->album();
            resultStream << album->artist()->name();
            resultStream << album->name();
        }
        else
        {
            fail = true;
            break;
        }
    }
    if ( !fail )
    {
        QMimeData* mimeData = new QMimeData();
        mimeData->setData( "application/tomahawk.metadata.album", resultData );
        return mimeData;
    }

    // lets try with tracks only
    fail = false;
    resultData.clear();
    foreach ( const QModelIndex& i, indexes )
    {
        if ( i.column() > 0 || indexes.contains( i.parent() ) )
            continue;

        TreeModelItem* item = itemFromIndex( i );
        if ( !item )
            continue;

        if ( !item->result().isNull() )
        {
            const result_ptr& result = item->result();
            resultStream << qlonglong( &result );
        }
        else
        {
            fail = true;
            break;
        }
    }
    if ( !fail )
    {
        QMimeData* mimeData = new QMimeData();
        mimeData->setData( "application/tomahawk.result.list", resultData );
        return mimeData;
    }

    // Ok... we have to use mixed
    resultData.clear();
    foreach ( const QModelIndex& i, indexes )
    {
        if ( i.column() > 0 || indexes.contains( i.parent() ) )
            continue;

        TreeModelItem* item = itemFromIndex( i );
        if ( !item )
            continue;

        if ( !item->artist().isNull() )
        {
            const artist_ptr& artist = item->artist();
            resultStream << QString( "application/tomahawk.metadata.artist" ) << artist->name();
        }
        else if ( !item->album().isNull() )
        {
            const album_ptr& album = item->album();
            resultStream << QString( "application/tomahawk.metadata.album" ) << album->artist()->name() << album->name();
        }
        else if ( !item->result().isNull() )
        {
            const result_ptr& result = item->result();
            resultStream << QString( "application/tomahawk.result.list" ) << qlonglong( &result );
        }
    }

    QMimeData* mimeData = new QMimeData();
    mimeData->setData( "application/tomahawk.mixed", resultData );
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

    emit loadingStarted();
    DatabaseCommand_AllArtists* cmd = new DatabaseCommand_AllArtists();

    connect( cmd, SIGNAL( artists( QList<Tomahawk::artist_ptr> ) ),
                    SLOT( onArtistsAdded( QList<Tomahawk::artist_ptr> ) ) );

    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );

    connect( SourceList::instance(), SIGNAL( sourceAdded( Tomahawk::source_ptr ) ), SLOT( onSourceAdded( Tomahawk::source_ptr ) ) );

    QList<Tomahawk::source_ptr> sources = SourceList::instance()->sources();
    foreach ( const source_ptr& source, sources )
    {
        connect( source->collection().data(), SIGNAL( changed() ), SLOT( onCollectionChanged() ) );
    }

    m_title = tr( "All Artists" );
}


void
TreeModel::addArtists( const artist_ptr& artist )
{
    if ( artist.isNull() )
        return;

    emit loadingStarted();

    QList<Tomahawk::artist_ptr> artists;
    artists << artist;
    onArtistsAdded( artists );
}


void
TreeModel::addAlbums( const artist_ptr& artist, const QModelIndex& parent )
{
    emit loadingStarted();

    if ( m_mode == DatabaseMode )
    {
        DatabaseCommand_AllAlbums* cmd = new DatabaseCommand_AllAlbums( m_collection, artist );
        cmd->setData( parent.row() );

        connect( cmd, SIGNAL( albums( QList<Tomahawk::album_ptr>, QVariant ) ),
                        SLOT( onAlbumsAdded( QList<Tomahawk::album_ptr>, QVariant ) ) );

        Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
    }
    else if ( m_mode == InfoSystemMode )
    {
        Tomahawk::InfoSystem::InfoStringHash artistInfo;
        artistInfo["artist"] = artist->name();

        Tomahawk::InfoSystem::InfoRequestData requestData;
        requestData.caller = m_infoId;
        requestData.customData["row"] = parent.row();
        requestData.input = QVariant::fromValue< Tomahawk::InfoSystem::InfoStringHash >( artistInfo );
        requestData.type = Tomahawk::InfoSystem::InfoArtistReleases;
        Tomahawk::InfoSystem::InfoSystem::instance()->getInfo( requestData );
    }
    else
        Q_ASSERT( false );
}


void
TreeModel::addTracks( const album_ptr& album, const QModelIndex& parent )
{
    emit loadingStarted();

    QList< QVariant > rows;
    rows << parent.row();
    rows << parent.parent().row();

    if ( m_mode == DatabaseMode )
    {
        DatabaseCommand_AllTracks* cmd = new DatabaseCommand_AllTracks( m_collection );
        cmd->setAlbum( album.data() );
        cmd->setData( QVariant( rows ) );

        connect( cmd, SIGNAL( tracks( QList<Tomahawk::query_ptr>, QVariant ) ),
                        SLOT( onTracksAdded( QList<Tomahawk::query_ptr>, QVariant ) ) );

        Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
    }
    else if ( m_mode == InfoSystemMode )
    {
        Tomahawk::InfoSystem::InfoStringHash artistInfo;
        artistInfo["artist"] = album->artist()->name();
        artistInfo["album"] = album->name();

        m_receivedInfoData.remove( artistInfo );
        Tomahawk::InfoSystem::InfoRequestData requestData;
        requestData.caller = m_infoId;
        requestData.customData["rows"] = QVariant( rows );
        requestData.input = QVariant::fromValue< Tomahawk::InfoSystem::InfoStringHash >( artistInfo );
        requestData.type = Tomahawk::InfoSystem::InfoAlbumSongs;
        requestData.timeoutMillis = 0;
        requestData.allSources = true;
        Tomahawk::InfoSystem::InfoSystem::instance()->getInfo( requestData );
    }
    else
        Q_ASSERT( false );
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

    connect( collection.data(), SIGNAL( changed() ), SLOT( onCollectionChanged() ) );

    if ( !collection->source()->avatar().isNull() )
        setIcon( collection->source()->avatar() );

    if ( collection->source()->isLocal() )
        setTitle( tr( "My Collection" ) );
    else
        setTitle( tr( "Collection of %1" ).arg( collection->source()->friendlyName() ) );
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

    if ( collection->source()->isLocal() )
        setTitle( tr( "My Collection" ) );
    else
        setTitle( tr( "Collection of %1" ).arg( collection->source()->friendlyName() ) );
}


void
TreeModel::onSourceAdded( const Tomahawk::source_ptr& source )
{
    connect( source->collection().data(), SIGNAL( changed() ), SLOT( onCollectionChanged() ) );
}


void
TreeModel::onCollectionChanged()
{
    clear();

    if ( m_collection )
        addCollection( m_collection );
    else
        addAllCollections();
}


void
TreeModel::onArtistsAdded( const QList<Tomahawk::artist_ptr>& artists )
{
    emit loadingFinished();
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
        artistitem->index = createIndex( m_rootItem->children.count() - 1, 0, artistitem );
        connect( artistitem, SIGNAL( dataChanged() ), SLOT( onDataChanged() ) );
    }

    emit endInsertRows();
}


void
TreeModel::onAlbumsAdded( const QList<Tomahawk::album_ptr>& albums, const QVariant& data )
{
    emit loadingFinished();
    if ( !albums.count() )
        return;

    QModelIndex parent = index( data.toInt(), 0, QModelIndex() );
    TreeModelItem* parentItem = itemFromIndex( parent );

    QPair< int, int > crows;
    int c = rowCount( parent );
    crows.first = c;
    crows.second = c + albums.count() - 1;

    emit beginInsertRows( parent, crows.first, crows.second );

    TreeModelItem* albumitem = 0;
    foreach( const album_ptr& album, albums )
    {
        albumitem = new TreeModelItem( album, parentItem );
        albumitem->index = createIndex( parentItem->children.count() - 1, 0, albumitem );
        connect( albumitem, SIGNAL( dataChanged() ), SLOT( onDataChanged() ) );

        getCover( albumitem->index );
    }

    emit endInsertRows();
}


void
TreeModel::onTracksAdded( const QList<Tomahawk::query_ptr>& tracks, const QVariant& data )
{
    emit loadingFinished();
    if ( !tracks.count() )
        return;

    QList< QVariant > rows = data.toList();
    tDebug() << "Adding to:" << rows;

    QModelIndex parent = index( rows.first().toUInt(), 0, index( rows.at( 1 ).toUInt(), 0, QModelIndex() ) );
    TreeModelItem* parentItem = itemFromIndex( parent );

    QPair< int, int > crows;
    int c = rowCount( parent );
    crows.first = c;
    crows.second = c + tracks.count() - 1;

    emit beginInsertRows( parent, crows.first, crows.second );

    TreeModelItem* item = 0;
    foreach( const query_ptr& query, tracks )
    {
        if ( query->numResults() )
            item = new TreeModelItem( query->results().first(), parentItem );
        else
            item = new TreeModelItem( query, parentItem );

        item->index = createIndex( parentItem->children.count() - 1, 0, item );

        connect( item, SIGNAL( dataChanged() ), SLOT( onDataChanged() ) );
    }

    emit endInsertRows();
}


void
TreeModel::infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData requestData, QVariant output )
{
    if ( requestData.caller != m_infoId )
    {
        return;
    }

    switch ( requestData.type )
    {
        case Tomahawk::InfoSystem::InfoAlbumCoverArt:
        case Tomahawk::InfoSystem::InfoArtistImages:
        {
            Tomahawk::InfoSystem::InfoStringHash pptr = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash >();
            QVariantMap returnedData = output.value< QVariantMap >();
            const QByteArray ba = returnedData["imgbytes"].toByteArray();
            if ( ba.length() )
            {
                QPixmap pm;
                pm.loadFromData( ba );
                bool ok;
                qlonglong p = pptr["pptr"].toLongLong( &ok );
                TreeModelItem* ai = itemFromIndex( m_coverHash.take( p ) );
                if ( !ai )
                    return;

                if ( !pm.isNull() )
                    ai->cover = pm;

                emit dataChanged( ai->index, ai->index.sibling( ai->index.row(), columnCount( QModelIndex() ) - 1 ) );
            }

            break;
        }

        case Tomahawk::InfoSystem::InfoArtistReleases:
        {
            QVariantMap returnedData = output.value< QVariantMap >();
            QStringList albums = returnedData[ "albums" ].toStringList();
            QList<album_ptr> al;

            Tomahawk::InfoSystem::InfoStringHash inputInfo;
            inputInfo = requestData.input.value< InfoSystem::InfoStringHash >();
            artist_ptr artist = Artist::get( inputInfo[ "artist" ], false );

            if ( artist.isNull() )
                return;

            foreach ( const QString& albumName, albums )
            {
                Tomahawk::album_ptr album = Tomahawk::Album::get( artist, albumName, false );
                al << album;
            }
            onAlbumsAdded( al, requestData.customData[ "row" ] );

            break;
        }

        case Tomahawk::InfoSystem::InfoAlbumSongs:
        {
            if ( m_receivedInfoData.contains( requestData.input.value< Tomahawk::InfoSystem::InfoStringHash >() ) )
                break;

            emit loadingFinished();

            QVariantMap returnedData = output.value< QVariantMap >();
            if ( returnedData.isEmpty() )
                break;

            m_receivedInfoData.insert( requestData.input.value< Tomahawk::InfoSystem::InfoStringHash >() );


            QStringList tracks = returnedData[ "tracks" ].toStringList();
            QList<query_ptr> ql;

            Tomahawk::InfoSystem::InfoStringHash inputInfo;
            inputInfo = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash >();

            foreach ( const QString& trackName, tracks )
            {
                query_ptr query = Query::get( inputInfo[ "artist" ], trackName, inputInfo[ "album" ], uuid() );
                ql << query;
            }
            onTracksAdded( ql, requestData.customData[ "rows" ] );

            break;
        }

        default:
        {
            Q_ASSERT( false );
            break;
        }
    }
}


void
TreeModel::infoSystemFinished( QString target )
{
    Q_UNUSED( target );
}


void
TreeModel::onPlaybackFinished( const Tomahawk::result_ptr& result )
{
    TreeModelItem* oldEntry = itemFromIndex( m_currentIndex );
    qDebug() << oldEntry->result().data() << result.data();
    if ( oldEntry && !oldEntry->result().isNull() && oldEntry->result().data() == result.data() )
    {
        oldEntry->setIsPlaying( false );
    }
}


void
TreeModel::onPlaybackStopped()
{
    TreeModelItem* oldEntry = itemFromIndex( m_currentIndex );
    if ( oldEntry )
    {
        oldEntry->setIsPlaying( false );
    }
}


void
TreeModel::onDataChanged()
{
    TreeModelItem* p = (TreeModelItem*)sender();
    emit dataChanged( p->index, p->index.sibling( p->index.row(), columnCount( QModelIndex() ) - 1 ) );
}


void
TreeModel::setColumnStyle( TreeModel::ColumnStyle style )
{
    m_columnStyle = style;
}


QModelIndex
TreeModel::indexFromArtist( const Tomahawk::artist_ptr& artist ) const
{
    for ( int i = 0; i < rowCount(); i++ )
    {
        QModelIndex idx = index( i, 0, QModelIndex() );
        TreeModelItem* item = itemFromIndex( idx );
        if ( item && item->artist() == artist )
        {
            return idx;
        }
    }

    return QModelIndex();
}
