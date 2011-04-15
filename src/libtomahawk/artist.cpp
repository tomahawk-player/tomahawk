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

#include "artist.h"

#include <QDebug>

#include "collection.h"
#include "database/database.h"
#include "database/databasecommand_alltracks.h"

using namespace Tomahawk;

Artist::Artist() {}

Artist::~Artist() {}

artist_ptr
Artist::get( unsigned int id, const QString& name )
{
    static QHash< unsigned int, artist_ptr > s_artists;
    static QMutex s_mutex;

    QMutexLocker lock( &s_mutex );
    if ( s_artists.contains( id ) )
    {
        return s_artists.value( id );
    }

    artist_ptr a = artist_ptr( new Artist( id, name ) );
    if ( id > 0 )
        s_artists.insert( id, a );

    return a;
}


Artist::Artist( unsigned int id, const QString& name )
    : PlaylistInterface( this )
    , m_id( id )
    , m_name( name )
    , m_currentTrack( 0 )
{
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
        DatabaseCommand_AllTracks* cmd = new DatabaseCommand_AllTracks();
        cmd->setArtist( this );
        cmd->setSortOrder( DatabaseCommand_AllTracks::Album );

        connect( cmd, SIGNAL( tracks( QList<Tomahawk::query_ptr>, QVariant ) ),
                        SLOT( onTracksAdded( QList<Tomahawk::query_ptr> ) ) );

        Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
    }

    return m_queries;
}
