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

#include "audio/AudioEngine.h"
#include "utils/TomahawkUtils.h"
#include "Source.h"

#include "Artist.h"
#include "Album.h"
#include "Pipeline.h"
#include "PlayableItem.h"
#include "utils/Logger.h"

using namespace Tomahawk;


PlayableModel::PlayableModel( QObject* parent )
    : QAbstractItemModel( parent )
    , m_rootItem( new PlayableItem( 0, this ) )
    , m_readOnly( true )
    , m_style( Detailed )
{
    connect( AudioEngine::instance(), SIGNAL( started( Tomahawk::result_ptr ) ), SLOT( onPlaybackStarted( Tomahawk::result_ptr ) ), Qt::DirectConnection );
    connect( AudioEngine::instance(), SIGNAL( stopped() ), SLOT( onPlaybackStopped() ), Qt::DirectConnection );
}


PlayableModel::~PlayableModel()
{
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

    switch ( m_style )
    {
        case Short:
        case ShortWithAvatars:
        case Large:
            return 1;
            break;

        case Detailed:
        default:
            return 12;
            break;
    }
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
PlayableModel::data( const QModelIndex& index, int role ) const
{
    PlayableItem* entry = itemFromIndex( index );
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

    if ( role == Qt::TextAlignmentRole )
    {
        return QVariant( columnAlignment( index.column() ) );
    }

    if ( role == StyleRole )
    {
        return m_style;
    }

    if ( role != Qt::DisplayRole ) // && role != Qt::ToolTipRole )
        return QVariant();

    const query_ptr& query = entry->query()->displayQuery();
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
            
        case Composer:
            return query->composer();
            break;
            
        case Duration:
            return TomahawkUtils::timeToString( query->duration() );
            break;

        case AlbumPos:
            QString tPos;
            if ( query->albumpos() != 0 )
            {
                tPos = QString::number( query->albumpos() );
                if( query->discnumber() == 0 )
                    return tPos;
                else
                    return QString( "%1.%2" ).arg( QString::number( query->discnumber() ) )
                                             .arg( tPos );
            }
            break;
    }
    if ( query->numResults() )
    {
        switch( index.column() )
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
                return query->results().first()->score();
                break;
        }
    }

    return QVariant();
}


QVariant
PlayableModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    Q_UNUSED( orientation );

    QStringList headers;
    headers << tr( "Artist" ) << tr( "Title" ) << tr( "Composer" ) << tr( "Album" ) << tr( "Track" ) << tr( "Duration" ) << tr( "Bitrate" ) << tr( "Age" ) << tr( "Year" ) << tr( "Size" ) << tr( "Origin" ) << tr( "Score" );
    if ( role == Qt::DisplayRole && section >= 0 )
    {
        return headers.at( section );
    }

    if ( role == Qt::TextAlignmentRole )
    {
        return QVariant( columnAlignment( section ) );
    }

    return QVariant();
}


void
PlayableModel::updateDetailedInfo( const QModelIndex& index )
{
    if ( style() != PlayableModel::Short && style() != PlayableModel::Large )
        return;

    PlayableItem* item = itemFromIndex( index );
    if ( item->query().isNull() )
        return;

    if ( style() == PlayableModel::Short || style() == PlayableModel::Large )
    {
        item->query()->cover( QSize( 0, 0 ) );
    }

    if ( style() == PlayableModel::Large )
    {
        item->query()->loadSocialActions();
    }
}


void
PlayableModel::setCurrentItem( const QModelIndex& index )
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
    types << "application/tomahawk.query.list";
    return types;
}


QMimeData*
PlayableModel::mimeData( const QModelIndexList &indexes ) const
{
    qDebug() << Q_FUNC_INFO;

    QByteArray queryData;
    QDataStream queryStream( &queryData, QIODevice::WriteOnly );

    foreach ( const QModelIndex& i, indexes )
    {
        if ( i.column() > 0 )
            continue;

        QModelIndex idx = index( i.row(), 0, i.parent() );
        PlayableItem* item = itemFromIndex( idx );
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
PlayableModel::clear()
{
    if ( rowCount( QModelIndex() ) )
    {
        emit loadingFinished();

        emit beginResetModel();
        delete m_rootItem;
        m_rootItem = 0;
        m_rootItem = new PlayableItem( 0, this );
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


void
PlayableModel::append( const Tomahawk::query_ptr& query )
{
    insert( query, rowCount( QModelIndex() ) );
}


void
PlayableModel::append( const QList< Tomahawk::query_ptr >& queries )
{
    insert( queries, rowCount( QModelIndex() ) );
}


void
PlayableModel::insert( const Tomahawk::query_ptr& query, int row )
{
    if ( query.isNull() )
        return;

    QList< Tomahawk::query_ptr > ql;
    ql << query;

    insert( ql, row );
}


void
PlayableModel::insert( const QList< Tomahawk::query_ptr >& queries, int row )
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
    PlayableItem* plitem;
    foreach( const query_ptr& query, queries )
    {
        plitem = new PlayableItem( query, m_rootItem, row + i );
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
PlayableModel::remove( int row, bool moreToCome )
{
    remove( index( row, 0, QModelIndex() ), moreToCome );
}


void
PlayableModel::remove( const QModelIndex& index, bool moreToCome )
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
        emit beginRemoveRows( index.parent(), index.row(), index.row() );
        delete item;
        emit endRemoveRows();
    }

    if ( !moreToCome )
        emit trackCountChanged( rowCount( QModelIndex() ) );
}


void
PlayableModel::remove( const QList<QModelIndex>& indexes )
{
    QList<QPersistentModelIndex> pil;
    foreach ( const QModelIndex& idx, indexes )
    {
        pil << idx;
    }

    remove( pil );
}


void
PlayableModel::remove( const QList<QPersistentModelIndex>& indexes )
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


void
PlayableModel::setStyle( PlayableModel::PlayableItemStyle style )
{
    m_style = style;
}


Qt::Alignment
PlayableModel::columnAlignment( int column ) const
{
    switch( column )
    {
        case Age:
        case AlbumPos:
        case Bitrate:
        case Duration:
        case Filesize:
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
