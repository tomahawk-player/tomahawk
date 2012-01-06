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

#include <QMimeData>
#include <QTreeView>

#include "artist.h"
#include "album.h"
#include "pipeline.h"
#include "source.h"
#include "sourcelist.h"
#include "database/database.h"
#include "database/databasecommand_playbackhistory.h"
#include "dynamic/GeneratorInterface.h"
#include "dropjob.h"
#include "utils/tomahawkutils.h"
#include "utils/logger.h"

using namespace Tomahawk;


PlaylistModel::PlaylistModel( QObject* parent )
    : TrackModel( parent )
    , m_isTemporary( false )
    , m_changesOngoing( false )
{
    m_dropStorage.parent = QPersistentModelIndex();
    m_dropStorage.row = -10;

    setReadOnly( true );
}


PlaylistModel::~PlaylistModel()
{
}


void
PlaylistModel::loadPlaylist( const Tomahawk::playlist_ptr& playlist, bool loadEntries )
{
    if ( !m_playlist.isNull() )
    {
        disconnect( m_playlist.data(), SIGNAL( revisionLoaded( Tomahawk::PlaylistRevision ) ), this, SLOT( onRevisionLoaded( Tomahawk::PlaylistRevision ) ) );
        disconnect( m_playlist.data(), SIGNAL( deleted( Tomahawk::playlist_ptr ) ), this, SIGNAL( playlistDeleted() ) );
        disconnect( m_playlist.data(), SIGNAL( changed() ), this, SIGNAL( playlistChanged() ) );
    }

    if ( loadEntries )
        clear();

    m_playlist = playlist;
    connect( playlist.data(), SIGNAL( revisionLoaded( Tomahawk::PlaylistRevision ) ), SLOT( onRevisionLoaded( Tomahawk::PlaylistRevision ) ) );
    connect( playlist.data(), SIGNAL( deleted( Tomahawk::playlist_ptr ) ), this, SIGNAL( playlistDeleted() ) );
    connect( playlist.data(), SIGNAL( changed() ), this, SIGNAL( playlistChanged() ) );

    setReadOnly( !m_playlist->author()->isLocal() );
    setTitle( playlist->title() );
    setDescription( tr( "A playlist by %1, created %2" )
                  .arg( playlist->author()->isLocal() ? tr( "you" ) : playlist->author()->friendlyName() )
                  .arg( TomahawkUtils::ageToString( QDateTime::fromTime_t( playlist->createdOn() ), true ) ) );

    m_isTemporary = false;
    if ( !loadEntries )
        return;

    QList<plentry_ptr> entries = playlist->entries();
    append( entries );
}


void
PlaylistModel::loadHistory( const Tomahawk::source_ptr& source, unsigned int amount )
{
    if ( rowCount( QModelIndex() ) )
    {
        clear();
    }

    m_playlist.clear();

    DatabaseCommand_PlaybackHistory* cmd = new DatabaseCommand_PlaybackHistory( source );
    cmd->setLimit( amount );

    connect( cmd, SIGNAL( tracks( QList<Tomahawk::query_ptr> ) ),
                    SLOT( append( QList<Tomahawk::query_ptr> ) ), Qt::QueuedConnection );

    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
}


void
PlaylistModel::clear()
{
    TrackModel::clear();

    m_waitingForResolved.clear();
}


void
PlaylistModel::append( const QList< plentry_ptr >& entries )
{
    insert( entries, rowCount( QModelIndex() ) );
}


void
PlaylistModel::append( const QList< query_ptr >& queries )
{
    insert( queries, rowCount( QModelIndex() ) );
}


void
PlaylistModel::append( const Tomahawk::query_ptr& query )
{
    insert( query, rowCount( QModelIndex() ) );
}


void
PlaylistModel::append( const Tomahawk::album_ptr& album )
{
    if ( album.isNull() )
        return;

    connect( album.data(), SIGNAL( tracksAdded( QList<Tomahawk::query_ptr> ) ),
                             SLOT( append( QList<Tomahawk::query_ptr> ) ) );

    if ( rowCount( QModelIndex() ) == 0 )
    {
        setTitle( album->name() );
        setDescription( tr( "All tracks by %1 on album %2" ).arg( album->artist()->name() ).arg( album->name() ) );
        m_isTemporary = true;
    }

    append( album->getPlaylistInterface()->tracks() );
}


void
PlaylistModel::append( const Tomahawk::artist_ptr& artist )
{
    if ( artist.isNull() )
        return;

    connect( artist.data(), SIGNAL( tracksAdded( QList<Tomahawk::query_ptr> ) ),
                              SLOT( append( QList<Tomahawk::query_ptr> ) ) );

    if ( rowCount( QModelIndex() ) == 0 )
    {
        setTitle( artist->name() );
        setDescription( tr( "All tracks by %1" ).arg( artist->name() ) );
        m_isTemporary = true;
    }

    append( artist->getPlaylistInterface()->tracks() );
}


void
PlaylistModel::insert( const Tomahawk::query_ptr& query, int row )
{
    TrackModel::insert( query, row );
}


void
PlaylistModel::insert( const QList< Tomahawk::query_ptr >& queries, int row )
{
    QList< Tomahawk::plentry_ptr > entries;
    foreach( const query_ptr& query, queries )
    {
        plentry_ptr entry = plentry_ptr( new PlaylistEntry() );

        if ( query->results().count() )
            entry->setDuration( query->results().at( 0 )->duration() );
        else
            entry->setDuration( 0 );

        entry->setLastmodified( 0 );
        entry->setAnnotation( "" ); // FIXME
        entry->setQuery( query );
        entry->setGuid( uuid() );

        entries << entry;
    }

    insert( entries, row );
}


void
PlaylistModel::insert( const QList< Tomahawk::plentry_ptr >& entries, int row )
{
    if ( !entries.count() )
    {
        emit trackCountChanged( rowCount( QModelIndex() ) );
        return;
    }

    int c = row;
    QPair< int, int > crows;
    crows.first = c;
    crows.second = c + entries.count() - 1;

    emit beginInsertRows( QModelIndex(), crows.first, crows.second );

    QList< Tomahawk::query_ptr > queries;
    int i = 0;
    TrackModelItem* plitem;
    foreach( const plentry_ptr& entry, entries )
    {
        plitem = new TrackModelItem( entry, rootItem(), row + i );
        plitem->index = createIndex( row + i, 0, plitem );
        i++;

        if ( entry->query()->id() == currentItemUuid() )
            setCurrentItem( plitem->index );

        if ( !entry->query()->resolvingFinished() && !entry->query()->playable() )
        {
            queries << entry->query();
            m_waitingForResolved.append( entry->query().data() );
            connect( entry->query().data(), SIGNAL( resolvingFinished( bool ) ), SLOT( trackResolved( bool ) ) );
        }

        connect( plitem, SIGNAL( dataChanged() ), SLOT( onDataChanged() ) );
    }

    if ( !m_waitingForResolved.isEmpty() )
    {
        Pipeline::instance()->resolve( queries );
        emit loadingStarted();
    }

    emit endInsertRows();
    emit trackCountChanged( rowCount( QModelIndex() ) );
}


void
PlaylistModel::trackResolved( bool )
{
    Tomahawk::Query* q = qobject_cast< Query* >( sender() );
    if ( !q )
    {
        // Track has been removed from the playlist by now
        return;
    }

    m_waitingForResolved.removeAll( q );
    disconnect( q, SIGNAL( resolvingFinished( bool ) ), this, SLOT( trackResolved( bool ) ) );

    if ( m_waitingForResolved.isEmpty() )
    {
        emit loadingFinished();
    }
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
    if ( !m_waitForRevision.contains( revision.revisionguid ) )
        loadPlaylist( m_playlist );
    else
        m_waitForRevision.removeAll( revision.revisionguid );
}


QMimeData*
PlaylistModel::mimeData( const QModelIndexList& indexes ) const
{
    // Add the playlist id to the mime data so that we can detect dropping on ourselves
    QMimeData* d = TrackModel::mimeData( indexes );
    if ( !m_playlist.isNull() )
        d->setData( "application/tomahawk.playlist.id", m_playlist->guid().toLatin1() );

    return d;
}


bool
PlaylistModel::dropMimeData( const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent )
{
    Q_UNUSED( column );

    if ( action == Qt::IgnoreAction || isReadOnly() )
        return true;

    if ( !DropJob::acceptsMimeData( data ) )
        return false;

    m_dropStorage.row = row;
    m_dropStorage.parent = QPersistentModelIndex( parent );
    m_dropStorage.action = action;

    DropJob* dj = new DropJob();

    if ( !DropJob::acceptsMimeData( data, DropJob::Track | DropJob::Playlist | DropJob::Album | DropJob::Artist ) )
        return false;

    dj->setDropTypes( DropJob::Track | DropJob::Playlist | DropJob::Artist | DropJob::Album );
    dj->setDropAction( DropJob::Append );
/*    if ( action & Qt::MoveAction )
        dj->setDropAction( DropJob::Move ); */

#ifdef Q_WS_MAC
    // On mac, drags from outside the app are still Qt::MoveActions instead of Qt::CopyAction by default
    // so check if the drag originated in this playlist to determine whether or not to copy
    if ( !data->hasFormat( "application/tomahawk.playlist.id" ) ||
       ( !m_playlist.isNull() && data->data( "application/tomahawk.playlist.id" ) != m_playlist->guid() ) )
    {
        dj->setDropAction( DropJob::Append );
    }
#endif

    connect( dj, SIGNAL( tracks( QList< Tomahawk::query_ptr > ) ), SLOT( parsedDroppedTracks( QList< Tomahawk::query_ptr > ) ) );
    dj->tracksFromMimeData( data );

    return true;
}


void
PlaylistModel::parsedDroppedTracks( QList< query_ptr > tracks )
{
    if ( m_dropStorage.row == -10  ) // nope
        return;

    int beginRow;
    if ( m_dropStorage.row != -1 )
        beginRow = m_dropStorage.row;
    else if ( m_dropStorage.parent.isValid() )
        beginRow = m_dropStorage.parent.row();
    else
        beginRow = rowCount( QModelIndex() );

    if ( tracks.count() )
    {
        bool update = ( m_dropStorage.action & Qt::CopyAction || m_dropStorage.action & Qt::MoveAction );
        if ( update )
            beginPlaylistChanges();

        insert( tracks, beginRow );

        if ( update && m_dropStorage.action & Qt::CopyAction )
            endPlaylistChanges();
    }

    m_dropStorage.parent = QPersistentModelIndex();
    m_dropStorage.row = -10;
}


void
PlaylistModel::beginPlaylistChanges()
{
    if ( m_playlist.isNull() || !m_playlist->author()->isLocal() )
        return;

    Q_ASSERT( !m_changesOngoing );
    m_changesOngoing = true;
}


void
PlaylistModel::endPlaylistChanges()
{
    if ( m_playlist.isNull() || !m_playlist->author()->isLocal() )
        return;

    if ( m_changesOngoing )
    {
        m_changesOngoing = false;
    }
    else
    {
        tDebug() << "Called" << Q_FUNC_INFO << "unexpectedly!";
        Q_ASSERT( false );
    }

    QList<plentry_ptr> l = playlistEntries();
    QString newrev = uuid();
    m_waitForRevision << newrev;

    if ( dynplaylist_ptr dynplaylist = m_playlist.dynamicCast<Tomahawk::DynamicPlaylist>() )
    {
        if ( dynplaylist->mode() == OnDemand )
        {
            dynplaylist->createNewRevision( newrev );
        }
        else if ( dynplaylist->mode() == Static )
        {
            dynplaylist->createNewRevision( newrev, dynplaylist->currentrevision(), dynplaylist->type(), dynplaylist->generator()->controls(), l );
        }
    }
    else
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
PlaylistModel::remove( int row, bool moreToCome )
{
    TrackModel::remove( row, moreToCome );
}


void
PlaylistModel::remove( const QModelIndex& index, bool moreToCome )
{
    TrackModelItem* item = itemFromIndex( index );

    if ( item && m_waitingForResolved.contains( item->query().data() ) )
    {
        m_waitingForResolved.removeAll( item->query().data() );
        if ( m_waitingForResolved.isEmpty() )
            emit loadingFinished();
    }

    if ( !m_changesOngoing )
        beginPlaylistChanges();

    TrackModel::remove( index, moreToCome );

    if ( !moreToCome )
        endPlaylistChanges();
}


void
PlaylistModel::remove( const QList<QModelIndex>& indexes )
{
    TrackModel::remove( indexes );
}


void
PlaylistModel::remove( const QList<QPersistentModelIndex>& indexes )
{
    TrackModel::remove( indexes );
}


bool
PlaylistModel::isTemporary() const
{
    return m_isTemporary;
}
