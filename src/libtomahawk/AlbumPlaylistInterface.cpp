/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include "AlbumPlaylistInterface.h"

#include "Artist.h"
#include "database/database.h"
#include "database/DatabaseImpl.h"
#include "database/databasecommand_alltracks.h"
#include "Query.h"

#include "utils/logger.h"

using namespace Tomahawk;

AlbumPlaylistInterface::AlbumPlaylistInterface() {}

AlbumPlaylistInterface::AlbumPlaylistInterface( Tomahawk::Album *album )
    : Tomahawk::PlaylistInterface()
    , m_currentItem( 0 )
    , m_currentTrack( 0 )
    , m_album( QWeakPointer< Tomahawk::Album >( album ) )
{
}


AlbumPlaylistInterface::~AlbumPlaylistInterface()
{
    m_album.clear();
}


Tomahawk::result_ptr
AlbumPlaylistInterface::siblingItem( int itemsAway )
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


result_ptr
AlbumPlaylistInterface::currentItem() const
{
    return m_currentItem;
}


bool
AlbumPlaylistInterface::hasNextItem()
{
    int p = m_currentTrack;
    p++;
    if ( p < 0 || p >= m_queries.count() )
        return false;

    return true;
}


QList< Tomahawk::query_ptr >
AlbumPlaylistInterface::tracks()
{
    if ( m_queries.isEmpty() && m_album )
    {
        DatabaseCommand_AllTracks* cmd = new DatabaseCommand_AllTracks();
        cmd->setAlbum( m_album );
        cmd->setSortOrder( DatabaseCommand_AllTracks::AlbumPosition );
        //this takes discnumber into account as well

        connect( cmd,          SIGNAL( tracks( QList<Tomahawk::query_ptr>, QVariant ) ),
                 m_album.data(), SLOT( onTracksAdded( QList<Tomahawk::query_ptr> ) ) );

        Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
    }

    return m_queries;
}


void
AlbumPlaylistInterface::addQueries( const QList< query_ptr >& tracks )
{
    m_queries << tracks;
}
