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

#include "album.h"

#include "artist.h"
#include "database/database.h"
#include "database/databaseimpl.h"
#include "database/databasecommand_alltracks.h"
#include "query.h"

#include "utils/logger.h"

using namespace Tomahawk;


album_ptr
Album::get( const Tomahawk::artist_ptr& artist, const QString& name, bool autoCreate )
{
    int albid = Database::instance()->impl()->albumId( artist->id(), name, autoCreate );
    if ( albid < 1 )
        return album_ptr();

    return Album::get( albid, name, artist );
}


album_ptr
Album::get( unsigned int id, const QString& name, const Tomahawk::artist_ptr& artist )
{
    static QHash< unsigned int, album_ptr > s_albums;
    static QMutex s_mutex;

    QMutexLocker lock( &s_mutex );
    if ( s_albums.contains( id ) )
    {
        return s_albums.value( id );
    }

    album_ptr a = album_ptr( new Album( id, name, artist ) );
    if ( id > 0 )
        s_albums.insert( id, a );

    return a;
}


Album::Album( unsigned int id, const QString& name, const Tomahawk::artist_ptr& artist )
    : PlaylistInterface( this )
    , m_id( id )
    , m_name( name )
    , m_artist( artist )
    , m_currentItem( 0 )
    , m_currentTrack( 0 )
{
}


void
Album::onTracksAdded( const QList<Tomahawk::query_ptr>& tracks )
{
    qDebug() << Q_FUNC_INFO;

    m_queries << tracks;
    emit tracksAdded( tracks );
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
    m_currentItem = m_queries.at( p )->results().first();
    return m_currentItem;
}


bool
Album::hasNextItem()
{
    int p = m_currentTrack;
    p++;
    if ( p < 0 || p >= m_queries.count() )
        return false;

    return true;
}


QList<Tomahawk::query_ptr>
Album::tracks()
{
    if ( m_queries.isEmpty() )
    {
        DatabaseCommand_AllTracks* cmd = new DatabaseCommand_AllTracks();
        cmd->setAlbum( this );
        cmd->setSortOrder( DatabaseCommand_AllTracks::AlbumPosition );

        connect( cmd, SIGNAL( tracks( QList<Tomahawk::query_ptr>, QVariant ) ),
                        SLOT( onTracksAdded( QList<Tomahawk::query_ptr> ) ) );

        Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
    }

    return m_queries;
}
