#include "databasecollection.h"

#include "tomahawk/tomahawkapp.h"
#include "database.h"
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
    // load our playlists
    DatabaseCommand_LoadAllPlaylists* cmd = new DatabaseCommand_LoadAllPlaylists( source() );
    connect( cmd,  SIGNAL( done( const QList<Tomahawk::playlist_ptr>& ) ),
                     SLOT( setPlaylists( const QList<Tomahawk::playlist_ptr>& ) ) );

    TomahawkApp::instance()->database()->enqueue(
            QSharedPointer<DatabaseCommand>( cmd )
    );
}


void
DatabaseCollection::loadAllTracks( boost::function<void( const QList<QVariant>&, collection_ptr )> callback )
{
    qDebug() << Q_FUNC_INFO << source()->userName();
    m_callback = callback;
    DatabaseCommand_AllTracks* cmd = new DatabaseCommand_AllTracks( source() );
    connect( cmd,  SIGNAL( done( const QList<QVariant>& ) ),
                     SLOT( callCallback( const QList<QVariant>& ) ) );

    TomahawkApp::instance()->database()->enqueue(
            QSharedPointer<DatabaseCommand>( cmd )
    );
}


void
DatabaseCollection::addTracks( const QList<QVariant> &newitems )
{
    qDebug() << Q_FUNC_INFO << newitems.length();
    DatabaseCommand_AddFiles* cmd = new DatabaseCommand_AddFiles( newitems, source() );
    TomahawkApp::instance()->database()->enqueue(
            QSharedPointer<DatabaseCommand>( cmd )
    );
}


void
DatabaseCollection::removeTracks( const QList<QVariant> &olditems )
{
    // FIXME
    Q_ASSERT( false );

    // TODO RemoveTracks cmd, probably builds a temp table of all the URLs in
    // olditems, then joins on that to batch-delete.
}


void
DatabaseCollection::callCallback( const QList<QVariant>& res )
{
    qDebug() << Q_FUNC_INFO << res.length() << this->source()->collection().data();
    m_callback( res, this->source()->collection() );
}
