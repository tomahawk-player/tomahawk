#include "dynamicplaylist.h"

#include "tomahawk/tomahawkapp.h"

using namespace Tomahawk;

// Called by loadAllPlaylists command
DynamicPlaylist::DynamicPlaylist ( const Tomahawk::source_ptr& src, 
                                   const QString& currentrevision,
                                   const QString& title, 
                                   const QString& info, 
                                   const QString& creator, 
                                   const QString& type, 
                                   bool shared, 
                                   int lastmod, 
                                   const QString& guid )
    : Playlist( src, currentrevision, title, info, creator, shared, lastmod, guid )
{
    qDebug() << "Creating Dynamic Playlist 1";
    // TODO instantiate generator
}


// called when a new playlist is created (no currentrevision, new guid)
DynamicPlaylist::DynamicPlaylist ( const Tomahawk::source_ptr& author, 
                                   const QString& guid, 
                                   const QString& title, 
                                   const QString& info, 
                                   const QString& creator, 
                                   const QString& type, 
                                   bool shared )
    : Playlist ( author, guid, title, info, creator, shared )
{
        qDebug() << "Creating Dynamic Playlist 2";
    // TODO instantiate generator
}

dynplaylist_ptr DynamicPlaylist::create( const Tomahawk::source_ptr& author, 
                                         const QString& guid, 
                                         const QString& title, 
                                         const QString& info, 
                                         const QString& creator, 
                                         bool shared )
{
    // TODO default generator?
    QString type = "default_generator";
    dynplaylist_ptr dynplaylist = new dynplaylist_ptr( new DynamicPlaylist( author, guid, title, info, creator, type, shared ) );
    
    DatabaseCommand_CreateDynamicPlaylist* cmd = new DatabaseCommand_CreateDynamicPlaylist( author, dynplaylist );
    connect( cmd, SIGNAL(finished()), dynplaylist.data(), SIGNAL(created()) );
    APP->database()->enqueue( QSharedPointer<DatabaseCommand>(cmd) );
    dynplaylist->reportCreated( dynplaylist );
    return dynplaylist;
    
}

// create a new revision that will be a static playlist, as it has entries
void DynamicPlaylist::createNewRevision( const QString& newrev, 
                                          const QString& oldrev, 
                                          const QString& type, 
                                          const QList< dyncontrol_ptr>& controls, 
                                          const QList< plentry_ptr >& entries )
{
    // get the newly added tracks
    QList< plentry_ptr > added = newEntries( entries );
    
    QStringList orderedguids;
    foreach( plentry_ptr p, entries )
        orderedguids << p->guid();
    
    // no conflict resolution or partial updating for controls. all or nothing baby
        
    // source making the change (localy user in this case)
    source_ptr author = APP->sourcelist().getLocal();
    // command writes new rev to DB and calls setRevision, which emits our signal
    DatabaseCommand_SetDynamicPlaylistRevision* cmd =
    new DatabaseCommand_SetDynamicPlaylistRevision( author,
                                                     guid(),
                                                     newrev,
                                                     oldrev,
                                                     orderedguids,
                                                     added,
                                                     type,
                                                     STATIC,
                                                    controls );
    APP->database()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
}

// create a new revision that will be an ondemand playlist, as it has no entries
void DynamicPlaylist::createNewRevision( const QString& newrev, 
                                          const QString& oldrev, 
                                          const QString& type, 
                                          const QList< dyncontrol_ptr>& controls )
{
    // can skip the entry stuff. just overwrite with new info
    source_ptr author = APP->sourcelist().getLocal();
    // command writes new rev to DB and calls setRevision, which emits our signal
    DatabaseCommand_SetDynamicPlaylistRevision* cmd =
    new DatabaseCommand_SetDynamicPlaylistRevision( author,
                                                    guid(),
                                                    newrev,
                                                    oldrev,
                                                    type,
                                                    STATIC,
                                                    controls );
    APP->database()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
}

void DynamicPlaylist::loadRevision( const QString& rev )
{
    qDebug() << Q_FUNC_INFO;
    
    DatabaseCommand_LoadDynamicPlaylist* cmd =
    new DatabaseCommand_LoadDynamicPlaylist( rev.isEmpty() ? currentrevision() : rev, m_generator->mode() );
    
    if( m_generator->mode() == ONDEMAND ) {
        connect( cmd, SIGNAL( done( QString,
                                    bool,
                                    const QString,
                                    QList< dyncontrol_ptr>,
                                    bool ) ),
                 SLOT( setRevision( QString,
                                    bool,
                                    const QString,
                                    QList< dyncontrol_ptr>,
                                    bool) ) );
    } else if( m_generator->mode() == STATIC ) {
        connect( cmd, SIGNAL( done( QString,
                                    QList< QString >,
                                    QList< QString >,
                                    QString,
                                    QList< dyncontrol_ptr>,
                                    bool,
                                    const QMap< QString, Tomahawk::plentry_ptr >,
                                    bool ) ),
                 SLOT( setRevision( QString,
                                    QList< QString >,
                                    QList< QString >,
                                    QString,
                                    QList< dyncontrol_ptr>,
                                    bool,
                                    const QMap< QString, Tomahawk::plentry_ptr >,
                                    bool ) ) );        
        
    }
    APP->database()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
}

bool DynamicPlaylist::remove( const Tomahawk::dynplaylist_ptr& playlist )
{
    return remove( playlist.staticCast<Tomahawk::playlist_ptr>() );
}

void DynamicPlaylist::reportCreated( const Tomahawk::dynplaylist_ptr& self )
{
    qDebug() << Q_FUNC_INFO;
    Q_ASSERT( self.data() == this );
    // will emit Collection::playlistCreated(...)
    m_source->collection()->addPlaylist( self.staticCast<Tomahawk::playlist_ptr>() );    
}

void DynamicPlaylist::reportDeleted( const Tomahawk::dynplaylist_ptr& self )
{
    qDebug() << Q_FUNC_INFO;
    Q_ASSERT( self.data() == this );
    // will emit Collection::playlistCreated(...)
    m_source->collection()->deletePlaylist( self.staticCast<Tomahawk::playlist_ptr>() ); 
}

// static version
void DynamicPlaylist::setRevision( const QString& rev, 
                                    const QList< QString >& neworderedguids, 
                                    const QList< QString >& oldorderedguids, 
                                    const QString& type, 
                                    const QList< dyncontrol_ptr>& controls, 
                                    bool is_newest_rev, 
                                    const QMap< QString, plentry_ptr >& addedmap, 
                                    bool applied )
{
    // we're probably being called by a database worker thread
    if( QThread::currentThread() != thread() )
    {
        QMetaObject::invokeMethod( this,
                                   "setRevision",
                                   Qt::BlockingQueuedConnection,
                                   Q_ARG( QString, rev ),
                                   Q_ARG( QList<QString>, neworderedguids ),
                                   Q_ARG( QList<QString>, oldorderedguids ),
                                   Q_ARG( QString, type ),
                                   QGenericArgument( "QList< dyncontrol_ptr >" , (const void*)&controls ),
                                   Q_ARG( bool, is_newest_rev ),
                                   QGenericArgument( "QMap< QString,Tomahawk::plentry_ptr >" , (const void*)&addedmap ),
                                   Q_ARG( bool, applied ) );
        return;
    }
    if( m_generator->type() != type ) { // new generator needed
        m_generator = generatorinterface_ptr( GeneratorFactory::create( type ) );
    }
    
    m_generator->setControls( controls );
    m_generator->setMode( ONDEMAND )
    
    DynamicPlaylistRevision pr = DynamicPlaylistRevision( setNewRevision( rev, neworderedguids, oldorderedguids, is_newest_rev, addedmap ) );
    pr.controls = controls;
    pr.type = type;
    pr.mode = STATIC;
    
    if( applied )
        setCurrentRevision( rev );
    pr.applied = applied;
    
    emit revisionLoaded( pr );    
}

// ondemand version
void DynamicPlaylist::setRevision( const QString& rev, 
                                    bool is_newest_rev, 
                                    const QString& type, 
                                    const QList< dyncontrol_ptr>& controls, 
                                    bool applied )
{
    if( m_generator->type() != type ) { // new generator needed
        m_generator = generatorinterface_ptr( GeneratorFactory::create( type ) );
    }

    m_generator->setControls( controls );
    m_generator->setMode( ONDEMAND )
    
    DynamicPlaylistRevision pr;
    pr.oldrevisionguid = m_currentrevision;
    pr.revisionguid = rev;
    pr.controls = controls;
    pr.type = type;
    pr.mode = ONDEMAND;
    
    emit revisionLoaded( pr );
}

