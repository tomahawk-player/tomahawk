#include "tomahawk/artist.h"

#include <QDebug>

#include "tomahawk/collection.h"
#include "tomahawk/tomahawkapp.h"
#include "database/database.h"
#include "database/databasecommand_alltracks.h"

using namespace Tomahawk;


artist_ptr
Artist::get( unsigned int id, const QString& name, const Tomahawk::collection_ptr& collection )
{
    static QHash< unsigned int, artist_ptr > s_artists;

    if ( s_artists.contains( id ) )
    {
        return s_artists.value( id );
    }

    artist_ptr a = artist_ptr( new Artist( id, name, collection ) );
    s_artists.insert( id, a );

    return a;
}


Artist::Artist( unsigned int id, const QString& name, const Tomahawk::collection_ptr& collection )
    : m_id( id )
    , m_name( name )
    , m_collection( collection )
{
}
