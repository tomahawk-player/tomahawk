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


Tomahawk::collection_ptr 
Artist::collection() const
{
	return m_collection;
}