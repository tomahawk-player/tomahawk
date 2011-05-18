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

#include "playlist.h"

#include <QDomDocument>
#include <QDomElement>

#include "database/database.h"
#include "database/databasecommand_loadplaylistentries.h"
#include "database/databasecommand_setplaylistrevision.h"
#include "database/databasecommand_createplaylist.h"
#include "database/databasecommand_deleteplaylist.h"
#include "database/databasecommand_renameplaylist.h"

#include "pipeline.h"
#include "source.h"
#include "sourcelist.h"

using namespace Tomahawk;


PlaylistEntry::PlaylistEntry() {}
PlaylistEntry::~PlaylistEntry() {}

void
PlaylistEntry::setQueryVariant( const QVariant& v )
{
    QVariantMap m = v.toMap();

    QString artist = m.value( "artist" ).toString();
    QString album = m.value( "album" ).toString();
    QString track = m.value( "track" ).toString();
    m_query = Tomahawk::Query::get( artist, track, album );
}


QVariant
PlaylistEntry::queryVariant() const
{
    return m_query->toVariant();
}


void
PlaylistEntry::setQuery( const Tomahawk::query_ptr& q )
{
    m_query = q;
}


const Tomahawk::query_ptr&
PlaylistEntry::query() const
{
    return m_query;
}


source_ptr
PlaylistEntry::lastSource() const
{
    return m_lastsource;
}


void
PlaylistEntry::setLastSource( source_ptr s )
{
    m_lastsource = s;
}


Playlist::Playlist( const source_ptr& author )
    : m_source( author )
    , m_lastmodified( 0 )
{
    qDebug() << Q_FUNC_INFO << "JSON";
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
    , m_source( src )
    , m_currentrevision( currentrevision )
    , m_guid( guid == "" ? uuid() : guid )
    , m_title( title )
    , m_info( info )
    , m_creator( creator )
    , m_lastmodified( lastmod )
    , m_createdOn( createdOn )
    , m_shared( shared )
{
//    qDebug() << Q_FUNC_INFO << "1";
    init();
}


Playlist::Playlist( const source_ptr& author,
                    const QString& guid,
                    const QString& title,
                    const QString& info,
                    const QString& creator,
                    bool shared )
    : QObject()
    , m_source( author )
    , m_guid( guid )
    , m_title( title )
    , m_info ( info )
    , m_creator( creator )
    , m_lastmodified( 0 )
    , m_createdOn( 0 ) // will be set by db command
    , m_shared( shared )
{
    qDebug() << Q_FUNC_INFO << "2";
    init();
}


void
Playlist::init()
{
   m_locallyChanged = false;
   connect( Pipeline::instance(), SIGNAL( idle() ), SLOT( onResolvingFinished() ) );
}


Playlist::~Playlist() {}


playlist_ptr
Playlist::create( const source_ptr& author,
                  const QString& guid,
                  const QString& title,
                  const QString& info,
                  const QString& creator,
                  bool shared )
{
    playlist_ptr playlist( new Playlist( author, guid, title, info, creator, shared ) );
    // save to DB in the background
    // Hope this doesn't cause any problems..
    // Watch for the created() signal if you need to be sure it's written.
    //
    // When a playlist is created it will reportCreated(), adding it to the
    // collection it belongs to and emitting the appropriate signal.
    // When we create a new playlist for the local source here we call reportCreated()
    // immediately, so the GUI can reflect the new playlist without waiting for the DB sync
    //
    // When createplaylist DBOPs come from peers, the postCommitHook will call
    // reportCreated for us automatically, which should cause new playlists to be added to the GUI.

    DatabaseCommand_CreatePlaylist* cmd = new DatabaseCommand_CreatePlaylist( author, playlist );
    connect( cmd, SIGNAL(finished()), playlist.data(), SIGNAL(created()) );
    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>(cmd) );
    playlist->reportCreated( playlist );
    return playlist;
}


playlist_ptr
Playlist::load( const QString& guid )
{
    playlist_ptr p;

    foreach( const Tomahawk::source_ptr& source, SourceList::instance()->sources() )
    {
        p = source->collection()->playlist( guid );
        if ( !p.isNull() )
            return p;
    }

    return p;
}


bool
Playlist::remove( const playlist_ptr& playlist )
{
    DatabaseCommand_DeletePlaylist* cmd = new DatabaseCommand_DeletePlaylist( playlist->author(), playlist->guid() );
    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>(cmd) );

    return true; // FIXME
}


bool
Playlist::rename( const QString& title )
{
    DatabaseCommand_RenamePlaylist* cmd = new DatabaseCommand_RenamePlaylist( author(), guid(), title );
    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>(cmd) );

    return true; // FIXME
}


void
Playlist::reportCreated( const playlist_ptr& self )
{
    qDebug() << Q_FUNC_INFO;
    Q_ASSERT( self.data() == this );
    // will emit Collection::playlistCreated(...)
    m_source->collection()->addPlaylist( self );
}


void
Playlist::reportDeleted( const Tomahawk::playlist_ptr& self )
{
    qDebug() << Q_FUNC_INFO;
    Q_ASSERT( self.data() == this );
    m_source->collection()->deletePlaylist( self );

    emit deleted( self );
}


void
Playlist::loadRevision( const QString& rev )
{
    qDebug() << Q_FUNC_INFO << currentrevision() << rev << m_title;

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

    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
}


//public, model can call this if user changes a playlist:
void
Playlist::createNewRevision( const QString& newrev, const QString& oldrev, const QList< plentry_ptr >& entries )
{
   // qDebug() << "m_entries guids:";
    // calc list of newly added entries:
    QList<plentry_ptr> added = newEntries( entries );
    QStringList orderedguids;
    foreach( plentry_ptr p, entries )
        orderedguids << p->guid();

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
                                                     entries
                                                   );
    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
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
    if( QThread::currentThread() != thread() )
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

    if( applied )
        m_currentrevision = rev;
    pr.applied = applied;

    foreach( const plentry_ptr& entry, m_entries )
    {
        connect( entry->query().data(), SIGNAL( resultsAdded( QList<Tomahawk::result_ptr> ) ),
                 SLOT( onResultsFound( QList<Tomahawk::result_ptr> ) ), Qt::UniqueConnection );

    }
    emit revisionLoaded( pr );
}


PlaylistRevision
Playlist::setNewRevision( const QString& rev,
                                 const QList<QString>& neworderedguids,
                                 const QList<QString>& oldorderedguids,
                                 bool is_newest_rev,
                                 const QMap< QString, Tomahawk::plentry_ptr >& addedmap )
{
    qDebug() << Q_FUNC_INFO << rev << is_newest_rev << m_title << addedmap.count() << neworderedguids.count() << oldorderedguids.count();
    // build up correctly ordered new list of plentry_ptrs from
    // existing ones, and the ones that have been added
    QMap<QString, plentry_ptr> entriesmap;
    foreach( const plentry_ptr& p, m_entries )
        entriesmap.insert( p->guid(), p );

    QList<plentry_ptr> entries;
    foreach( const QString& id, neworderedguids )
    {
        qDebug() << "id:" << id;
        qDebug() << "newordered:" << neworderedguids.count() << neworderedguids;
        qDebug() << "entriesmap:" << entriesmap.count() << entriesmap;
        qDebug() << "addedmap:" << addedmap.count() << addedmap;
        qDebug() << "m_entries" << m_entries;

        if( entriesmap.contains( id ) )
        {
            entries.append( entriesmap.value( id ) );
        }
        else if( addedmap.contains( id ) )
        {
            entries.append( addedmap.value( id ) );
            if( is_newest_rev )
                m_entries.append( addedmap.value( id ) );
        }
        else
        {
            Q_ASSERT( false ); // XXX
        }
    }

    //qDebug() << Q_FUNC_INFO << rev << entries.length() << applied;

    PlaylistRevision pr;
    pr.oldrevisionguid = m_currentrevision;
    pr.revisionguid = rev;

    // entries that have been removed:
    QSet<QString> removedguids = oldorderedguids.toSet().subtract( neworderedguids.toSet() );
    //qDebug() << "Removedguids:" << removedguids << "oldorederedguids" << oldorderedguids << "newog" << neworderedguids;
    foreach( QString remid, removedguids )
    {
        // NB: entriesmap will contain old/removed entries only if the removal was done
        // in the same session - after a restart, history is not in memory.
        if( entriesmap.contains( remid ) )
        {
            pr.removed << entriesmap.value( remid );
            if( is_newest_rev )
            {
                //qDebug() << "Removing from m_entries" << remid;
                for( int k = 0 ; k < m_entries.length(); ++k )
                {
                    if( m_entries.at( k )->guid() == remid )
                    {
                        //qDebug() << "removed at" << k;
                        m_entries.removeAt( k );
                        break;
                    }
                }
            }
        }
    }

    pr.added = addedmap.values();

    pr.newlist = entries;
    return pr;
}


source_ptr
Playlist::author() const
{
    return m_source;
}


void
Playlist::resolve()
{
    QList< query_ptr > qlist;
    foreach( const plentry_ptr& p, m_entries )
    {
        qlist << p->query();
    }

    Pipeline::instance()->resolve( qlist );
}


void
Playlist::onResultsFound( const QList<Tomahawk::result_ptr>& results )
{
    Q_UNUSED( results );
    m_locallyChanged = true;
}


void
Playlist::onResolvingFinished()
{
    if ( m_locallyChanged )
    {
        qDebug() << Q_FUNC_INFO;
        m_locallyChanged = false;
        createNewRevision( currentrevision(), currentrevision(), m_entries );
    }
}


void
Playlist::addEntry( const query_ptr& query, const QString& oldrev )
{
    QList<query_ptr> queries;
    queries << query;

    addEntries( queries, oldrev );
}


void
Playlist::addEntries( const QList<query_ptr>& queries, const QString& oldrev )
{
    //qDebug() << Q_FUNC_INFO;

    QList<plentry_ptr> el = addEntriesInternal( queries );

    QString newrev = uuid();
    createNewRevision( newrev, oldrev, el );
}


QList<plentry_ptr>
Playlist::addEntriesInternal( const QList<Tomahawk::query_ptr>& queries )
{
    QList<plentry_ptr> el = entries();
    foreach( const query_ptr& query, queries )
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

        el << e;
    }
    return el;
}


QList< plentry_ptr >
Playlist::newEntries( const QList< plentry_ptr >& entries )
{
    QSet<QString> currentguids;
    foreach( const plentry_ptr& p, m_entries )
        currentguids.insert( p->guid() ); // could be cached as member?

    // calc list of newly added entries:
    QList<plentry_ptr> added;
    foreach( const plentry_ptr& p, entries )
    {
        if( !currentguids.contains( p->guid() ) )
            added << p;
    }
    return added;
}


QList<Tomahawk::query_ptr>
Playlist::tracks()
{
    QList<Tomahawk::query_ptr> queries;
    foreach( const plentry_ptr& p, m_entries )
        queries << p->query();

    return queries;
}

