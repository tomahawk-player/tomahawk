/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2011       Leo Franchi <lfranchi@kde.org>
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

#include "PlayableModel.h"

#include <QDateTime>
#include <QMimeData>
#include <QTreeView>

#include "Artist.h"
#include "Album.h"
#include "Pipeline.h"
#include "PlayableItem.h"
#include "PlayableProxyModel.h"
#include "Source.h"
#include "Typedefs.h"
#include "audio/AudioEngine.h"
#include "utils/TomahawkUtils.h"
#include "utils/Logger.h"

using namespace Tomahawk;


PlayableModel::PlayableModel( QObject* parent, bool loading )
    : QAbstractItemModel( parent )
    , m_rootItem( new PlayableItem( 0 ) )
    , m_readOnly( true )
    , m_loading( loading )
{
    connect( AudioEngine::instance(), SIGNAL( started( Tomahawk::result_ptr ) ), SLOT( onPlaybackStarted( Tomahawk::result_ptr ) ), Qt::DirectConnection );
    connect( AudioEngine::instance(), SIGNAL( stopped() ), SLOT( onPlaybackStopped() ), Qt::DirectConnection );

    m_header << tr( "Artist" ) << tr( "Title" ) << tr( "Composer" ) << tr( "Album" ) << tr( "Track" ) << tr( "Duration" )
             << tr( "Bitrate" ) << tr( "Age" ) << tr( "Year" ) << tr( "Size" ) << tr( "Origin" ) << tr( "Accuracy" ) << tr( "Name" );
}


PlayableModel::~PlayableModel()
{
}


QModelIndex
PlayableModel::createIndex( int row, int column, PlayableItem* item ) const
{
    if ( item->query() )
    {
        connect( item->query().data(), SIGNAL( playableStateChanged( bool ) ), SLOT( onQueryBecamePlayable( bool ) ), Qt::UniqueConnection );
        connect( item->query().data(), SIGNAL( resolvingFinished( bool ) ), SLOT( onQueryResolved( bool ) ), Qt::UniqueConnection );
    }

    return QAbstractItemModel::createIndex( row, column, item );
}


QModelIndex
PlayableModel::index( int row, int column, const QModelIndex& parent ) const
{
    if ( !m_rootItem || row < 0 || column < 0 )
        return QModelIndex();

    PlayableItem* parentItem = itemFromIndex( parent );
    PlayableItem* childItem = parentItem->children.value( row );
    if ( !childItem )
        return QModelIndex();

    return createIndex( row, column, childItem );
}


int
PlayableModel::rowCount( const QModelIndex& parent ) const
{
    if ( parent.column() > 0 )
        return 0;

    PlayableItem* parentItem = itemFromIndex( parent );
    if ( !parentItem )
        return 0;

    return parentItem->children.count();
}


int
PlayableModel::columnCount( const QModelIndex& parent ) const
{
    Q_UNUSED( parent );

    return 12;
}


bool
PlayableModel::hasChildren( const QModelIndex& parent ) const
{
    PlayableItem* parentItem = itemFromIndex( parent );
    if ( !parentItem )
        return false;

    if ( parentItem == m_rootItem )
        return true;

    return ( !parentItem->artist().isNull() || !parentItem->album().isNull() );
}


QModelIndex
PlayableModel::parent( const QModelIndex& child ) const
{
    PlayableItem* entry = itemFromIndex( child );
    if ( !entry )
        return QModelIndex();

    PlayableItem* parentEntry = entry->parent();
    if ( !parentEntry )
        return QModelIndex();

    PlayableItem* grandparentEntry = parentEntry->parent();
    if ( !grandparentEntry )
        return QModelIndex();

    int row = grandparentEntry->children.indexOf( parentEntry );
    return createIndex( row, 0, parentEntry );
}


QVariant
PlayableModel::artistData( const artist_ptr& artist, int role ) const
{
    if ( role != Qt::DisplayRole ) // && role != Qt::ToolTipRole )
        return QVariant();

    return artist->name();
}


QVariant
PlayableModel::albumData( const album_ptr& album, int role ) const
{
    if ( role != Qt::DisplayRole ) // && role != Qt::ToolTipRole )
        return QVariant();

    return album->name();
}


QVariant
PlayableModel::queryData( const query_ptr& query, int column, int role ) const
{
    if ( role != Qt::DisplayRole ) // && role != Qt::ToolTipRole )
        return QVariant();

    switch ( column )
    {
        case Artist:
            return query->artist();
            break;

        case Name:
        case Track:
            return query->track();
            break;

        case Album:
            return query->album();
            break;

        case Composer:
            return query->composer();
            break;

        case Duration:
            return TomahawkUtils::timeToString( query->duration() );
            break;

        case AlbumPos:
        {
            QString tPos;
            if ( query->albumpos() != 0 )
            {
                tPos = QString::number( query->albumpos() );
                if ( query->discnumber() == 0 )
                    return tPos;
                else
                    return QString( "%1.%2" ).arg( QString::number( query->discnumber() ) )
                                             .arg( tPos );
            }
        }
        break;

        default:
            break;
    }
    if ( query->numResults() )
    {
        switch ( column )
        {
            case Bitrate:
                if ( query->results().first()->bitrate() > 0 )
                    return query->results().first()->bitrate();
                break;

            case Age:
                return TomahawkUtils::ageToString( QDateTime::fromTime_t( query->results().first()->modificationTime() ) );
                break;

            case Year:
                if ( query->results().first()->year() != 0 )
                    return query->results().first()->year();
                break;

            case Filesize:
                return TomahawkUtils::filesizeToString( query->results().first()->size() );
                break;

            case Origin:
                return query->results().first()->friendlySource();
                break;

            case Score:
            {
                float score = query->results().first()->score();
                if ( score == 1.0 )
                    return tr( "Perfect match" );
                if ( score > 0.9 )
                    return tr( "Very good match" );
                if ( score > 0.7 )
                    return tr( "Good match" );
                if ( score > 0.5 )
                    return tr( "Vague match" );
                if ( score > 0.3 )
                    return tr( "Bad match" );
                if ( score > 0.0 )
                    return tr( "Very bad match" );
                
                return tr( "Not available" );
                break;
            }

            default:
                break;
        }
    }

    return QVariant();
}


QVariant
PlayableModel::data( const QModelIndex& index, int role ) const
{
    PlayableItem* entry = itemFromIndex( index );
    if ( !entry )
        return QVariant();

    if ( role == Qt::DecorationRole )
    {
        return QVariant();
    }
    else if ( role == Qt::TextAlignmentRole )
    {
        return QVariant( columnAlignment( index.column() ) );
    }
    else if ( role == PlayableProxyModel::TypeRole )
    {
        if ( entry->result() )
        {
            return Tomahawk::TypeResult;
        }
        else if ( entry->query() )
        {
            return Tomahawk::TypeQuery;
        }
        else if ( entry->artist() )
        {
            return Tomahawk::TypeArtist;
        }
        else if ( entry->album() )
        {
            return Tomahawk::TypeAlbum;
        }
    }

    if ( !entry->query().isNull() )
    {
        return queryData( entry->query()->displayQuery(), index.column(), role );
    }
    else if ( !entry->artist().isNull() )
    {
        return artistData( entry->artist(), role );
    }
    else if ( !entry->album().isNull() )
    {
        return albumData( entry->album(), role );
    }

    return QVariant();
}


QVariant
PlayableModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    Q_UNUSED( orientation );

    if ( role == Qt::DisplayRole && section >= 0 )
    {
        if ( section < m_header.count() )
            return m_header.at( section );
        else
            return tr( "Name" );
    }

    if ( role == Qt::TextAlignmentRole )
    {
        return QVariant( columnAlignment( section ) );
    }

    return QVariant();
}


void
PlayableModel::setCurrentIndex( const QModelIndex& index )
{
    PlayableItem* oldEntry = itemFromIndex( m_currentIndex );
    if ( oldEntry )
    {
        oldEntry->setIsPlaying( false );
    }

    PlayableItem* entry = itemFromIndex( index );
    if ( index.isValid() && entry && !entry->query().isNull() )
    {
        m_currentIndex = index;
        m_currentUuid = entry->query()->id();
        entry->setIsPlaying( true );
    }
    else
    {
        m_currentIndex = QModelIndex();
        m_currentUuid = QString();
    }

    emit currentIndexChanged();
}


Qt::DropActions
PlayableModel::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}


Qt::ItemFlags
PlayableModel::flags( const QModelIndex& index ) const
{
    Qt::ItemFlags defaultFlags = QAbstractItemModel::flags( index );

    if ( index.isValid() && index.column() == 0 )
        return Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | defaultFlags;
    else
        return Qt::ItemIsDropEnabled | defaultFlags;
}


QStringList
PlayableModel::mimeTypes() const
{
    QStringList types;
    types << "application/tomahawk.mixed";
    return types;
}


QMimeData*
PlayableModel::mimeData( const QModelIndexList &indexes ) const
{
    qDebug() << Q_FUNC_INFO;

    QByteArray resultData;
    QDataStream resultStream( &resultData, QIODevice::WriteOnly );

    // lets try with artist only
    bool fail = false;
    foreach ( const QModelIndex& i, indexes )
    {
        if ( i.column() > 0 || indexes.contains( i.parent() ) )
            continue;

        PlayableItem* item = itemFromIndex( i );
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

        PlayableItem* item = itemFromIndex( i );
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

        PlayableItem* item = itemFromIndex( i );
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
    QDataStream mixedStream( &resultData, QIODevice::WriteOnly );
    foreach ( const QModelIndex& i, indexes )
    {
        if ( i.column() > 0 || indexes.contains( i.parent() ) )
            continue;

        PlayableItem* item = itemFromIndex( i );
        if ( !item )
            continue;

        if ( !item->artist().isNull() )
        {
            const artist_ptr& artist = item->artist();
            mixedStream << QString( "application/tomahawk.metadata.artist" ) << artist->name();
        }
        else if ( !item->album().isNull() )
        {
            const album_ptr& album = item->album();
            mixedStream << QString( "application/tomahawk.metadata.album" ) << album->artist()->name() << album->name();
        }
        else if ( !item->result().isNull() )
        {
            const result_ptr& result = item->result();
            mixedStream << QString( "application/tomahawk.result.list" ) << qlonglong( &result );
        }
        else if ( !item->query().isNull() )
        {
            const query_ptr& query = item->query();
            mixedStream << QString( "application/tomahawk.query.list" ) << qlonglong( &query );
        }
    }

    QMimeData* mimeData = new QMimeData();
    mimeData->setData( "application/tomahawk.mixed", resultData );
    return mimeData;
}


void
PlayableModel::clear()
{
    if ( rowCount( QModelIndex() ) )
    {
        finishLoading();

        emit beginResetModel();
        delete m_rootItem;
        m_rootItem = 0;
        m_rootItem = new PlayableItem( 0 );
        emit endResetModel();
    }
}


QList< query_ptr >
PlayableModel::queries() const
{
    Q_ASSERT( m_rootItem );

    QList< query_ptr > tracks;
    foreach ( PlayableItem* item, m_rootItem->children )
    {
        tracks << item->query();
    }

    return tracks;
}


template <typename T>
void
PlayableModel::insertInternal( const QList< T >& items, int row )
{
    if ( !items.count() )
    {
        emit itemCountChanged( rowCount( QModelIndex() ) );

        finishLoading();
        return;
    }

    int c = row;
    QPair< int, int > crows;
    crows.first = c;
    crows.second = c + items.count() - 1;

    emit beginInsertRows( QModelIndex(), crows.first, crows.second );

    int i = 0;
    PlayableItem* plitem;
    foreach ( const T& item, items )
    {
        plitem = new PlayableItem( item, m_rootItem, row + i );
        plitem->index = createIndex( row + i, 0, plitem );
        i++;

/*        if ( item->id() == currentItemUuid() )
            setCurrentItem( plitem->index );*/

        connect( plitem, SIGNAL( dataChanged() ), SLOT( onDataChanged() ) );
    }

    emit endInsertRows();
    emit itemCountChanged( rowCount( QModelIndex() ) );
    finishLoading();
}


void
PlayableModel::remove( int row, bool moreToCome )
{
    removeIndex( index( row, 0, QModelIndex() ), moreToCome );
}


void
PlayableModel::removeIndex( const QModelIndex& index, bool moreToCome )
{
    if ( QThread::currentThread() != thread() )
    {
        QMetaObject::invokeMethod( this, "remove",
                                   Qt::QueuedConnection,
                                   Q_ARG(const QModelIndex, index),
                                   Q_ARG(bool, moreToCome) );
        return;
    }

    if ( index.column() > 0 )
        return;

    PlayableItem* item = itemFromIndex( index );
    if ( item )
    {
        if ( index == m_currentIndex )
            setCurrentIndex( QModelIndex() );
            
        emit beginRemoveRows( index.parent(), index.row(), index.row() );
        delete item;
        emit endRemoveRows();
    }

    if ( !moreToCome )
        emit itemCountChanged( rowCount( QModelIndex() ) );
}


void
PlayableModel::removeIndexes( const QList<QModelIndex>& indexes )
{
    QList<QPersistentModelIndex> pil;
    foreach ( const QModelIndex& idx, indexes )
    {
        pil << idx;
    }

    removeIndexes( pil );
}


void
PlayableModel::removeIndexes( const QList<QPersistentModelIndex>& indexes )
{
    QList<QPersistentModelIndex> finalIndexes;
    foreach ( const QPersistentModelIndex index, indexes )
    {
        if ( index.column() > 0 )
            continue;
        finalIndexes << index;
    }

    for ( int i = 0; i < finalIndexes.count(); i++ )
    {
        removeIndex( finalIndexes.at( i ), i + 1 != finalIndexes.count() );
    }
}


void
PlayableModel::onPlaybackStarted( const Tomahawk::result_ptr& result )
{
    PlayableItem* oldEntry = itemFromIndex( m_currentIndex );
    if ( oldEntry && ( oldEntry->query().isNull() || !oldEntry->query()->numResults() || oldEntry->query()->results().first().data() != result.data() ) )
    {
        oldEntry->setIsPlaying( false );
    }
}


void
PlayableModel::onPlaybackStopped()
{
    PlayableItem* oldEntry = itemFromIndex( m_currentIndex );
    if ( oldEntry )
    {
        oldEntry->setIsPlaying( false );
    }
}


void
PlayableModel::ensureResolved()
{
    for( int i = 0; i < rowCount( QModelIndex() ); i++ )
    {
        query_ptr query = itemFromIndex( index( i, 0, QModelIndex() ) )->query();

        if ( !query->resolvingFinished() )
            Pipeline::instance()->resolve( query );
    }
}


Qt::Alignment
PlayableModel::columnAlignment( int column ) const
{
    switch ( column )
    {
        case Age:
        case AlbumPos:
        case Bitrate:
        case Duration:
        case Filesize:
        case Score:
        case Year:
            return Qt::AlignHCenter;
            break;

        default:
            return Qt::AlignLeft;
    }
}


void
PlayableModel::onDataChanged()
{
    PlayableItem* p = (PlayableItem*)sender();
    if ( p && p->index.isValid() )
        emit dataChanged( p->index, p->index.sibling( p->index.row(), columnCount() - 1 ) );
}


void
PlayableModel::startLoading()
{
    m_loading = true;
    emit loadingStarted();
}


void
PlayableModel::finishLoading()
{
    m_loading = false;
    emit loadingFinished();
}


PlayableItem*
PlayableModel::itemFromIndex( const QModelIndex& index ) const
{
    if ( index.isValid() )
    {
        return static_cast<PlayableItem*>( index.internalPointer() );
    }
    else
    {
        return m_rootItem;
    }
}


void
PlayableModel::appendArtist( const Tomahawk::artist_ptr& artist )
{
    QList< artist_ptr > artists;
    artists << artist;

    appendArtists( artists );
}


void
PlayableModel::appendAlbum( const Tomahawk::album_ptr& album )
{
    QList< album_ptr > albums;
    albums << album;

    appendAlbums( albums );
}


void
PlayableModel::appendQuery( const Tomahawk::query_ptr& query )
{
    QList< query_ptr > queries;
    queries << query;

    appendQueries( queries );
}


void
PlayableModel::appendArtists( const QList< Tomahawk::artist_ptr >& artists )
{
    insertArtists( artists, rowCount( QModelIndex() ) );
}


void
PlayableModel::appendAlbums( const QList< Tomahawk::album_ptr >& albums )
{
    insertAlbums( albums, rowCount( QModelIndex() ) );
}


void
PlayableModel::appendQueries( const QList< Tomahawk::query_ptr >& queries )
{
    insertQueries( queries, rowCount( QModelIndex() ) );
}


void
PlayableModel::insertArtist( const Tomahawk::artist_ptr& artist, int row )
{
    QList< artist_ptr > artists;
    artists << artist;

    insertArtists( artists, row );
}


void
PlayableModel::insertAlbum( const Tomahawk::album_ptr& album, int row )
{
    QList< album_ptr > albums;
    albums << album;

    insertAlbums( albums, row );
}


void
PlayableModel::insertQuery( const Tomahawk::query_ptr& query, int row )
{
    QList< query_ptr > queries;
    queries << query;

    insertQueries( queries, row );
}


void
PlayableModel::insertArtists( const QList< Tomahawk::artist_ptr >& artists, int row )
{
    insertInternal( artists, row );
}


void
PlayableModel::insertAlbums( const QList< Tomahawk::album_ptr >& albums, int row )
{
    insertInternal( albums, row );
}


void
PlayableModel::insertQueries( const QList< Tomahawk::query_ptr >& queries, int row )
{
    insertInternal( queries, row );
}


void
PlayableModel::setTitle( const QString& title )
{
    m_title = title;
    emit changed();
}


void
PlayableModel::setDescription( const QString& description )
{
    m_description = description;
    emit changed();
}


void
PlayableModel::setIcon( const QPixmap& pixmap )
{
    m_icon = pixmap;
    emit changed();
}


void
PlayableModel::onQueryBecamePlayable( bool playable )
{
    Q_UNUSED( playable );

    Tomahawk::Query* q = qobject_cast< Query* >( sender() );
    if ( !q )
    {
        // Track has been removed from the playlist by now
        return;
    }

    Tomahawk::query_ptr query = q->weakRef().toStrongRef();
    PlayableItem* item = itemFromQuery( query );

    if ( item )
    {
        emit indexPlayable( item->index );
    }
}


void
PlayableModel::onQueryResolved( bool hasResults )
{
    Q_UNUSED( hasResults );
    
    Tomahawk::Query* q = qobject_cast< Query* >( sender() );
    if ( !q )
    {
        // Track has been removed from the playlist by now
        return;
    }
    
    Tomahawk::query_ptr query = q->weakRef().toStrongRef();
    PlayableItem* item = itemFromQuery( query );
    
    if ( item )
    {
        emit indexResolved( item->index );
    }
}


PlayableItem*
PlayableModel::itemFromQuery( const Tomahawk::query_ptr& query ) const
{
    if ( !query )
        return 0;

    for ( int i = 0; i < rowCount( QModelIndex() ); i++ )
    {
        QModelIndex idx = index( i, 0, QModelIndex() );
        PlayableItem* item = itemFromIndex( idx );
        if ( item && item->query() == query )
        {
            return item;
        }
    }

    tDebug() << "Could not find item for query:" << query->toString();
    return 0;
}


PlayableItem*
PlayableModel::itemFromResult( const Tomahawk::result_ptr& result ) const
{
    if ( !result )
        return 0;

    for ( int i = 0; i < rowCount( QModelIndex() ); i++ )
    {
        QModelIndex idx = index( i, 0, QModelIndex() );
        PlayableItem* item = itemFromIndex( idx );
        if ( item && item->result() == result )
        {
            return item;
        }
    }

    tDebug() << "Could not find item for result:" << result->toString();
    return 0;
}
