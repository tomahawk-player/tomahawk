/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright 2012,      Leo Franchi <lfranchi@kde.org>
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

#include "TreeModel.h"

#include <QMimeData>

#include "Pipeline.h"
#include "Source.h"
#include "SourceList.h"
#include "audio/AudioEngine.h"
#include "database/DatabaseCommand_AllAlbums.h"
#include "database/DatabaseCommand_AllTracks.h"
#include "database/Database.h"
#include "utils/TomahawkUtils.h"
#include "utils/Logger.h"

using namespace Tomahawk;


TreeModel::TreeModel( QObject* parent )
    : QAbstractItemModel( parent )
    , m_rootItem( new TreeModelItem( 0, this ) )
    , m_infoId( uuid() )
    , m_columnStyle( AllColumns )
    , m_mode( DatabaseMode )
{
    setIcon( QPixmap( RESPATH "images/music-icon.png" ) );

    connect( AudioEngine::instance(), SIGNAL( started( Tomahawk::result_ptr ) ), SLOT( onPlaybackStarted( Tomahawk::result_ptr ) ), Qt::DirectConnection );
    connect( AudioEngine::instance(), SIGNAL( stopped() ), SLOT( onPlaybackStopped() ), Qt::DirectConnection );

    connect( Tomahawk::InfoSystem::InfoSystem::instance(),
             SIGNAL( info( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ),
               SLOT( infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ) );
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


void
TreeModel::setMode( ModelMode mode )
{
    clear();
    m_mode = mode;
    emit modeChanged( mode );
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

    if ( !item->artist().isNull() && !item->artist()->infoLoaded() )
        item->artist()->cover( QSize( 0, 0 ) );
    else if ( !item->album().isNull() && !item->album()->infoLoaded() )
        item->album()->cover( QSize( 0, 0 ) );
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
        tDebug() << Q_FUNC_INFO << "Loading Artist:" << parentItem->artist()->name();
        fetchAlbums( parentItem->artist() );
    }
    else if ( !parentItem->album().isNull() )
    {
        tDebug() << Q_FUNC_INFO << "Loading Album:" << parentItem->album()->name();
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
        return 8;
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
        unsigned int discnumber = 0;
        if ( !entry->query().isNull() )
            discnumber = entry->query()->discnumber();
        if ( discnumber == 0 )
            discnumber = result->discnumber();

        unsigned int albumpos = 0;
        if ( !entry->query().isNull() )
            albumpos = entry->query()->albumpos();
        if ( albumpos == 0 )
            albumpos = result->albumpos();

        switch( index.column() )
        {
            case Name:
                return QString( "%1%2%3" ).arg( discnumber > 0 ? QString( "%1." ).arg( discnumber ) : QString() )
                                          .arg( albumpos > 0 ? QString( "%1. ").arg( albumpos ) : QString() )
                                          .arg( result->track() );

            case Duration:
                return TomahawkUtils::timeToString( result->duration() );

            case Bitrate:
                if ( result->bitrate() > 0 )
                    return result->bitrate();
                break;

            case Age:
                return TomahawkUtils::ageToString( QDateTime::fromTime_t( result->modificationTime() ) );

            case Year:
                if ( result->year() != 0 )
                    return result->year();
                break;

            case Filesize:
                return TomahawkUtils::filesizeToString( result->size() );

            case Origin:
                return result->friendlySource();

            case AlbumPosition:
                return result->albumpos();

            case Composer:
                if ( !result->composer().isNull() )
                    return result->composer()->name();
                break;

            default:
                return QVariant();
        }
    }
    else if ( !entry->query().isNull() )
    {
        const query_ptr& query = entry->query();
        switch( index.column() )
        {
            case Name:
                return QString( "%1%2%3" ).arg( query->discnumber() > 0 ? QString( "%1." ).arg( query->discnumber() ) : QString() )
                                          .arg( query->albumpos() > 0 ? QString( "%1. ").arg( query->albumpos() ) : QString() )
                                          .arg( query->track() );

            case AlbumPosition:
                return entry->query()->albumpos();

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
    headers << tr( "Name" ) << tr( "Composer" ) << tr( "Duration" ) << tr( "Bitrate" ) << tr( "Age" ) << tr( "Year" ) << tr( "Size" ) << tr( "Origin" );
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
    emit loadingStarted();
    DatabaseCommand_AllArtists* cmd = new DatabaseCommand_AllArtists();

    connect( cmd, SIGNAL( artists( QList<Tomahawk::artist_ptr> ) ),
                    SLOT( onArtistsAdded( QList<Tomahawk::artist_ptr> ) ) );

    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );

    connect( SourceList::instance(), SIGNAL( sourceAdded( Tomahawk::source_ptr ) ), SLOT( onSourceAdded( Tomahawk::source_ptr ) ), Qt::UniqueConnection );

    QList<Tomahawk::source_ptr> sources = SourceList::instance()->sources();
    foreach ( const source_ptr& source, sources )
    {
        connect( source->collection().data(), SIGNAL( changed() ), SLOT( onCollectionChanged() ), Qt::UniqueConnection );
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
TreeModel::fetchAlbums( const artist_ptr& artist )
{
    emit loadingStarted();

    connect( artist.data(), SIGNAL( albumsAdded( QList<Tomahawk::album_ptr>, Tomahawk::ModelMode ) ),
                              SLOT( onAlbumsFound( QList<Tomahawk::album_ptr>, Tomahawk::ModelMode ) ), Qt::UniqueConnection );
        
    const QModelIndex parent = indexFromArtist( artist );
    addAlbums( parent, artist->albums( m_mode, m_collection ) );
}


void
TreeModel::onAlbumsFound( const QList<Tomahawk::album_ptr>& albums, ModelMode mode )
{
    if ( m_mode != mode )
        return;

    const artist_ptr artist = qobject_cast< Artist* >( sender() )->weakRef().toStrongRef();
    disconnect( artist.data(), SIGNAL( albumsAdded( QList<Tomahawk::album_ptr>, Tomahawk::ModelMode ) ),
                this,            SLOT( onAlbumsFound( QList<Tomahawk::album_ptr>, Tomahawk::ModelMode ) ) );

    const QModelIndex parent = indexFromArtist( artist );
    addAlbums( parent, albums );
}


void
TreeModel::addAlbums( const QModelIndex& parent, const QList<Tomahawk::album_ptr>& albums )
{
    emit loadingFinished();
    if ( !albums.count() )
        return;

    TreeModelItem* parentItem = itemFromIndex( parent );

    QPair< int, int > crows;
    const int c = rowCount( parent );
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
TreeModel::addTracks( const album_ptr& album, const QModelIndex& parent, bool autoRefetch )
{
    emit loadingStarted();

    QList< QVariant > rows;
    rows << parent.row();
    rows << parent.parent().row();

    if ( m_mode == DatabaseMode )
    {
        DatabaseCommand_AllTracks* cmd = new DatabaseCommand_AllTracks( m_collection );
        cmd->setAlbum( album );
        cmd->setData( QVariant( rows ) );

        connect( cmd, SIGNAL( tracks( QList<Tomahawk::query_ptr>, QVariant ) ),
                        SLOT( onTracksFound( QList<Tomahawk::query_ptr>, QVariant ) ) );

        Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
    }
    else if ( m_mode == InfoSystemMode )
    {
        Tomahawk::InfoSystem::InfoStringHash artistInfo;
        artistInfo["artist"] = album->artist()->name();
        artistInfo["album"] = album->name();

        m_receivedInfoData.removeAll( artistInfo );
        Tomahawk::InfoSystem::InfoRequestData requestData;
        requestData.caller = m_infoId;
        requestData.customData["rows"] = rows;
        requestData.customData["refetch"] = autoRefetch;
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

    connect( collection.data(), SIGNAL( changed() ), SLOT( onCollectionChanged() ), Qt::UniqueConnection );

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
    connect( source->collection().data(), SIGNAL( changed() ), SLOT( onCollectionChanged() ), Qt::UniqueConnection );
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
    {
        emit itemCountChanged( rowCount( QModelIndex() ) );
        return;
    }

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
    emit itemCountChanged( rowCount( QModelIndex() ) );
}


void
TreeModel::onTracksAdded( const QList<Tomahawk::query_ptr>& tracks, const QModelIndex& parent )
{
    emit loadingFinished();
    if ( !tracks.count() )
        return;

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
TreeModel::onTracksFound( const QList<Tomahawk::query_ptr>& tracks, const QVariant& variant )
{
    QList< QVariant > rows = variant.toList();
    QModelIndex idx = index( rows.first().toUInt(), 0, index( rows.at( 1 ).toUInt(), 0, QModelIndex() ) );

    onTracksAdded( tracks, idx );
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
        case Tomahawk::InfoSystem::InfoAlbumSongs:
        {
            m_receivedInfoData.append( requestData.input.value< Tomahawk::InfoSystem::InfoStringHash >() );

            QVariantMap returnedData = output.value< QVariantMap >();
            if ( !returnedData.isEmpty() )
            {
                emit loadingFinished();

                QList< QVariant > rows = requestData.customData[ "rows" ].toList();
                QModelIndex idx = index( rows.first().toUInt(), 0, index( rows.at( 1 ).toUInt(), 0, QModelIndex() ) );
                if ( rowCount( idx ) )
                    return;

                Tomahawk::InfoSystem::InfoStringHash inputInfo;
                inputInfo = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash >();

                QStringList tracks = returnedData[ "tracks" ].toStringList();
                QList<query_ptr> ql;

                //TODO: Figure out how to do this with a multi-disk album without breaking the
                //      current behaviour. I just know too little about InfoSystem to deal with
                //      it right now, I've only taken the liberty of adding Query::setDiscNumber
                //      which should make this easier. --Teo 11/2011
                unsigned int trackNo = 1;

                foreach ( const QString& trackName, tracks )
                {
                    query_ptr query = Query::get( inputInfo[ "artist" ], trackName, inputInfo[ "album" ] );
                    query->setAlbumPos( trackNo++ );
                    ql << query;
                }
                Pipeline::instance()->resolve( ql );

                onTracksAdded( ql, idx );
            }
            else if ( m_receivedInfoData.count() == 2 /* FIXME */ )
            {
                // If the second load got no data, but the first load did, don't do anything
                QList< QVariant > rows = requestData.customData[ "rows" ].toList();
                QModelIndex idx = index( rows.first().toUInt(), 0, index( rows.at( 1 ).toUInt(), 0, QModelIndex() ) );
                if ( rowCount( idx ) )
                    return;

                if ( requestData.customData[ "refetch" ].toBool() )
                {
                    setMode( DatabaseMode );

                    Tomahawk::InfoSystem::InfoStringHash inputInfo;
                    inputInfo = requestData.input.value< InfoSystem::InfoStringHash >();
                    artist_ptr artist = Artist::get( inputInfo[ "artist" ], false );
                    album_ptr album = Album::get( artist, inputInfo[ "album" ], false );

                    addTracks( album, QModelIndex() );
                }
                else
                    emit loadingFinished();
            }

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
TreeModel::onPlaybackStarted( const Tomahawk::result_ptr& result )
{
    TreeModelItem* oldEntry = itemFromIndex( m_currentIndex );
    if ( oldEntry && ( oldEntry->result().isNull() || oldEntry->result().data() != result.data() ) )
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
