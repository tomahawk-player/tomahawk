#include "artist.h"

#include <QDebug>

#include "collection.h"
#include "database/database.h"
#include "database/databasecommand_alltracks.h"

using namespace Tomahawk;

Artist::Artist() {}

Artist::~Artist() {}

artist_ptr
Artist::get( unsigned int id, const QString& name, const Tomahawk::collection_ptr& collection )
{
    static QHash< unsigned int, artist_ptr > s_artists;
    static QMutex s_mutex;

    QMutexLocker lock( &s_mutex );
    if ( s_artists.contains( id ) )
    {
        return s_artists.value( id );
    }

    artist_ptr a = artist_ptr( new Artist( id, name, collection ) );
    s_artists.insert( id, a );

    return a;
}


Artist::Artist( unsigned int id, const QString& name, const Tomahawk::collection_ptr& collection )
    : PlaylistInterface( this )
    , m_id( id )
    , m_name( name )
    , m_currentTrack( 0 )
    , m_collection( collection )
{
}


Tomahawk::collection_ptr 
Artist::collection() const
{
    return m_collection;
}


void
Artist::onTracksAdded( const QList<Tomahawk::query_ptr>& tracks )
{
    qDebug() << Q_FUNC_INFO;

    m_queries << tracks;
    emit tracksAdded( tracks );
}


Tomahawk::result_ptr
Artist::siblingItem( int itemsAway )
{
    int p = m_currentTrack;
    p += itemsAway;

    if ( p < 0 )
        return Tomahawk::result_ptr();

    if ( p >= m_queries.count() )
        return Tomahawk::result_ptr();

    m_currentTrack = p;
    return m_queries.at( p )->results().first();
}


QList<Tomahawk::query_ptr>
Artist::tracks()
{
    if ( m_queries.isEmpty() )
    {
        DatabaseCommand_AllTracks* cmd = new DatabaseCommand_AllTracks( m_collection );
        cmd->setArtist( this );
        cmd->setSortOrder( DatabaseCommand_AllTracks::Album );

        connect( cmd, SIGNAL( tracks( QList<Tomahawk::query_ptr> ) ),
                        SLOT( onTracksAdded( QList<Tomahawk::query_ptr> ) ) );

        Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
    }

    return m_queries;
}
