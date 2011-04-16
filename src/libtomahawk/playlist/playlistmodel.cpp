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

#include "playlistmodel.h"

#include <QDebug>
#include <QMimeData>
#include <QTreeView>

#include "album.h"

#include "database/database.h"
#include "database/databasecommand_playbackhistory.h"
#include "dynamic/GeneratorInterface.h"

using namespace Tomahawk;


PlaylistModel::PlaylistModel( QObject* parent )
    : TrackModel( parent )
    , m_waitForUpdate( false )
{
    qDebug() << Q_FUNC_INFO;

    setReadOnly( false );
}


PlaylistModel::~PlaylistModel()
{
}


int
PlaylistModel::columnCount( const QModelIndex& parent ) const
{
    return TrackModel::columnCount( parent );
}


QVariant
PlaylistModel::data( const QModelIndex& index, int role ) const
{
    return TrackModel::data( index, role );
}


QVariant
PlaylistModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    return TrackModel::headerData( section, orientation, role );
}


void
PlaylistModel::loadPlaylist( const Tomahawk::playlist_ptr& playlist, bool loadEntries )
{
    if ( !m_playlist.isNull() )
    {
        disconnect( m_playlist.data(), SIGNAL( revisionLoaded( Tomahawk::PlaylistRevision ) ), this, SLOT( onRevisionLoaded( Tomahawk::PlaylistRevision ) ) );
        disconnect( m_playlist.data(), SIGNAL( deleted( Tomahawk::playlist_ptr ) ), this, SIGNAL( playlistDeleted() ) );
    }

    if ( rowCount( QModelIndex() ) && loadEntries )
    {
        clear();
    }

    m_playlist = playlist;
    connect( playlist.data(), SIGNAL( revisionLoaded( Tomahawk::PlaylistRevision ) ), SLOT( onRevisionLoaded( Tomahawk::PlaylistRevision ) ) );
    connect( playlist.data(), SIGNAL( deleted( Tomahawk::playlist_ptr ) ), this, SIGNAL( playlistDeleted() ) );

    setReadOnly( !m_playlist->author()->isLocal() );
    setTitle( playlist->title() );
    setDescription( tr( "A playlist by %1" ).arg( playlist->author()->isLocal() ? tr( "you" ) : playlist->author()->friendlyName() ) );

    if ( !loadEntries )
        return;

    TrackModelItem* plitem;
    QList<plentry_ptr> entries = playlist->entries();
    if ( entries.count() )
    {
        int c = rowCount( QModelIndex() );

        qDebug() << "Starting loading" << playlist->title();
        emit beginInsertRows( QModelIndex(), c, c + entries.count() - 1 );

        m_waitingForResolved.clear();
        foreach( const plentry_ptr& entry, entries )
        {
            qDebug() << entry->query()->toString();
            plitem = new TrackModelItem( entry, m_rootItem );
            plitem->index = createIndex( m_rootItem->children.count() - 1, 0, plitem );

            connect( plitem, SIGNAL( dataChanged() ), SLOT( onDataChanged() ) );

            if( !entry->query()->resolvingFinished() && !entry->query()->playable() ) {
                m_waitingForResolved.append( entry->query().data() );
                connect( entry->query().data(), SIGNAL( resolvingFinished( bool ) ), this, SLOT( trackResolved( bool ) ) );
            }
        }

        emit endInsertRows();
    }
    else
        qDebug() << "Playlist seems empty:" << playlist->title();

    if( !m_waitingForResolved.isEmpty() )
        emit loadingStarted();

    emit trackCountChanged( rowCount( QModelIndex() ) );
}


void
PlaylistModel::loadHistory( const Tomahawk::source_ptr& source, unsigned int amount )
{
    if ( rowCount( QModelIndex() ) )
    {
        clear();
    }

    m_playlist.clear();
    setReadOnly( true );

    DatabaseCommand_PlaybackHistory* cmd = new DatabaseCommand_PlaybackHistory( source );
    cmd->setLimit( amount );

    connect( cmd, SIGNAL( tracks( QList<Tomahawk::query_ptr> ) ),
                    SLOT( onTracksAdded( QList<Tomahawk::query_ptr> ) ), Qt::QueuedConnection );

    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
}


void
PlaylistModel::clear()
{
    if ( rowCount( QModelIndex() ) )
    {
        emit beginResetModel();
        delete m_rootItem;
        m_rootItem = 0;
        emit endResetModel();
        m_rootItem = new TrackModelItem( 0, this );
    }
}


void
PlaylistModel::append( const Tomahawk::query_ptr& query )
{
    if ( query.isNull() )
        return;

    QList< Tomahawk::query_ptr > ql;
    ql << query;

    onTracksAdded( ql );
}


void
PlaylistModel::append( const Tomahawk::album_ptr& album )
{
    if ( album.isNull() )
        return;

    connect( album.data(), SIGNAL( tracksAdded( QList<Tomahawk::query_ptr> ) ),
                             SLOT( onTracksAdded( QList<Tomahawk::query_ptr> ) ) );

    onTracksAdded( album->tracks() );
}


void
PlaylistModel::append( const Tomahawk::artist_ptr& artist )
{
    if ( artist.isNull() )
        return;

    connect( artist.data(), SIGNAL( tracksAdded( QList<Tomahawk::query_ptr> ) ),
                              SLOT( onTracksAdded( QList<Tomahawk::query_ptr> ) ) );

    onTracksAdded( artist->tracks() );
}


void
PlaylistModel::insert( unsigned int row, const Tomahawk::query_ptr& query )
{
    if ( query.isNull() )
        return;

    QList< Tomahawk::query_ptr > ql;
    ql << query;

    onTracksInserted( row, ql );
}

void
PlaylistModel::trackResolved( bool )
{
    Tomahawk::Query* q = qobject_cast< Query* >( sender() );
    Q_ASSERT( q );

    m_waitingForResolved.removeAll( q );
    disconnect( q, SIGNAL( resolvingFinished( bool ) ), this, SLOT( trackResolved( bool ) ) );

    if( m_waitingForResolved.isEmpty() )
        emit loadingFinished();
}


void
PlaylistModel::onTracksAdded( const QList<Tomahawk::query_ptr>& tracks )
{
    onTracksInserted( rowCount( QModelIndex() ), tracks );
}


void
PlaylistModel::onTracksInserted( unsigned int row, const QList<Tomahawk::query_ptr>& tracks )
{
    if ( !tracks.count() )
    {
        emit trackCountChanged( rowCount( QModelIndex() ) );
        return;
    }

    int c = row;
    QPair< int, int > crows;
    crows.first = c;
    crows.second = c + tracks.count() - 1;

    emit beginInsertRows( QModelIndex(), crows.first, crows.second );

    int i = 0;
    TrackModelItem* plitem;
    foreach( const query_ptr& query, tracks )
    {
        plentry_ptr entry = plentry_ptr( new PlaylistEntry() );
        entry->setQuery( query );

        plitem = new TrackModelItem( entry, m_rootItem, row + i );
        plitem->index = createIndex( row + i, 0, plitem );

        i++;

        connect( plitem, SIGNAL( dataChanged() ), SLOT( onDataChanged() ) );
    }

    emit endInsertRows();
    emit trackCountChanged( rowCount( QModelIndex() ) );
}


void
PlaylistModel::onDataChanged()
{
    TrackModelItem* p = (TrackModelItem*)sender();
    if ( p && p->index.isValid() )
        emit dataChanged( p->index, p->index.sibling( p->index.row(), columnCount() - 1 ) );
}


void
PlaylistModel::onRevisionLoaded( Tomahawk::PlaylistRevision revision )
{
    qDebug() << Q_FUNC_INFO;

    if ( m_waitForUpdate )
    {
        qDebug() << m_playlist->currentrevision() << revision.revisionguid;
        m_waitForUpdate = false;
    }
    else
        loadPlaylist( m_playlist );
}


bool
PlaylistModel::dropMimeData( const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent )
{
    Q_UNUSED( column );
    if ( action == Qt::IgnoreAction || isReadOnly() )
        return true;

    if ( !data->hasFormat( "application/tomahawk.query.list" )
        && !data->hasFormat( "application/tomahawk.plentry.list" )
        && !data->hasFormat( "application/tomahawk.result.list" ) )
        return false;

    int beginRow;
    if ( row != -1 )
        beginRow = row;
    else if ( parent.isValid() )
        beginRow = parent.row();
    else
        beginRow = rowCount( QModelIndex() );

    qDebug() << data->formats();

    if ( data->hasFormat( "application/tomahawk.query.list" ) )
    {
        QByteArray itemData = data->data( "application/tomahawk.query.list" );
        QDataStream stream( &itemData, QIODevice::ReadOnly );
        QList<Tomahawk::query_ptr> queries;

        while ( !stream.atEnd() )
        {
            qlonglong qptr;
            stream >> qptr;

            Tomahawk::query_ptr* query = reinterpret_cast<Tomahawk::query_ptr*>(qptr);
            if ( query && !query->isNull() )
            {
                qDebug() << "Dropped query item:" << query->data()->artist() << "-" << query->data()->track() << action;
                queries << *query;
            }
        }

        emit beginInsertRows( QModelIndex(), beginRow, beginRow + queries.count() - 1 );
        foreach( const Tomahawk::query_ptr& query, queries )
        {
            plentry_ptr e( new PlaylistEntry() );
            e->setGuid( uuid() );

            if ( query->results().count() )
                e->setDuration( query->results().at( 0 )->duration() );
            else
                e->setDuration( 0 );

            e->setLastmodified( 0 );
            e->setAnnotation( "" ); // FIXME
            e->setQuery( query );

            TrackModelItem* plitem = new TrackModelItem( e, m_rootItem, beginRow );
            plitem->index = createIndex( beginRow++, 0, plitem );

            connect( plitem, SIGNAL( dataChanged() ), SLOT( onDataChanged() ) );
        }
        emit endInsertRows();

        if ( action == Qt::CopyAction )
        {
            onPlaylistChanged();
        }
    }

    return true;
}


void
PlaylistModel::onPlaylistChanged( bool waitForUpdate )
{
    qDebug() << Q_FUNC_INFO;

    if ( m_playlist.isNull() )
        return;

    QList<plentry_ptr> l = playlistEntries();

    foreach( const plentry_ptr& ple, l )
    {
        qDebug() << "updateinternal:" << ple->query()->toString();
    }

    m_waitForUpdate = waitForUpdate;
    QString newrev = uuid();
    if( dynplaylist_ptr dynplaylist = m_playlist.dynamicCast<Tomahawk::DynamicPlaylist>() )
    {
        if( dynplaylist->mode() == OnDemand )
        {
            dynplaylist->createNewRevision( newrev );
        }
        else if( dynplaylist->mode() == Static )
        {
            dynplaylist->createNewRevision( newrev, dynplaylist->currentrevision(), dynplaylist->type(), dynplaylist->generator()->controls(), l );
        }
    } else
    {
        m_playlist->createNewRevision( newrev, m_playlist->currentrevision(), l );
    }
}


QList<Tomahawk::plentry_ptr>
PlaylistModel::playlistEntries() const
{
    QList<plentry_ptr> l;
    for ( int i = 0; i < rowCount( QModelIndex() ); i++ )
    {
        QModelIndex idx = index( i, 0, QModelIndex() );
        if ( !idx.isValid() )
            continue;

        TrackModelItem* item = itemFromIndex( idx );
        if ( item )
            l << item->entry();
    }

    return l;
}


void
PlaylistModel::remove( unsigned int row, bool moreToCome )
{
    removeIndex( index( row, 0, QModelIndex() ), moreToCome );
}


void
PlaylistModel::removeIndex( const QModelIndex& index, bool moreToCome )
{
    TrackModel::removeIndex( index );

    if ( !moreToCome && !m_playlist.isNull() )
    {
        onPlaylistChanged();
    }
}
