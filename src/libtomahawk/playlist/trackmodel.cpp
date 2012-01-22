/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2011       Leo Franchi <lfranchi@kde.org>
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

#include "trackmodel.h"

#include <QDateTime>
#include <QMimeData>
#include <QTreeView>

#include "audio/audioengine.h"
#include "utils/tomahawkutils.h"

#include "artist.h"
#include "album.h"
#include "pipeline.h"
#include "utils/logger.h"

using namespace Tomahawk;


TrackModel::TrackModel( QObject* parent )
    : QAbstractItemModel( parent )
    , m_rootItem( new TrackModelItem( 0, this ) )
    , m_readOnly( true )
    , m_style( Detailed )
{
    connect( AudioEngine::instance(), SIGNAL( started( Tomahawk::result_ptr ) ), SLOT( onPlaybackStarted( Tomahawk::result_ptr ) ), Qt::DirectConnection );
    connect( AudioEngine::instance(), SIGNAL( stopped() ), SLOT( onPlaybackStopped() ), Qt::DirectConnection );
}


TrackModel::~TrackModel()
{
}


QModelIndex
TrackModel::index( int row, int column, const QModelIndex& parent ) const
{
    if ( !m_rootItem || row < 0 || column < 0 )
        return QModelIndex();

    TrackModelItem* parentItem = itemFromIndex( parent );
    TrackModelItem* childItem = parentItem->children.value( row );
    if ( !childItem )
        return QModelIndex();

    return createIndex( row, column, childItem );
}


int
TrackModel::rowCount( const QModelIndex& parent ) const
{
    if ( parent.column() > 0 )
        return 0;

    TrackModelItem* parentItem = itemFromIndex( parent );
    if ( !parentItem )
        return 0;

    return parentItem->children.count();
}


int
TrackModel::columnCount( const QModelIndex& parent ) const
{
    Q_UNUSED( parent );

    switch ( m_style )
    {
        case Short:
        case ShortWithAvatars:
            return 1;
            break;

        case Detailed:
        default:
            return 12;
            break;
    }
}


QModelIndex
TrackModel::parent( const QModelIndex& child ) const
{
    TrackModelItem* entry = itemFromIndex( child );
    if ( !entry )
        return QModelIndex();

    TrackModelItem* parentEntry = entry->parent;
    if ( !parentEntry )
        return QModelIndex();

    TrackModelItem* grandparentEntry = parentEntry->parent;
    if ( !grandparentEntry )
        return QModelIndex();

    int row = grandparentEntry->children.indexOf( parentEntry );
    return createIndex( row, 0, parentEntry );
}


QVariant
TrackModel::data( const QModelIndex& index, int role ) const
{
    TrackModelItem* entry = itemFromIndex( index );
    if ( !entry )
        return QVariant();

    if ( role == Qt::DecorationRole )
    {
        return QVariant();
    }

    if ( role == Qt::SizeHintRole )
    {
        return QSize( 0, 18 );
    }

    if ( role == StyleRole )
    {
        return m_style;
    }

    if ( role != Qt::DisplayRole ) // && role != Qt::ToolTipRole )
        return QVariant();

    const query_ptr& query = entry->query();
    if ( !query->numResults() )
    {
        switch( index.column() )
        {
            case Artist:
                return query->artist();
                break;

            case Track:
                return query->track();
                break;

            case Album:
                return query->album();
                break;
        }
    }
    else
    {
        switch( index.column() )
        {
            case Artist:
                return query->results().first()->artist()->name();
                break;

            case Track:
                return query->results().first()->track();
                break;

            case Album:
                return query->results().first()->album()->name();
                break;

            case Composer:
                if ( !query->results().first()->composer().isNull() )
                    return query->results().first()->composer()->name();
                break;

            case Duration:
                return TomahawkUtils::timeToString( query->results().first()->duration() );
                break;

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
                return query->results().first()->score();
                break;

            case AlbumPos:
                QString tPos;
                if ( query->results().first()->albumpos() != 0 )
                {
                    tPos = QString::number( query->results().first()->albumpos() );
                    if( query->results().first()->discnumber() == 0 )
                        return tPos;
                    else
                        return QString( "%1.%2" ).arg( QString::number( query->results().first()->discnumber() ) )
                                                 .arg( tPos );
                }
                break;
        }
    }

    return QVariant();
}


QVariant
TrackModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    Q_UNUSED( orientation );

    QStringList headers;
    headers << tr( "Artist" ) << tr( "Title" ) << tr( "Composer" ) << tr( "Album" ) << tr( "Track" ) << tr( "Duration" ) << tr( "Bitrate" ) << tr( "Age" ) << tr( "Year" ) << tr( "Size" ) << tr( "Origin" ) << tr( "Score" );
    if ( role == Qt::DisplayRole && section >= 0 )
    {
        return headers.at( section );
    }

    return QVariant();
}


void
TrackModel::setCurrentItem( const QModelIndex& index )
{
    TrackModelItem* oldEntry = itemFromIndex( m_currentIndex );
    if ( oldEntry )
    {
        oldEntry->setIsPlaying( false );
    }

    TrackModelItem* entry = itemFromIndex( index );
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

    if ( index.isValid() && index.column() == 0 )
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
        TrackModelItem* item = itemFromIndex( idx );
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


void
TrackModel::clear()
{
    if ( rowCount( QModelIndex() ) )
    {
        emit loadingFinished();

        emit beginResetModel();
        delete m_rootItem;
        m_rootItem = 0;
        m_rootItem = new TrackModelItem( 0, this );
        emit endResetModel();
    }
}


QList< query_ptr >
TrackModel::queries() const
{
    Q_ASSERT( m_rootItem );

    QList< query_ptr > tracks;
    foreach ( TrackModelItem* item, m_rootItem->children )
    {
        tracks << item->query();
    }

    return tracks;
}


void
TrackModel::append( const Tomahawk::query_ptr& query )
{
    insert( query, rowCount( QModelIndex() ) );
}


void
TrackModel::append( const QList< Tomahawk::query_ptr >& queries )
{
    insert( queries, rowCount( QModelIndex() ) );
}


void
TrackModel::insert( const Tomahawk::query_ptr& query, int row )
{
    if ( query.isNull() )
        return;

    QList< Tomahawk::query_ptr > ql;
    ql << query;

    insert( ql, row );
}


void
TrackModel::insert( const QList< Tomahawk::query_ptr >& queries, int row )
{
    if ( !queries.count() )
    {
        emit trackCountChanged( rowCount( QModelIndex() ) );
        return;
    }

    int c = row;
    QPair< int, int > crows;
    crows.first = c;
    crows.second = c + queries.count() - 1;

    emit beginInsertRows( QModelIndex(), crows.first, crows.second );

    int i = 0;
    TrackModelItem* plitem;
    foreach( const query_ptr& query, queries )
    {
        plitem = new TrackModelItem( query, m_rootItem, row + i );
        plitem->index = createIndex( row + i, 0, plitem );
        i++;

        if ( query->id() == currentItemUuid() )
            setCurrentItem( plitem->index );

        connect( plitem, SIGNAL( dataChanged() ), SLOT( onDataChanged() ) );
    }

    emit endInsertRows();
    emit trackCountChanged( rowCount( QModelIndex() ) );
}


void
TrackModel::remove( int row, bool moreToCome )
{
    remove( index( row, 0, QModelIndex() ), moreToCome );
}


void
TrackModel::remove( const QModelIndex& index, bool moreToCome )
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

    TrackModelItem* item = itemFromIndex( index );
    if ( item )
    {
        emit beginRemoveRows( index.parent(), index.row(), index.row() );
        delete item;
        emit endRemoveRows();
    }

    if ( !moreToCome )
        emit trackCountChanged( rowCount( QModelIndex() ) );
}


void
TrackModel::remove( const QList<QModelIndex>& indexes )
{
    QList<QPersistentModelIndex> pil;
    foreach ( const QModelIndex& idx, indexes )
    {
        pil << idx;
    }

    remove( pil );
}


void
TrackModel::remove( const QList<QPersistentModelIndex>& indexes )
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
        remove( finalIndexes.at( i ), i + 1 != finalIndexes.count() );
    }
}


TrackModelItem*
TrackModel::itemFromIndex( const QModelIndex& index ) const
{
    if ( index.isValid() )
    {
        return static_cast<TrackModelItem*>( index.internalPointer() );
    }
    else
    {
        return m_rootItem;
    }
}


void
TrackModel::onPlaybackStarted( const Tomahawk::result_ptr& result )
{
    TrackModelItem* oldEntry = itemFromIndex( m_currentIndex );
    if ( oldEntry && ( oldEntry->query().isNull() || !oldEntry->query()->numResults() || oldEntry->query()->results().first().data() != result.data() ) )
    {
        oldEntry->setIsPlaying( false );
    }
}


void
TrackModel::onPlaybackStopped()
{
    TrackModelItem* oldEntry = itemFromIndex( m_currentIndex );
    if ( oldEntry )
    {
        oldEntry->setIsPlaying( false );
    }
}


void
TrackModel::ensureResolved()
{
    for( int i = 0; i < rowCount( QModelIndex() ); i++ )
    {
        query_ptr query = itemFromIndex( index( i, 0, QModelIndex() ) )->query();

        if ( !query->resolvingFinished() )
            Pipeline::instance()->resolve( query );
    }
}


void
TrackModel::setStyle( TrackModel::TrackItemStyle style )
{
    m_style = style;
}
