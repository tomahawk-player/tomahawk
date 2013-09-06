/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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

#include "DynamicPlaylist_p.h"

#include "collection/Collection.h"
#include "database/Database.h"
#include "database/DatabaseCommand.h"
#include "database/DatabaseCommand_CreateDynamicPlaylist.h"
#include "database/DatabaseCommand_SetDynamicPlaylistRevision.h"
#include "database/DatabaseCommand_LoadDynamicPlaylistEntries.h"
#include "database/DatabaseCommand_DeleteDynamicPlaylist.h"
#include "utils/Logger.h"

#include "GeneratorFactory.h"
#include "Playlist_p.h"
#include "PlaylistInterface.h"
#include "PlaylistEntry.h"
#include "PlaylistInterface.h"
#include "SourceList.h"


using namespace Tomahawk;


DynamicPlaylist::DynamicPlaylist( const Tomahawk::source_ptr& author, const QString& type )
    : Playlist( new DynamicPlaylistPrivate( this, author ) )
{
    Q_D( DynamicPlaylist );
    qDebug() << Q_FUNC_INFO << "JSON";
    d->generator = geninterface_ptr( GeneratorFactory::create( type ) );
}


DynamicPlaylist::~DynamicPlaylist()
{
}


// Called by loadAllPlaylists command
DynamicPlaylist::DynamicPlaylist ( const Tomahawk::source_ptr& src,
                                   const QString& currentrevision,
                                   const QString& title,
                                   const QString& info,
                                   const QString& creator,
                                   uint createdOn,
                                   const QString& type,
                                   GeneratorMode mode,
                                   bool shared,
                                   int lastmod,
                                   const QString& guid )
    : Playlist( new DynamicPlaylistPrivate( this, src, currentrevision, title, info, creator, createdOn, shared, lastmod, guid, false ) )
{
    // TODO instantiate generator
    Q_D( DynamicPlaylist );
    d->generator = geninterface_ptr( GeneratorFactory::create( type ) );
    d->generator->setMode( mode );
}


// called when a new playlist is created (no currentrevision, new guid)
DynamicPlaylist::DynamicPlaylist( const Tomahawk::source_ptr& author,
                                  const QString& guid,
                                  const QString& title,
                                  const QString& info,
                                  const QString& creator,
                                  const QString& type,
                                  GeneratorMode mode,
                                  bool shared,
                                  bool autoLoad )
    : Playlist( new DynamicPlaylistPrivate( this, author, QString(), title, info, creator, 0, shared, 0, guid, autoLoad ) )
{
    Q_D( DynamicPlaylist );
    d->generator = geninterface_ptr( GeneratorFactory::create( type ) );
    d->generator->setMode( mode );
}


geninterface_ptr
DynamicPlaylist::generator() const
{
    Q_D( const DynamicPlaylist );
    return d->generator;
}


int
DynamicPlaylist::mode() const
{
    Q_D( const DynamicPlaylist );
    return d->generator->mode();
}


void
DynamicPlaylist::setGenerator( const Tomahawk::geninterface_ptr& gen_ptr )
{
    Q_D( DynamicPlaylist );
    d->generator = gen_ptr;
}


QString
DynamicPlaylist::type() const
{
    Q_D( const DynamicPlaylist );
    return d->generator->type();
}


void
DynamicPlaylist::setMode( int mode )
{
    Q_D( DynamicPlaylist );
    d->generator->setMode( (GeneratorMode)mode );
}


dynplaylist_ptr
DynamicPlaylist::get( const QString& guid )
{
    dynplaylist_ptr p;

    foreach( const Tomahawk::source_ptr& source, SourceList::instance()->sources() )
    {
        p = source->dbCollection()->autoPlaylist( guid );
        if ( !p )
            p = source->dbCollection()->station( guid );

        if ( p )
            return p;
    }

    return p;
}


dynplaylist_ptr
DynamicPlaylist::create( const Tomahawk::source_ptr& author,
                         const QString& guid,
                         const QString& title,
                         const QString& info,
                         const QString& creator,
                         GeneratorMode mode,
                         bool shared,
                         const QString& type,
                         bool autoLoad
                       )
{
    dynplaylist_ptr dynplaylist = Tomahawk::dynplaylist_ptr( new DynamicPlaylist( author, guid, title, info, creator, type, mode, shared, autoLoad ), &QObject::deleteLater );
    dynplaylist->setWeakSelf( dynplaylist.toWeakRef() );

    DatabaseCommand_CreateDynamicPlaylist* cmd = new DatabaseCommand_CreateDynamicPlaylist( author, dynplaylist, autoLoad );
    connect( cmd, SIGNAL(finished()), dynplaylist.data(), SIGNAL(created()) );
    Database::instance()->enqueue( Tomahawk::dbcmd_ptr(cmd) );
    if ( autoLoad )
        dynplaylist->reportCreated( dynplaylist );

    return dynplaylist;
}


void
DynamicPlaylist::setWeakSelf( QWeakPointer< DynamicPlaylist > self )
{
    Q_D( DynamicPlaylist );
    d->weakSelf = self;
}


void
DynamicPlaylist::createNewRevision( const QString& newUuid )
{
    if ( mode() == Static )
    {
        createNewRevision( newUuid.isEmpty() ? uuid() : newUuid, currentrevision(), type(), generator()->controls(), entries() );
    }
    else if ( mode() == OnDemand )
    {
        createNewRevision( newUuid.isEmpty() ? uuid() : newUuid, currentrevision(), type(), generator()->controls() );
    }
}


// create a new revision that will be a static playlist, as it has entries
void
DynamicPlaylist::createNewRevision( const QString& newrev,
                                    const QString& oldrev,
                                    const QString& type,
                                    const QList< dyncontrol_ptr>& controls,
                                    const QList< plentry_ptr >& entries )
{
    Q_D( DynamicPlaylist );
    Q_ASSERT( Playlist::d_func()->source->isLocal() || newrev == oldrev );

    if ( busy() )
    {
        d->revisionQueue.enqueue( DynQueueItem( newrev, oldrev, type, controls, (int)Static, entries, oldrev == currentrevision() ) );
        return;
    }

    setBusy( true );

    // get the newly added tracks
    QList< plentry_ptr > added = newEntries( entries );

    QStringList orderedguids;
    for( int i = 0; i < entries.size(); ++i )
        orderedguids << entries.at(i)->guid();

    // no conflict resolution or partial updating for controls. all or nothing baby

    // source making the change (local user in this case)
    source_ptr author = SourceList::instance()->getLocal();
    // command writes new rev to DB and calls setRevision, which emits our signal
    DatabaseCommand_SetDynamicPlaylistRevision* cmd =
    new DatabaseCommand_SetDynamicPlaylistRevision( author,
                                                    guid(),
                                                    newrev,
                                                    oldrev,
                                                    orderedguids,
                                                    added,
                                                    entries,
                                                    type,
                                                    Static,
                                                    controls );
    if ( !d->autoLoad )
        cmd->setPlaylist( this );

    Database::instance()->enqueue( Tomahawk::dbcmd_ptr( cmd ) );
}


// create a new revision that will be an ondemand playlist, as it has no entries
void
DynamicPlaylist::createNewRevision( const QString& newrev,
                                    const QString& oldrev,
                                    const QString& type,
                                    const QList< dyncontrol_ptr>& controls )
{
    Q_D( DynamicPlaylist );
    Q_ASSERT( Playlist::d_func()->source->isLocal() || newrev == oldrev );

    if ( busy() )
    {
        d->revisionQueue.enqueue( DynQueueItem( newrev, oldrev, type, controls, (int)OnDemand, QList< plentry_ptr >(), oldrev == currentrevision() ) );
        return;
    }

    setBusy( true );

    // can skip the entry stuff. just overwrite with new info
    source_ptr author = SourceList::instance()->getLocal();
    // command writes new rev to DB and calls setRevision, which emits our signal
    DatabaseCommand_SetDynamicPlaylistRevision* cmd =
    new DatabaseCommand_SetDynamicPlaylistRevision( author,
                                                    guid(),
                                                    newrev,
                                                    oldrev,
                                                    type,
                                                    OnDemand,
                                                    controls );
    if ( !d->autoLoad )
        cmd->setPlaylist( this );

    Database::instance()->enqueue( Tomahawk::dbcmd_ptr( cmd ) );
}


void
DynamicPlaylist::loadRevision( const QString& rev )
{
    Q_D( DynamicPlaylist );
    //tDebug() << Q_FUNC_INFO << "Loading with:" << ( rev.isEmpty() ? currentrevision() : rev );

    setBusy( true );
    DatabaseCommand_LoadDynamicPlaylistEntries* cmd = new DatabaseCommand_LoadDynamicPlaylistEntries( rev.isEmpty() ? currentrevision() : rev );

    if ( d->generator->mode() == OnDemand ) {
        connect( cmd, SIGNAL( done( QString,
                                    bool,
                                    QString,
                                    QList< QVariantMap >,
                                    bool ) ),
                 SLOT( setRevision( QString,
                                    bool,
                                    QString,
                                    QList< QVariantMap >,
                                    bool) ) );
    } else if ( d->generator->mode() == Static ) {
        connect( cmd, SIGNAL( done( QString,
                                    QList< QString >,
                                    QList< QString >,
                                    QString,
                                    QList< QVariantMap >,
                                    bool,
                                    QMap< QString, Tomahawk::plentry_ptr >,
                                    bool ) ),
                 SLOT( setRevision( QString,
                                    QList< QString >,
                                    QList< QString >,
                                    QString,
                                    QList< QVariantMap >,
                                    bool,
                                    QMap< QString, Tomahawk::plentry_ptr >,
                                    bool ) ) );

    }
    Database::instance()->enqueue( Tomahawk::dbcmd_ptr( cmd ) );
}


void
DynamicPlaylist::reportCreated( const Tomahawk::dynplaylist_ptr& self )
{
//    qDebug() << Q_FUNC_INFO;
    Q_ASSERT( self.data() == this );
    Q_ASSERT( !author().isNull() );
    Q_ASSERT( !author()->dbCollection().isNull() );
    // will emit Collection::playlistCreated(...)
    //    qDebug() << "Creating dynplaylist belonging to:" << author().data() << author().isNull();
    //    qDebug() << "REPORTING DYNAMIC PLAYLIST CREATED:" << this << author()->friendlyName();
    if ( self->mode() == Static )
        author()->dbCollection()->addAutoPlaylist( self );
    else
        author()->dbCollection()->addStation( self );
}


void
DynamicPlaylist::reportDeleted( const Tomahawk::dynplaylist_ptr& self )
{
//    qDebug() << Q_FUNC_INFO;
    Q_ASSERT( self.data() == this );
    // will emit Collection::playlistDeleted(...)
    if ( self->mode() == Static )
        author()->dbCollection()->deleteAutoPlaylist( self );
    else
        author()->dbCollection()->deleteStation( self );

    emit deleted( self );
}


void
DynamicPlaylist::addEntries( const QList< query_ptr >& queries )
{
    Q_D( DynamicPlaylist );
    Q_ASSERT( d->generator->mode() == Static );

    QList<plentry_ptr> el = entriesFromQueries( queries );

    QString newrev = uuid();
    createNewRevision( newrev, Playlist::d_func()->currentrevision, d->generator->type(), d->generator->controls(), el );
}


void
DynamicPlaylist::addEntry( const Tomahawk::query_ptr& query )
{
    QList<query_ptr> queries;
    queries << query;

    addEntries( queries );
}


void
DynamicPlaylist::setRevision( const QString& rev,
                              const QList< QString >& neworderedguids,
                              const QList< QString >& oldorderedguids,
                              const QString& type,
                              const QList< dyncontrol_ptr >& controls,
                              bool is_newest_rev,
                              const QMap< QString, plentry_ptr >& addedmap,
                              bool applied )
{
    Q_D( DynamicPlaylist );
    // we're probably being called by a database worker thread
    if ( QThread::currentThread() != thread() )
    {
        QMetaObject::invokeMethod( this,
                                   "setRevision",
                                   Qt::BlockingQueuedConnection,
                                   Q_ARG( QString,  rev ),
                                   Q_ARG( QList<QString> , neworderedguids ),
                                   Q_ARG( QList<QString> , oldorderedguids ),
                                   Q_ARG( QString , type ),
                                   QGenericArgument( "QList< Tomahawk::dyncontrol_ptr > " , (const void*)&controls ),
                                   Q_ARG( bool, is_newest_rev ),
                                   QGenericArgument( "QMap< QString,Tomahawk::plentry_ptr > " , (const void*)&addedmap ),
                                   Q_ARG( bool, applied ) );
        return;
    }

    if ( d->generator->type() != type ) { // new generator needed
        d->generator = GeneratorFactory::create( type );
    }

    d->generator->setControls( controls );
    d->generator->setMode( Static );

    DynamicPlaylistRevision dpr = setNewRevision( rev, neworderedguids, oldorderedguids, is_newest_rev, addedmap );
    dpr.applied = applied;
    dpr.controls = controls;
    dpr.type = type;
    dpr.mode = Static;

    if ( applied )
        setCurrentrevision( rev );

    //qDebug() << "EMITTING REVISION LOADED!";
    setBusy( false );
    setLoaded( true );
    emit dynamicRevisionLoaded( dpr );
}


void
DynamicPlaylist::setRevision( const QString& rev,
                              const QList< QString >& neworderedguids,
                              const QList< QString >& oldorderedguids,
                              const QString& type,
                              const QList< QVariantMap>& controlsV,
                              bool is_newest_rev,
                              const QMap< QString, Tomahawk::plentry_ptr >& addedmap,
                              bool applied )
{
    if ( QThread::currentThread() != thread() )
    {
        QMetaObject::invokeMethod( this,
                                   "setRevision",
                                   Qt::BlockingQueuedConnection,
                                   Q_ARG( QString,  rev ),
                                   Q_ARG( QList<QString> , neworderedguids ),
                                   Q_ARG( QList<QString> , oldorderedguids ),
                                   Q_ARG( QString , type ),
                                   QGenericArgument( "QList< QVariantMap > " , (const void*)&controlsV ),
                                   Q_ARG( bool, is_newest_rev ),
                                   QGenericArgument( "QMap< QString,Tomahawk::plentry_ptr > " , (const void*)&addedmap ),
                                   Q_ARG( bool, applied ) );
        return;
    }

    QList<dyncontrol_ptr> controls = variantsToControl( controlsV );
    setRevision( rev, neworderedguids, oldorderedguids, type, controls, is_newest_rev, addedmap, applied );
}


void
DynamicPlaylist::setRevision( const QString& rev,
                              bool is_newest_rev,
                              const QString& type,
                              const QList< dyncontrol_ptr >& controls,
                              bool applied )
{
    Q_D( DynamicPlaylist );
    if ( QThread::currentThread() != thread() )
    {
        QMetaObject::invokeMethod( this,
                                   "setRevision",
                                   Qt::BlockingQueuedConnection,
                                   Q_ARG( QString, rev ),
                                   Q_ARG( bool, is_newest_rev ),
                                   Q_ARG( QString, type ),
                                   QGenericArgument( "QList< Tomahawk::dyncontrol_ptr >" , (const void*)&controls ),
                                   Q_ARG( bool, applied ) );
        return;
    }

    if ( d->generator->type() != type )
    {
        // new generator needed
        d->generator = geninterface_ptr( GeneratorFactory::create( type ) );
    }

    d->generator->setControls( controls );
    d->generator->setMode( OnDemand );

    DynamicPlaylistRevision dpr;
    dpr.oldrevisionguid = currentrevision();
    dpr.revisionguid = rev;
    dpr.controls = controls;
    dpr.type = type;
    dpr.mode = OnDemand;

    if ( applied )
        setCurrentrevision( rev );

    //     qDebug() << "EMITTING REVISION LOADED 2!";
    setBusy( false );
    setLoaded( true );
    emit dynamicRevisionLoaded( dpr );
}


void
DynamicPlaylist::removeFromDatabase()
{
    Q_D( DynamicPlaylist );

    emit aboutToBeDeleted( d->weakSelf.toStrongRef() );
    DatabaseCommand_DeletePlaylist* cmd = new DatabaseCommand_DeleteDynamicPlaylist( author(), guid() ) ;
    Database::instance()->enqueue( Tomahawk::dbcmd_ptr( cmd ) );
}


void
DynamicPlaylist::setRevision( const QString& rev,
                              bool is_newest_rev,
                              const QString& type,
                              const QList< QVariantMap >& controlsV,
                              bool applied )
{
    if ( QThread::currentThread() != thread() )
    {
        QMetaObject::invokeMethod( this,
                                   "setRevision",
                                   Qt::BlockingQueuedConnection,
                                   Q_ARG( QString, rev ),
                                   Q_ARG( bool, is_newest_rev ),
                                   Q_ARG( QString, type ),
                                   QGenericArgument( "QList< QVariantMap >" , (const void*)&controlsV ),
                                   Q_ARG( bool, applied ) );
        return;
    }

    QList<dyncontrol_ptr> controls = variantsToControl( controlsV );
    setRevision( rev, is_newest_rev, type, controls, applied );
}


QList< dyncontrol_ptr >
DynamicPlaylist::variantsToControl( const QList< QVariantMap >& controlsV )
{
    QList<dyncontrol_ptr> realControls;
    foreach( QVariantMap controlV, controlsV )
    {
        dyncontrol_ptr control = GeneratorFactory::createControl( controlV.value( "type" ).toString(), controlV.value( "selectedType" ).toString() );
//        qDebug() << "Creating control with data:" << controlV;
        if ( control )
        {
            QJson::QObjectHelper::qvariant2qobject( controlV, control.data() );
            realControls << control;
        }
    }
    return realControls;
}


void
DynamicPlaylist::checkRevisionQueue()
{
    Q_D( DynamicPlaylist );
    if ( !d->revisionQueue.isEmpty() )
    {
        DynQueueItem item = d->revisionQueue.dequeue();
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

        if ( item.mode == Static )
            createNewRevision( item.newRev, item.oldRev, item.type, item.controls, item.entries );
        else
            createNewRevision( item.newRev, item.oldRev, item.type, item.controls );
    }
}


bool
DynamicPlaylist::autoLoad() const
{
    Q_D( const DynamicPlaylist );
    return d->autoLoad;
}
