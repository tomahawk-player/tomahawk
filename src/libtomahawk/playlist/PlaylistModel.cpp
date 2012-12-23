/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.o
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>rg>
 *   Copyright 2010-2012, Leo Franchi <lfranchi@kde.org>
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

#include "PlaylistModel.h"

#include <QMimeData>
#include <QTreeView>

#include "Artist.h"
#include "Album.h"
#include "Pipeline.h"
#include "Source.h"
#include "SourceList.h"
#include "database/Database.h"
#include "database/DatabaseCommand_PlaybackHistory.h"
#include "dynamic/GeneratorInterface.h"
#include "DropJob.h"
#include "PlayableItem.h"
#include "utils/TomahawkUtils.h"
#include "utils/Logger.h"
#include <utils/WebResultHintChecker.h>

using namespace Tomahawk;


PlaylistModel::PlaylistModel( QObject* parent )
    : PlayableModel( parent )
    , m_isTemporary( false )
    , m_changesOngoing( false )
    , m_isLoading( false )
    , m_acceptPlayableQueriesOnly( false )
    , m_savedInsertPos( -1 )
{
    m_dropStorage.parent = QPersistentModelIndex();
    m_dropStorage.row = -10;

    setReadOnly( true );
}


PlaylistModel::~PlaylistModel()
{
}


QString
PlaylistModel::guid() const
{
    if ( !m_playlist.isNull() )
    {
        return QString( "playlistmodel/%1" ).arg( m_playlist->guid() );
    }
    else
        return QString();
}


void
PlaylistModel::loadPlaylist( const Tomahawk::playlist_ptr& playlist, bool loadEntries )
{
    if ( !m_playlist.isNull() )
    {
        disconnect( m_playlist.data(), SIGNAL( revisionLoaded( Tomahawk::PlaylistRevision ) ), this, SLOT( onRevisionLoaded( Tomahawk::PlaylistRevision ) ) );
        disconnect( m_playlist.data(), SIGNAL( deleted( Tomahawk::playlist_ptr ) ), this, SIGNAL( playlistDeleted() ) );
        disconnect( m_playlist.data(), SIGNAL( changed() ), this, SLOT( onPlaylistChanged() ) );
    }

    m_isLoading = true;

    if ( loadEntries )
        clear();

    m_playlist = playlist;
    connect( playlist.data(), SIGNAL( revisionLoaded( Tomahawk::PlaylistRevision ) ), SLOT( onRevisionLoaded( Tomahawk::PlaylistRevision ) ) );
    connect( playlist.data(), SIGNAL( deleted( Tomahawk::playlist_ptr ) ), SIGNAL( playlistDeleted() ) );
    connect( playlist.data(), SIGNAL( changed() ), SLOT( onPlaylistChanged() ) );

    setReadOnly( !m_playlist->author()->isLocal() );
    m_isTemporary = false;

    onPlaylistChanged();

    if ( !loadEntries )
    {
        m_isLoading = false;
        return;
    }

    QList<plentry_ptr> entries = playlist->entries();
/*    foreach ( const plentry_ptr& p, entries )
        qDebug() << p->guid() << p->query()->track() << p->query()->artist();*/

    appendEntries( entries );

    m_isLoading = false;
}


void
PlaylistModel::onPlaylistChanged()
{
    QString age = TomahawkUtils::ageToString( QDateTime::fromTime_t( m_playlist->createdOn() ), true );
    QString desc;
    if ( m_playlist->creator().isEmpty() )
    {
        if ( m_playlist->author()->isLocal() )
        {
             desc = tr( "A playlist you created %1." )
                    .arg( age );
        }
        else
        {
            desc = tr( "A playlist by %1, created %2." )
                   .arg( m_playlist->author()->friendlyName() )
                   .arg( age );
        }
    }
    else
    {
        desc = tr( "A playlist by %1, created %2." )
               .arg( m_playlist->creator() )
               .arg( age );
    }

    setTitle( m_playlist->title() );
    setDescription( desc );

    emit playlistChanged();
}


void
PlaylistModel::clear()
{
    m_waitingForResolved.clear();
    PlayableModel::clear();
}


void
PlaylistModel::appendEntries( const QList< plentry_ptr >& entries )
{
    insertEntries( entries, rowCount( QModelIndex() ) );
}


void
PlaylistModel::insertAlbums( const QList< Tomahawk::album_ptr >& albums, int row )
{
    // FIXME: This currently appends, not inserts!
    Q_UNUSED( row );

    foreach ( const album_ptr& album, albums )
    {
        if ( album.isNull() )
            return;

        connect( album.data(), SIGNAL( tracksAdded( QList<Tomahawk::query_ptr>, Tomahawk::ModelMode, Tomahawk::collection_ptr ) ),
                                 SLOT( appendQueries( QList<Tomahawk::query_ptr> ) ) );

        appendQueries( album->playlistInterface( Mixed )->tracks() );
    }

    if ( rowCount( QModelIndex() ) == 0 && albums.count() == 1 )
    {
        setTitle( albums.first()->name() );
        setDescription( tr( "All tracks by %1 on album %2" ).arg( albums.first()->artist()->name() ).arg( albums.first()->name() ) );
        m_isTemporary = true;
    }
}


void
PlaylistModel::insertArtists( const QList< Tomahawk::artist_ptr >& artists, int row )
{
    // FIXME: This currently appends, not inserts!
    Q_UNUSED( row );

    foreach ( const artist_ptr& artist, artists )
    {
        if ( artist.isNull() )
            return;

        connect( artist.data(), SIGNAL( tracksAdded( QList<Tomahawk::query_ptr>, Tomahawk::ModelMode, Tomahawk::collection_ptr ) ),
                                  SLOT( appendQueries( QList<Tomahawk::query_ptr> ) ) );

        appendQueries( artist->playlistInterface( Mixed )->tracks() );
    }

    if ( rowCount( QModelIndex() ) == 0 && artists.count() == 1 )
    {
        setTitle( artists.first()->name() );
        setDescription( tr( "All tracks by %1" ).arg( artists.first()->name() ) );
        m_isTemporary = true;
    }
}


void
PlaylistModel::insertQueries( const QList< Tomahawk::query_ptr >& queries, int row )
{
    QList< Tomahawk::plentry_ptr > entries;
    foreach ( const query_ptr& query, queries )
    {
        if ( m_acceptPlayableQueriesOnly && query && query->resolvingFinished() && !query->playable() )
            continue;

        plentry_ptr entry = plentry_ptr( new PlaylistEntry() );

        entry->setDuration( query->displayQuery()->duration() );
        entry->setLastmodified( 0 );
        QString annotation = "";
        if ( !query->property( "annotation" ).toString().isEmpty() )
            annotation = query->property( "annotation" ).toString();
        entry->setAnnotation( annotation );

        entry->setQuery( query );
        entry->setGuid( uuid() );

        entries << entry;
    }

    insertEntries( entries, row );
}


void
PlaylistModel::insertEntries( const QList< Tomahawk::plentry_ptr >& entries, int row )
{
    if ( !entries.count() )
    {
        emit itemCountChanged( rowCount( QModelIndex() ) );
        finishLoading();
        return;
    }

    int c = row;
    QPair< int, int > crows;
    crows.first = c;
    crows.second = c + entries.count() - 1;

    if ( !m_isLoading )
    {
        m_savedInsertPos = row;
        m_savedInsertTracks = entries;
    }

    emit beginInsertRows( QModelIndex(), crows.first, crows.second );

    QList< Tomahawk::query_ptr > queries;
    int i = 0;
    PlayableItem* plitem;
    foreach( const plentry_ptr& entry, entries )
    {
        plitem = new PlayableItem( entry, rootItem(), row + i );
        plitem->index = createIndex( row + i, 0, plitem );
        i++;

        if ( entry->query()->id() == currentItemUuid() )
            setCurrentIndex( plitem->index );

        if ( !entry->query()->resolvingFinished() && !entry->query()->playable() )
        {
            queries << entry->query();
            m_waitingForResolved.append( entry->query().data() );
            connect( entry->query().data(), SIGNAL( resolvingFinished( bool ) ), SLOT( trackResolved( bool ) ) );
        }

        connect( plitem, SIGNAL( dataChanged() ), SLOT( onDataChanged() ) );

        WebResultHintChecker::checkQuery( entry->query() );
    }

    if ( !m_waitingForResolved.isEmpty() )
    {
        Pipeline::instance()->resolve( queries );
        emit loadingStarted();
    }
    else
        finishLoading();

    emit endInsertRows();
    emit itemCountChanged( rowCount( QModelIndex() ) );
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

    if ( m_waitingForResolved.contains( q ) )
    {
        m_waitingForResolved.removeAll( q );
        disconnect( q, SIGNAL( resolvingFinished( bool ) ), this, SLOT( trackResolved( bool ) ) );
    }

    if ( m_waitingForResolved.isEmpty() )
    {
        emit loadingFinished();
    }
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
    QMimeData* d = PlayableModel::mimeData( indexes );
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

        insertQueries( tracks, beginRow );

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

    if ( m_savedInsertPos >= 0 && !m_savedInsertTracks.isEmpty() &&
         !m_savedRemoveTracks.isEmpty() )
    {
        // If we have *both* an insert and remove, then it's a move action
        // However, since we got the insert before the remove (Qt...), the index we have as the saved
        // insert position is no longer valid. Find the proper one by finding the location of the first inserted
        // track
        for ( int i = 0; i < rowCount( QModelIndex() ); i++ )
        {
            const QModelIndex idx = index( i, 0, QModelIndex() );
            if ( !idx.isValid() )
                continue;
            const PlayableItem* item = itemFromIndex( idx );
            if ( !item || item->entry().isNull() )
                continue;

//             qDebug() << "Checking for equality:" << (item->entry() == m_savedInsertTracks.first()) << m_savedInsertTracks.first()->query()->track() << m_savedInsertTracks.first()->query()->artist();
            if ( item->entry() == m_savedInsertTracks.first() )
            {
                // Found our index
                emit m_playlist->tracksMoved( m_savedInsertTracks, i );
                break;
            }
        }
        m_savedInsertPos = -1;
        m_savedInsertTracks.clear();
        m_savedRemoveTracks.clear();
    }
    else if ( m_savedInsertPos >= 0 )
    {
        emit m_playlist->tracksInserted( m_savedInsertTracks, m_savedInsertPos );
        m_savedInsertPos = -1;
        m_savedInsertTracks.clear();
    }
    else if ( !m_savedRemoveTracks.isEmpty() )
    {
        emit m_playlist->tracksRemoved( m_savedRemoveTracks );
        m_savedRemoveTracks.clear();
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

        PlayableItem* item = itemFromIndex( idx );
        if ( item )
            l << item->entry();
    }

    return l;
}


void
PlaylistModel::removeIndex( const QModelIndex& index, bool moreToCome )
{
    PlayableItem* item = itemFromIndex( index );

    if ( item && m_waitingForResolved.contains( item->query().data() ) )
    {
        disconnect( item->query().data(), SIGNAL( resolvingFinished( bool ) ), this, SLOT( trackResolved( bool ) ) );
        m_waitingForResolved.removeAll( item->query().data() );
        if ( m_waitingForResolved.isEmpty() )
            emit loadingFinished();
    }

    if ( !m_changesOngoing )
        beginPlaylistChanges();

    if ( item && !m_isLoading )
        m_savedRemoveTracks << item->query();

    PlayableModel::removeIndex( index, moreToCome );

    if ( !moreToCome )
        endPlaylistChanges();
}


bool
PlaylistModel::isTemporary() const
{
    return m_isTemporary;
}
