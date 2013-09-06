/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include "Playlist_p.h"

#include "collection/Collection.h"
#include "database/Database.h"
#include "database/DatabaseCommand_LoadPlaylistEntries.h"
#include "database/DatabaseCommand_SetPlaylistRevision.h"
#include "database/DatabaseCommand_CreatePlaylist.h"
#include "database/DatabaseCommand_DeletePlaylist.h"
#include "database/DatabaseCommand_RenamePlaylist.h"
#include "playlist/PlaylistUpdaterInterface.h"
#include "utils/Logger.h"
#include "utils/Closure.h"

#include "Pipeline.h"
#include "PlaylistEntry.h"
#include "PlaylistPlaylistInterface.h"
#include "Source.h"
#include "SourceList.h"

#include <QDomDocument>
#include <QDomElement>

using namespace Tomahawk;

static QSharedPointer<PlaylistRemovalHandler> s_removalHandler;

Playlist::Playlist( const source_ptr& author )
    : d_ptr( new PlaylistPrivate( this, author ) )
{
}


// used when loading from DB:
Playlist::Playlist( const source_ptr& src,
                    const QString& currentrevision,
                    const QString& title,
                    const QString& info,
                    const QString& creator,
                    uint createdOn,
                    bool shared,
                    int lastmod,
                    const QString& guid )
    : QObject()
    , d_ptr( new PlaylistPrivate( this, src, currentrevision, title, info, creator, createdOn, shared, lastmod, guid ) )
{
    init();
}


Playlist::Playlist( const source_ptr& author,
                    const QString& guid,
                    const QString& title,
                    const QString& info,
                    const QString& creator,
                    bool shared,
                    const QList< Tomahawk::plentry_ptr >& entries )
    : QObject()
    , d_ptr( new PlaylistPrivate( this, author, guid, title, info, creator, shared, entries ) )
{
    init();
}

Playlist::Playlist( const source_ptr& author,
                    const QString& guid,
                    const QString& title,
                    const QString& info,
                    const QString& creator,
                    bool shared)
    : QObject()
    , d_ptr( new PlaylistPrivate( this, author, guid, title, info, creator, shared, QList< Tomahawk::plentry_ptr >() ) )
{
    init();
}


void
Playlist::init()
{
    Q_D( Playlist );

    d->busy = false;
    d->deleted = false;
    d->locallyChanged = false;
    d->loaded = false;

    connect( Pipeline::instance(), SIGNAL( idle() ), SLOT( onResolvingFinished() ) );
}


Playlist::~Playlist()
{
    delete d_ptr;
}

QSharedPointer<PlaylistRemovalHandler> Playlist::removalHandler()
{
    if ( s_removalHandler.isNull() )
    {
        s_removalHandler = QSharedPointer<PlaylistRemovalHandler>( new PlaylistRemovalHandler() );
    }

    return s_removalHandler;
}


playlist_ptr
Playlist::create( const source_ptr& author,
                  const QString& guid,
                  const QString& title,
                  const QString& info,
                  const QString& creator,
                  bool shared,
                  const QList<Tomahawk::query_ptr>& queries )
{
    QList< plentry_ptr > entries;
    foreach( const Tomahawk::query_ptr& query, queries )
    {
        plentry_ptr p( new PlaylistEntry );
        p->setGuid( uuid() );
        p->setDuration( query->track()->duration() );
        p->setLastmodified( 0 );
        p->setAnnotation( query->property( "annotation" ).toString() );
        p->setQuery( query );

        entries << p;
    }

    playlist_ptr playlist( new Playlist( author, guid, title, info, creator, shared, entries ), &QObject::deleteLater );
    playlist->setWeakSelf( playlist.toWeakRef() );

    // save to DB in the background
    // Watch for the created() signal if you need to be sure it's written.
    //
    // When a playlist is created it will reportCreated(), adding it to the
    // collection it belongs to and emitting the appropriate signal.
    // When we create a new playlist for the local Source.here we call reportCreated()
    // immediately, so the GUI can reflect the new playlist without waiting for the DB sync
    //
    // When createplaylist DBOPs come from peers, the postCommitHook will call
    // reportCreated for us automatically, which should cause new playlists to be added to the GUI.

    DatabaseCommand_CreatePlaylist* cmd = new DatabaseCommand_CreatePlaylist( author, playlist );
    connect( cmd, SIGNAL( finished() ), playlist.data(), SIGNAL( created() ) );
    Database::instance()->enqueue( dbcmd_ptr(cmd) );
    playlist->reportCreated( playlist );

    return playlist;
}


playlist_ptr
Playlist::get( const QString& guid )
{
    playlist_ptr p;

    foreach( const Tomahawk::source_ptr& source, SourceList::instance()->sources() )
    {
        p = source->dbCollection()->playlist( guid );
        if ( !p )
            p = source->dbCollection()->autoPlaylist( guid );
        if ( !p )
            p = source->dbCollection()->station( guid );

        if ( p )
            return p;
    }

    return p;
}


void
Playlist::rename( const QString& title )
{
    DatabaseCommand_RenamePlaylist* cmd = new DatabaseCommand_RenamePlaylist( author(), guid(), title );
    Database::instance()->enqueue( Tomahawk::dbcmd_ptr(cmd) );
}


void
Playlist::setTitle( const QString& title )
{
    Q_D( Playlist );

    if ( title == d->title )
        return;

    const QString oldTitle = d->title;
    d->title = title;

    emit changed();
    emit renamed( d->title, oldTitle );
}


void
Playlist::reportCreated( const playlist_ptr& self )
{
    Q_D( Playlist );

    Q_ASSERT( self.data() == this );
    d->source->dbCollection()->addPlaylist( self );
}


void
Playlist::reportDeleted( const Tomahawk::playlist_ptr& self )
{
    Q_D( Playlist );

    Q_ASSERT( self.data() == this );
    if ( !d->updaters.isEmpty() )
    {
        foreach( PlaylistUpdaterInterface* updater, d->updaters )
        {
            updater->remove();
        }
    }

    d->deleted = true;
    d->source->dbCollection()->deletePlaylist( self );

    emit deleted( self );
}


void
Playlist::addUpdater( PlaylistUpdaterInterface* updater )
{
    Q_D( Playlist );
    d->updaters << updater;

    connect( updater, SIGNAL( changed() ), this, SIGNAL( changed() ), Qt::UniqueConnection );
    connect( updater, SIGNAL( destroyed( QObject* ) ), this, SIGNAL( changed() ), Qt::QueuedConnection );

    emit changed();
}


void
Playlist::removeUpdater( PlaylistUpdaterInterface* updater )
{
    Q_D( Playlist );
    d->updaters.removeAll( updater );

    disconnect( updater, SIGNAL( changed() ), this, SIGNAL( changed() ) );
    disconnect( updater, SIGNAL( destroyed( QObject* ) ), this, SIGNAL( changed() ) );

    emit changed();
}


bool
Playlist::hasCustomDeleter() const
{
    Q_D( const Playlist );
    foreach ( PlaylistUpdaterInterface* updater, d->updaters )
    {
        if ( updater->hasCustomDeleter() )
            return true;
    }

    return false;
}


void
Playlist::loadRevision( const QString& rev )
{
//    qDebug() << Q_FUNC_INFO << currentrevision() << rev << m_title;

    setBusy( true );
    DatabaseCommand_LoadPlaylistEntries* cmd =
            new DatabaseCommand_LoadPlaylistEntries( rev.isEmpty() ? currentrevision() : rev );

    connect( cmd, SIGNAL( done( const QString&,
                                const QList<QString>&,
                                const QList<QString>&,
                                bool,
                                const QMap< QString, Tomahawk::plentry_ptr >&,
                                bool ) ),
                    SLOT( setRevision( const QString&,
                                       const QList<QString>&,
                                       const QList<QString>&,
                                       bool,
                                       const QMap< QString, Tomahawk::plentry_ptr >&,
                                       bool ) ) );

    Database::instance()->enqueue( Tomahawk::dbcmd_ptr( cmd ) );
}


//public, model can call this if user changes a playlist:
void
Playlist::createNewRevision( const QString& newrev, const QString& oldrev, const QList< plentry_ptr >& entries )
{
    Q_D( Playlist );
    tDebug() << Q_FUNC_INFO << newrev << oldrev << entries.count();
    Q_ASSERT( d->source->isLocal() || newrev == oldrev );

    if ( busy() )
    {
        d->revisionQueue.enqueue( RevisionQueueItem( newrev, oldrev, entries, oldrev == currentrevision() ) );
        return;
    }

    if ( newrev != oldrev )
        setBusy( true );

    // calc list of newly added entries:
    QList<plentry_ptr> added = newEntries( entries );
    QStringList orderedguids;
    qDebug() << "Inserting ordered GUIDs:";
    foreach( const plentry_ptr& p, entries )
    {
        qDebug() << p->guid() << p->query()->track()->toString();
        orderedguids << p->guid();
    }

    foreach( const plentry_ptr& p, added )
        qDebug() << p->guid();

    // source making the change (local user in this case)
    source_ptr author = SourceList::instance()->getLocal();
    // command writes new rev to DB and calls setRevision, which emits our signal
    DatabaseCommand_SetPlaylistRevision* cmd =
            new DatabaseCommand_SetPlaylistRevision( author,
                                                     guid(),
                                                     newrev,
                                                     oldrev,
                                                     orderedguids,
                                                     added,
                                                     entries );

    Database::instance()->enqueue( Tomahawk::dbcmd_ptr( cmd ) );
}


void
Playlist::updateEntries( const QString& newrev, const QString& oldrev, const QList< plentry_ptr >& entries )
{
    Q_D( Playlist );
    tDebug() << Q_FUNC_INFO << newrev << oldrev << entries.count();
    Q_ASSERT( d->source->isLocal() || newrev == oldrev );

    if ( busy() )
    {
        d->updateQueue.enqueue( RevisionQueueItem( newrev, oldrev, entries, oldrev == currentrevision() ) );
        return;
    }

    if ( newrev != oldrev )
        setBusy( true );

    QStringList orderedguids;
    foreach( const plentry_ptr& p, d->entries )
    {
        orderedguids << p->guid();
    }

    qDebug() << "Updating playlist metadata:" << entries;
    DatabaseCommand_SetPlaylistRevision* cmd =
    new DatabaseCommand_SetPlaylistRevision( SourceList::instance()->getLocal(),
                                             guid(),
                                             newrev,
                                             oldrev,
                                             orderedguids,
                                             entries );

    Database::instance()->enqueue( Tomahawk::dbcmd_ptr( cmd ) );
}


// private, called when we loadRevision, or by our friend class DatabaseCommand_SetPlaylistRevision
// used to save new revision data (either originating locally, or via network msg for syncing)
void
Playlist::setRevision( const QString& rev,
                       const QList<QString>& neworderedguids,
                       const QList<QString>& oldorderedguids,
                       bool is_newest_rev,
                       const QMap< QString, Tomahawk::plentry_ptr >& addedmap,
                       bool applied )
{
    Q_D( Playlist );
    if ( QThread::currentThread() != thread() )
    {
        QMetaObject::invokeMethod( this,
                                   "setRevision",
                                   Qt::BlockingQueuedConnection,
                                   Q_ARG( QString, rev ),
                                   Q_ARG( QList<QString>, neworderedguids ),
                                   Q_ARG( QList<QString>, oldorderedguids ),
                                   Q_ARG( bool, is_newest_rev ),
                                   QGenericArgument( "QMap< QString,Tomahawk::plentry_ptr >", (const void*)&addedmap ),
                                   Q_ARG( bool, applied )
                                 );
        return;
    }

    PlaylistRevision pr = setNewRevision( rev, neworderedguids, oldorderedguids, is_newest_rev, addedmap );

    Q_ASSERT( applied );
    if ( applied )
        d->currentrevision = rev;
    pr.applied = applied;

    foreach( const plentry_ptr& entry, d->entries )
    {
        connect( entry.data(), SIGNAL( resultChanged() ), SLOT( onResultsChanged() ), Qt::UniqueConnection );
    }

    setBusy( false );
    setLoaded( true );

    if ( d->initEntries.count() && currentrevision().isEmpty() )
    {
        // add initial tracks
        createNewRevision( uuid(), currentrevision(), d->initEntries );
        d->initEntries.clear();
    }
    else
        emit revisionLoaded( pr );

    checkRevisionQueue();
}


PlaylistRevision
Playlist::setNewRevision( const QString& rev,
                          const QList<QString>& neworderedguids,
                          const QList<QString>& oldorderedguids,
                          bool is_newest_rev,
                          const QMap< QString, Tomahawk::plentry_ptr >& addedmap )
{
    Q_D( Playlist );
    Q_UNUSED( oldorderedguids );
    Q_UNUSED( is_newest_rev );

    // build up correctly ordered new list of plentry_ptrs from
    // existing ones, and the ones that have been added
    QMap<QString, plentry_ptr> entriesmap;
    foreach ( const plentry_ptr& p, d->entries )
    {
        entriesmap.insert( p->guid(), p );
    }

    // re-build m_entries from neworderedguids. plentries come either from the old m_entries OR addedmap.
    d->entries.clear();

    foreach ( const QString& id, neworderedguids )
    {
        if ( entriesmap.contains( id ) )
        {
            d->entries.append( entriesmap.value( id ) );
        }
        else if ( addedmap.contains( id ) )
        {
            d->entries.append( addedmap.value( id ) );
        }
        else
        {
            tDebug() << "id:" << id;
            tDebug() << "newordered:" << neworderedguids.count() << neworderedguids;
            tDebug() << "entriesmap:" << entriesmap.count() << entriesmap;
            tDebug() << "addedmap:" << addedmap.count() << addedmap;
            tDebug() << "m_entries" << d->entries;

            tLog() << "Playlist error for playlist with guid" << guid() << "from source" << author()->friendlyName();
//             Q_ASSERT( false ); // XXX
        }
    }

    PlaylistRevision pr;
    pr.oldrevisionguid = d->currentrevision;
    pr.revisionguid = rev;
    pr.added = addedmap.values();
    pr.newlist = d->entries;

    return pr;
}

void
Playlist::removeFromDatabase()
{
    Q_D( Playlist );

    emit aboutToBeDeleted( d->weakSelf.toStrongRef() );
    DatabaseCommand_DeletePlaylist* cmd = new DatabaseCommand_DeletePlaylist( d->source, d->guid );
    Database::instance()->enqueue( Tomahawk::dbcmd_ptr( cmd ) );
}


Playlist::Playlist( PlaylistPrivate *d )
    : d_ptr( d )
{
    init();
}


source_ptr
Playlist::author() const
{
    Q_D( const Playlist );
    return d->source;
}


void
Playlist::resolve()
{
    Q_D( Playlist );
    QList< query_ptr > qlist;
    foreach( const plentry_ptr& p, d->entries )
    {
        qlist << p->query();
    }

    Pipeline::instance()->resolve( qlist );
}


void
Playlist::onResultsChanged()
{
    Q_D( Playlist );
    d->locallyChanged = true;
}


void
Playlist::onResolvingFinished()
{
    Q_D( Playlist );
    if ( d->locallyChanged && !d->deleted )
    {
        d->locallyChanged = false;
        createNewRevision( currentrevision(), currentrevision(), d->entries );
    }
}


void
Playlist::addEntry( const query_ptr& query )
{
    QList<query_ptr> queries;
    queries << query;

    addEntries( queries );
}


void
Playlist::addEntries( const QList<query_ptr>& queries )
{
    Q_D( Playlist );
    if ( !d->loaded )
    {
        tDebug() << Q_FUNC_INFO << "Queueing addEntries call!";
        loadRevision();
        d->queuedOps << NewClosure( 0, "", this, SLOT( addEntries( QList<Tomahawk::query_ptr> ) ), queries );
        return;
    }

    const QList<plentry_ptr> el = entriesFromQueries( queries );
    const int prevSize = d->entries.size();

    QString newrev = uuid();
    createNewRevision( newrev, d->currentrevision, el );

    // We are appending at end, so notify listeners.
    // PlaylistModel also emits during appends, but since we call
    // createNewRevision, it reloads instead of appends.
    const QList<plentry_ptr> added = el.mid( prevSize );
    tDebug( LOGVERBOSE ) << "Playlist got" << queries.size() << "tracks added, emitting tracksInserted with:" << added.size() << "at pos:" << prevSize - 1;
    emit tracksInserted( added, prevSize );
}


void
Playlist::insertEntries( const QList< query_ptr >& queries, const int position )
{
    Q_D( Playlist );
    if ( !d->loaded )
    {
        tDebug() << Q_FUNC_INFO << "Queueing insertEntries call!";
        loadRevision();
        d->queuedOps << NewClosure( 0, "", this, SLOT( insertEntries( QList<Tomahawk::query_ptr>, int ) ), queries, position );
        return;
    }

    QList<plentry_ptr> toInsert = entriesFromQueries( queries, true );
    QList<plentry_ptr> entries = d->entries;

    Q_ASSERT( position <= d->entries.size() );
    if ( position > d->entries.size() )
    {
        tDebug() << "ERROR trying to insert tracks past end of playlist! Appending!";
        addEntries( queries );
        return;
    }

    for ( int i = toInsert.size()-1; i >= 0; --i )
        entries.insert( position, toInsert.at(i) );

    createNewRevision( uuid(), d->currentrevision, entries );

    // We are appending at end, so notify listeners.
    // PlaylistModel also emits during appends, but since we call
    // createNewRevision, it reloads instead of appends.
    qDebug() << "Playlist got" << toInsert.size() << "tracks added, emitting tracksInserted at pos:" << position;
    emit tracksInserted( toInsert, position );
}


QList<plentry_ptr>
Playlist::entriesFromQueries( const QList<Tomahawk::query_ptr>& queries, bool clearFirst )
{
    QList<plentry_ptr> el;
    if ( !clearFirst )
        el = entries();

    foreach( const query_ptr& query, queries )
    {
        plentry_ptr e( new PlaylistEntry() );
        e->setGuid( uuid() );

        e->setDuration( query->track()->duration() );
        e->setLastmodified( 0 );
        QString annotation = "";
        if ( !query->property( "annotation" ).toString().isEmpty() )
            annotation = query->property( "annotation" ).toString();
        e->setAnnotation( annotation ); // FIXME
        e->setQuery( query );

        el << e;
    }
    return el;
}


QList< plentry_ptr >
Playlist::newEntries( const QList< plentry_ptr >& entries )
{
    Q_D( Playlist );
    QSet<QString> currentguids;
    foreach( const plentry_ptr& p, d->entries )
        currentguids.insert( p->guid() ); // could be cached as member?

    // calc list of newly added entries:
    QList<plentry_ptr> added;
    foreach( const plentry_ptr& p, entries )
    {
        if ( !currentguids.contains( p->guid() ) )
            added << p;
    }
    return added;
}


void
Playlist::setBusy( bool b )
{
    Q_D( Playlist );
    d->busy = b;
    emit changed();
}


void
Playlist::setLoaded( bool b )
{
    Q_D( Playlist );
    d->loaded = b;
}


void
Playlist::checkRevisionQueue()
{
    Q_D( Playlist );
    if ( !d->revisionQueue.isEmpty() )
    {
        RevisionQueueItem item = d->revisionQueue.dequeue();

        if ( item.oldRev != currentrevision() && item.applyToTip )
        {
            // this was applied to the then-latest, but the already-running operation changed it so it's out of date now. fix it
            if ( item.oldRev == item.newRev )
            {
                checkRevisionQueue();
                return;
            }

            item.oldRev = currentrevision();
        }
        createNewRevision( item.newRev, item.oldRev, item.entries );
    }
    if ( !d->updateQueue.isEmpty() )
    {
        RevisionQueueItem item = d->updateQueue.dequeue();

        if ( item.oldRev != currentrevision() && item.applyToTip )
        {
            // this was applied to the then-latest, but the already-running operation changed it so it's out of date now. fix it
            if ( item.oldRev == item.newRev )
            {
                checkRevisionQueue();
                return;
            }

            item.oldRev = currentrevision();
        }
        updateEntries( item.newRev, item.oldRev, item.entries );
    }

    if ( !d->queuedOps.isEmpty() )
    {
        _detail::Closure* next = d->queuedOps.dequeue();
        next->forceInvoke();
    }
}


void
Playlist::setWeakSelf( QWeakPointer< Playlist > self )
{
    Q_D( Playlist );
    d->weakSelf = self;
}


Tomahawk::playlistinterface_ptr
Playlist::playlistInterface()
{
    Q_D( Playlist );
    if ( d->playlistInterface.isNull() )
    {
        d->playlistInterface = Tomahawk::playlistinterface_ptr( new Tomahawk::PlaylistPlaylistInterface( this ) );
    }

    return d->playlistInterface;
}


QString
Playlist::currentrevision() const
{
    Q_D( const Playlist );
    return d->currentrevision;
}


QString
Playlist::title() const
{
    Q_D( const Playlist );
    return d->title;
}


QString
Playlist::info() const
{
    Q_D( const Playlist );
    return d->info;
}


QString
Playlist::creator() const
{
    Q_D( const Playlist );
    return d->creator;
}


QString
Playlist::guid() const
{
    Q_D( const Playlist );
    return d->guid;
}


bool
Playlist::shared() const
{
    Q_D( const Playlist );
    return d->shared;
}


unsigned int
Playlist::lastmodified() const
{
    Q_D( const Playlist );
    return d->lastmodified;
}


uint
Playlist::createdOn() const
{
    Q_D( const Playlist );
    return d->createdOn;
}


bool
Playlist::busy() const
{
    Q_D( const Playlist );
    return d->busy;
}


bool
Playlist::loaded() const
{
    Q_D( const Playlist );
    return d->loaded;
}


const QList<plentry_ptr>&
Playlist::entries()
{
    Q_D( const Playlist );
    return d->entries;
}


void
Playlist::setCurrentrevision( const QString& s )
{
    Q_D( Playlist );
    d->currentrevision = s;
}


void
Playlist::setInfo( const QString& s )
{
    Q_D( Playlist );
    d->info = s;
}


void
Playlist::setCreator( const QString& s )
{
    Q_D( Playlist );
    d->creator = s;
}


void
Playlist::setGuid( const QString& s )
{
    Q_D( Playlist );
    d->guid = s;
}


void
Playlist::setShared( bool b )
{
    Q_D( Playlist );
    d->shared = b;
}


void
Playlist::setCreatedOn( uint createdOn )
{
    Q_D( Playlist );
    d->createdOn = createdOn;
}


QList<PlaylistUpdaterInterface *>
Playlist::updaters() const
{
    Q_D( const Playlist );
    return d->updaters;
}


void
PlaylistRemovalHandler::remove( const playlist_ptr& playlist )
{
    playlist->removeFromDatabase();
}


PlaylistRemovalHandler::PlaylistRemovalHandler()
{
}
