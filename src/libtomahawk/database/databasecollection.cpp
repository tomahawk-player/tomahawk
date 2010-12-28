#include "databasecollection.h"

#include "database/database.h"
#include "databasecommand_alltracks.h"
#include "databasecommand_addfiles.h"
#include "databasecommand_loadallplaylists.h"

using namespace Tomahawk;


DatabaseCollection::DatabaseCollection( const source_ptr& src, QObject* parent )
    : Collection( src, QString( "dbcollection:%1" ).arg( src->userName() ), parent )
{
}


void
DatabaseCollection::loadPlaylists()
{
    qDebug() << Q_FUNC_INFO;
    DatabaseCommand_LoadAllPlaylists* cmd = new DatabaseCommand_LoadAllPlaylists( source() );

    connect( cmd,  SIGNAL( done( const QList<Tomahawk::playlist_ptr>& ) ),
                     SLOT( setPlaylists( const QList<Tomahawk::playlist_ptr>& ) ) );

    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
}


void
DatabaseCollection::loadTracks()
{
    qDebug() << Q_FUNC_INFO << source()->userName();
    DatabaseCommand_AllTracks* cmd = new DatabaseCommand_AllTracks( source()->collection() );

    connect( cmd,  SIGNAL( tracks( QList<Tomahawk::query_ptr>, Tomahawk::collection_ptr ) ),
                     SLOT( setTracks( QList<Tomahawk::query_ptr>, Tomahawk::collection_ptr ) ) );
/*    connect( cmd,  SIGNAL( done( Tomahawk::collection_ptr ) ),
                   SIGNAL( tracksFinished( Tomahawk::collection_ptr ) ) );*/

    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
}


void
DatabaseCollection::addTracks( const QList<QVariant> &newitems )
{
    qDebug() << Q_FUNC_INFO << newitems.length();
    DatabaseCommand_AddFiles* cmd = new DatabaseCommand_AddFiles( newitems, source() );

    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
}


void
DatabaseCollection::removeTracks( const QList<QVariant> &olditems )
{
    // FIXME
    Q_ASSERT( false );

    // TODO RemoveTracks cmd, probably builds a temp table of all the URLs in
    // olditems, then joins on that to batch-delete.
}


QList< Tomahawk::playlist_ptr >
DatabaseCollection::playlists()
{
    qDebug() << Q_FUNC_INFO;

    if ( Collection::playlists().isEmpty() )
    {
        loadPlaylists();
    }

    return Collection::playlists();
}


QList< Tomahawk::query_ptr >
DatabaseCollection::tracks()
{
    qDebug() << Q_FUNC_INFO;

    if ( Collection::tracks().isEmpty() )
    {
        loadTracks();
    }

    return Collection::tracks();
}
