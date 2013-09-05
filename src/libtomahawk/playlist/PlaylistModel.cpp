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

#include "PlaylistModel_p.h"

#include <QMimeData>
#include <QTreeView>

#include "database/Database.h"
#include "database/DatabaseCommand_PlaybackHistory.h"
#include "playlist/dynamic/GeneratorInterface.h"
#include "utils/Logger.h"
#include "utils/TomahawkUtils.h"

#include "Album.h"
#include "Artist.h"
#include "DropJob.h"
#include "Pipeline.h"
#include "PlayableItem.h"
#include "PlaylistEntry.h"
#include "Source.h"
#include "SourceList.h"

using namespace Tomahawk;


void
PlaylistModel::init()
{
    Q_D( PlaylistModel );
    d->dropStorage.parent = QPersistentModelIndex();
    d->dropStorage.row = -10;

    setReadOnly( true );
}


PlaylistModel::PlaylistModel( QObject* parent )
    : PlayableModel( parent, new PlaylistModelPrivate( this )  )
{
    init();
}


PlaylistModel::PlaylistModel( QObject* parent, PlaylistModelPrivate* d )
    : PlayableModel( parent, d  )
{
    init();
}


PlaylistModel::~PlaylistModel()
{
}


QString
PlaylistModel::guid() const
{
    Q_D( const PlaylistModel );

    if ( d->playlist )
    {
        return QString( "playlistmodel/%1" ).arg( d->playlist->guid() );
    }
    else
        return QString();
}


void
PlaylistModel::loadPlaylist( const Tomahawk::playlist_ptr& playlist, bool loadEntries )
{
    Q_D( PlaylistModel );

    if ( d->playlist )
    {
        disconnect( d->playlist.data(), SIGNAL( revisionLoaded( Tomahawk::PlaylistRevision ) ), this, SLOT( onRevisionLoaded( Tomahawk::PlaylistRevision ) ) );
        disconnect( d->playlist.data(), SIGNAL( deleted( Tomahawk::playlist_ptr ) ), this, SIGNAL( playlistDeleted() ) );
        disconnect( d->playlist.data(), SIGNAL( changed() ), this, SLOT( onPlaylistChanged() ) );
    }

    if ( loadEntries )
    {
        startLoading();
        clear();
    }

    d->playlist = playlist;
    connect( playlist.data(), SIGNAL( revisionLoaded( Tomahawk::PlaylistRevision ) ), SLOT( onRevisionLoaded( Tomahawk::PlaylistRevision ) ) );
    connect( playlist.data(), SIGNAL( deleted( Tomahawk::playlist_ptr ) ), SIGNAL( playlistDeleted() ) );
    connect( playlist.data(), SIGNAL( changed() ), SLOT( onPlaylistChanged() ) );

    setReadOnly( !d->playlist->author()->isLocal() );
    d->isTemporary = false;

    onPlaylistChanged();
    if ( !loadEntries )
        return;

    if ( playlist->loaded() )
    {
        QList<plentry_ptr> entries = playlist->entries();
        appendEntries( entries );

        // finishLoading() will be called by appendEntries, so it can keep showing it until all tracks are resolved
        // finishLoading();

        /*    foreach ( const plentry_ptr& p, entries )
                  qDebug() << p->guid() << p->query()->track() << p->query()->artist(); */
    }
}


void
PlaylistModel::onPlaylistChanged()
{
    Q_D( PlaylistModel );
    QString age = TomahawkUtils::ageToString( QDateTime::fromTime_t( d->playlist->createdOn() ), true );
    QString desc;
    if ( d->playlist->creator().isEmpty() )
    {
        if ( d->playlist->author()->isLocal() )
        {
             desc = tr( "A playlist you created %1." )
                    .arg( age );
        }
        else
        {
            desc = tr( "A playlist by %1, created %2." )
                   .arg( d->playlist->author()->friendlyName() )
                   .arg( age );
        }
    }
    else
    {
        desc = tr( "A playlist by %1, created %2." )
               .arg( d->playlist->creator() )
               .arg( age );
    }

    setTitle( d->playlist->title() );
    setDescription( desc );

    emit playlistChanged();
}


void
PlaylistModel::clear()
{
    Q_D( PlaylistModel );
    d->waitingForResolved.clear();
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
    Q_D( PlaylistModel );
    // FIXME: This currently appends, not inserts!
    Q_UNUSED( row );

    foreach ( const album_ptr& album, albums )
    {
        if ( !album )
            return;

        connect( album.data(), SIGNAL( tracksAdded( QList<Tomahawk::query_ptr>, Tomahawk::ModelMode, Tomahawk::collection_ptr ) ),
                                 SLOT( appendQueries( QList<Tomahawk::query_ptr> ) ) );

        appendQueries( album->playlistInterface( Mixed )->tracks() );
    }

    if ( rowCount( QModelIndex() ) == 0 && albums.count() == 1 )
    {
        setTitle( albums.first()->name() );
        setDescription( tr( "All tracks by %1 on album %2" ).arg( albums.first()->artist()->name() ).arg( albums.first()->name() ) );
        d->isTemporary = true;
    }
}


void
PlaylistModel::insertArtists( const QList< Tomahawk::artist_ptr >& artists, int row )
{
    Q_D( PlaylistModel );
    // FIXME: This currently appends, not inserts!
    Q_UNUSED( row );

    foreach ( const artist_ptr& artist, artists )
    {
        if ( !artist )
            return;

        connect( artist.data(), SIGNAL( tracksAdded( QList<Tomahawk::query_ptr>, Tomahawk::ModelMode, Tomahawk::collection_ptr ) ),
                                  SLOT( appendQueries( QList<Tomahawk::query_ptr> ) ) );

        appendQueries( artist->playlistInterface( Mixed )->tracks() );
    }

    if ( rowCount( QModelIndex() ) == 0 && artists.count() == 1 )
    {
        setTitle( artists.first()->name() );
        setDescription( tr( "All tracks by %1" ).arg( artists.first()->name() ) );
        d->isTemporary = true;
    }
}


void
PlaylistModel::insertQueries( const QList< Tomahawk::query_ptr >& queries, int row, const QList< Tomahawk::PlaybackLog >& logs )
{
    Q_D( PlaylistModel );
    QList< Tomahawk::plentry_ptr > entries;
    foreach ( const query_ptr& query, queries )
    {
        if ( d->acceptPlayableQueriesOnly && query && query->resolvingFinished() && !query->playable() )
            continue;

        plentry_ptr entry = plentry_ptr( new PlaylistEntry() );

        entry->setDuration( query->track()->duration() );
        entry->setLastmodified( 0 );
        QString annotation = "";
        if ( !query->property( "annotation" ).toString().isEmpty() )
            annotation = query->property( "annotation" ).toString();
        entry->setAnnotation( annotation );

        entry->setQuery( query );
        entry->setGuid( uuid() );

        entries << entry;
    }

    insertEntries( entries, row, logs );
}


void
PlaylistModel::insertEntries( const QList< Tomahawk::plentry_ptr >& entries, int row, const QList< Tomahawk::PlaybackLog >& logs )
{
    Q_D( PlaylistModel );
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

    if ( !d->isLoading )
    {
        d->savedInsertPos = row;
        d->savedInsertTracks = entries;
    }

    emit beginInsertRows( QModelIndex(), crows.first, crows.second );

    QList< Tomahawk::query_ptr > queries;
    int i = 0;
    PlayableItem* plitem;
    foreach( const plentry_ptr& entry, entries )
    {
        plitem = new PlayableItem( entry, rootItem(), row + i );
        plitem->index = createIndex( row + i, 0, plitem );

        if ( logs.count() > i )
            plitem->setPlaybackLog( logs.at( i ) );

        i++;

        if ( entry->query()->id() == currentItemUuid() )
            setCurrentIndex( plitem->index );

        if ( !entry->query()->resolvingFinished() && !entry->query()->playable() )
        {
            queries << entry->query();
            d->waitingForResolved.append( entry->query().data() );
            connect( entry->query().data(), SIGNAL( resolvingFinished( bool ) ), SLOT( trackResolved( bool ) ) );
        }

        connect( plitem, SIGNAL( dataChanged() ), SLOT( onDataChanged() ) );
    }

    if ( !d->waitingForResolved.isEmpty() )
    {
        startLoading();
        Pipeline::instance()->resolve( queries );
    }
    else
    {
        finishLoading();
    }

    emit endInsertRows();
    emit itemCountChanged( rowCount( QModelIndex() ) );
}


void
PlaylistModel::trackResolved( bool )
{
    Q_D( PlaylistModel );
    Tomahawk::Query* q = qobject_cast< Query* >( sender() );
    if ( !q )
    {
        // Track has been removed from the playlist by now
        return;
    }

    if ( d->waitingForResolved.contains( q ) )
    {
        d->waitingForResolved.removeAll( q );
        disconnect( q, SIGNAL( resolvingFinished( bool ) ), this, SLOT( trackResolved( bool ) ) );
    }

    if ( d->waitingForResolved.isEmpty() )
    {
        finishLoading();
    }
}


void
PlaylistModel::onRevisionLoaded( Tomahawk::PlaylistRevision revision )
{
    Q_D( PlaylistModel );

    if ( !d->waitForRevision.contains( revision.revisionguid ) )
    {
        loadPlaylist( d->playlist );
    }
    else
    {
        d->waitForRevision.removeAll( revision.revisionguid );
    }
}


QMimeData*
PlaylistModel::mimeData( const QModelIndexList& indexes ) const
{
    Q_D( const PlaylistModel );

    // Add the playlist id to the mime data so that we can detect dropping on ourselves
    QMimeData* data = PlayableModel::mimeData( indexes );
    if ( d->playlist )
        data->setData( "application/tomahawk.playlist.id", d->playlist->guid().toLatin1() );

    return data;
}


bool
PlaylistModel::dropMimeData( const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent )
{
    Q_D( PlaylistModel );
    Q_UNUSED( column );

    if ( action == Qt::IgnoreAction || isReadOnly() )
        return true;

    if ( !DropJob::acceptsMimeData( data ) )
        return false;

    d->dropStorage.row = row;
    d->dropStorage.parent = QPersistentModelIndex( parent );
    d->dropStorage.action = action;

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
       ( d->playlist && data->data( "application/tomahawk.playlist.id" ) != d->playlist->guid() ) )
    {
        dj->setDropAction( DropJob::Append );
    }
#endif

    connect( dj, SIGNAL( tracks( QList< Tomahawk::query_ptr > ) ), SLOT( parsedDroppedTracks( QList< Tomahawk::query_ptr > ) ) );
    dj->tracksFromMimeData( data );

    return true;
}


playlist_ptr
PlaylistModel::playlist() const
{
    Q_D( const PlaylistModel );
    return d->playlist;
}


void
PlaylistModel::parsedDroppedTracks( QList< query_ptr > tracks )
{
    Q_D( PlaylistModel );
    if ( d->dropStorage.row == -10  ) // nope
        return;

    int beginRow;
    if ( d->dropStorage.row != -1 )
        beginRow = d->dropStorage.row;
    else if ( d->dropStorage.parent.isValid() )
        beginRow = d->dropStorage.parent.row();
    else
        beginRow = rowCount( QModelIndex() );

    if ( tracks.count() )
    {
        bool update = ( d->dropStorage.action & Qt::CopyAction || d->dropStorage.action & Qt::MoveAction );
        if ( update )
            beginPlaylistChanges();

        insertQueries( tracks, beginRow );

        if ( update && d->dropStorage.action & Qt::CopyAction )
            endPlaylistChanges();
    }

    d->dropStorage.parent = QPersistentModelIndex();
    d->dropStorage.row = -10;
}


void
PlaylistModel::beginPlaylistChanges()
{
    Q_D( PlaylistModel );
    if ( !d->playlist || !d->playlist->author()->isLocal() )
        return;

    Q_ASSERT( !d->changesOngoing );
    d->changesOngoing = true;
}


void
PlaylistModel::endPlaylistChanges()
{
    Q_D( PlaylistModel );

    if ( !d->playlist || !d->playlist->author()->isLocal() )
    {
        d->savedInsertPos = -1;
        d->savedInsertTracks.clear();
        d->savedRemoveTracks.clear();
        return;
    }

    if ( d->changesOngoing )
    {
        d->changesOngoing = false;
    }
    else
    {
        tDebug() << "Called" << Q_FUNC_INFO << "unexpectedly!";
        Q_ASSERT( false );
    }

    QList<plentry_ptr> l = playlistEntries();
    QString newrev = uuid();
    d->waitForRevision << newrev;

    if ( dynplaylist_ptr dynplaylist = d->playlist.dynamicCast<Tomahawk::DynamicPlaylist>() )
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
        d->playlist->createNewRevision( newrev, d->playlist->currentrevision(), l );
    }

    if ( d->savedInsertPos >= 0 && !d->savedInsertTracks.isEmpty() &&
         !d->savedRemoveTracks.isEmpty() )
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
            if ( !item || !item->entry() )
                continue;

//             qDebug() << "Checking for equality:" << (item->entry() == m_savedInsertTracks.first()) << m_savedInsertTracks.first()->query()->track() << m_savedInsertTracks.first()->query()->artist();
            if ( item->entry() == d->savedInsertTracks.first() )
            {
                // Found our index
                emit d->playlist->tracksMoved( d->savedInsertTracks, i );
                break;
            }
        }
        d->savedInsertPos = -1;
        d->savedInsertTracks.clear();
        d->savedRemoveTracks.clear();
    }
    else if ( d->savedInsertPos >= 0 )
    {
        emit d->playlist->tracksInserted( d->savedInsertTracks, d->savedInsertPos );
        d->savedInsertPos = -1;
        d->savedInsertTracks.clear();
    }
    else if ( !d->savedRemoveTracks.isEmpty() )
    {
        emit d->playlist->tracksRemoved( d->savedRemoveTracks );
        d->savedRemoveTracks.clear();
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
    Q_D( PlaylistModel );

    PlayableItem* item = itemFromIndex( index );
    if ( item && d->waitingForResolved.contains( item->query().data() ) )
    {
        disconnect( item->query().data(), SIGNAL( resolvingFinished( bool ) ), this, SLOT( trackResolved( bool ) ) );
        d->waitingForResolved.removeAll( item->query().data() );
        if ( d->waitingForResolved.isEmpty() )
            finishLoading();
    }

    if ( !d->changesOngoing )
        beginPlaylistChanges();

    if ( item && !d->isLoading )
        d->savedRemoveTracks << item->query();

    PlayableModel::removeIndex( index, moreToCome );

    if ( !moreToCome )
        endPlaylistChanges();
}


bool
PlaylistModel::waitForRevision( const QString& revisionguid ) const
{
    Q_D( const PlaylistModel );
    return d->waitForRevision.contains( revisionguid );
}


void
PlaylistModel::removeFromWaitList( const QString& revisionguid )
{
    Q_D( PlaylistModel );
    d->waitForRevision.removeAll( revisionguid );
}


bool
PlaylistModel::isTemporary() const
{
    Q_D( const PlaylistModel );
    return d->isTemporary;
}


bool
PlaylistModel::acceptPlayableQueriesOnly() const
{
    Q_D( const PlaylistModel );
    return d->acceptPlayableQueriesOnly;
}


void
PlaylistModel::setAcceptPlayableQueriesOnly( bool b )
{
    Q_D( PlaylistModel );
    d->acceptPlayableQueriesOnly = b;
}
