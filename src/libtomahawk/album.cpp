#include "album.h"

#include <QDebug>

#include "collection.h"
#include "database/database.h"
#include "database/databasecommand_alltracks.h"

using namespace Tomahawk;


album_ptr
Album::get( unsigned int id, const QString& name, const Tomahawk::artist_ptr& artist, const Tomahawk::collection_ptr& collection )
{
    static QHash< unsigned int, album_ptr > s_albums;
    static QMutex s_mutex;

    QMutexLocker lock( &s_mutex );
    if ( s_albums.contains( id ) )
    {
        return s_albums.value( id );
    }

    album_ptr a = album_ptr( new Album( id, name, artist, collection ) );
    s_albums.insert( id, a );

    return a;
}


Album::Album( unsigned int id, const QString& name, const Tomahawk::artist_ptr& artist, const Tomahawk::collection_ptr& collection )
    : PlaylistInterface( this )
    , m_id( id )
    , m_name( name )
    , m_artist( artist )
    , m_collection( collection )
    , m_currentTrack( 0 )
{
}


void
Album::onTracksAdded( const QList<Tomahawk::query_ptr>& tracks, const Tomahawk::collection_ptr& collection )
{
    qDebug() << Q_FUNC_INFO;

    m_queries << tracks;
    emit tracksAdded( tracks, collection );
}


Tomahawk::result_ptr
Album::siblingItem( int itemsAway )
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
Album::tracks()
{
    if ( m_queries.isEmpty() )
    {
        DatabaseCommand_AllTracks* cmd = new DatabaseCommand_AllTracks( m_collection );
        cmd->setAlbum( this );
        cmd->setSortOrder( DatabaseCommand_AllTracks::AlbumPosition );

        connect( cmd, SIGNAL( tracks( QList<Tomahawk::query_ptr>, Tomahawk::collection_ptr ) ),
                        SLOT( onTracksAdded( QList<Tomahawk::query_ptr>, Tomahawk::collection_ptr ) ) );

        Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
    }

    return m_queries;
}
