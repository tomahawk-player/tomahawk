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

#include "artistplaylistinterface.h"
#include "collection.h"
#include "database/database.h"
#include "database/databaseimpl.h"
#include "query.h"

#include "utils/logger.h"

using namespace Tomahawk;


Artist::Artist()
{
}


Artist::~Artist()
{
}


artist_ptr
Artist::get( const QString& name, bool autoCreate )
{
    int artid = Database::instance()->impl()->artistId( name, autoCreate );
    if ( artid < 1 && autoCreate )
        return artist_ptr();

    return Artist::get( artid, name );
}


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
    : QObject()
    , m_id( id )
    , m_name( name )
{
    m_sortname = DatabaseImpl::sortname( name, true );
}


void
Artist::onTracksAdded( const QList<Tomahawk::query_ptr>& tracks )
{
    qDebug() << Q_FUNC_INFO;

    Tomahawk::ArtistPlaylistInterface* api = dynamic_cast< Tomahawk::ArtistPlaylistInterface* >( playlistInterface().data() );
    if ( api )
        api->addQueries( tracks );
    emit tracksAdded( tracks );
}

Tomahawk::playlistinterface_ptr
Artist::playlistInterface()
{
    if ( m_playlistInterface.isNull() )
    {
        m_playlistInterface = Tomahawk::playlistinterface_ptr( new Tomahawk::ArtistPlaylistInterface( this ) );
    }

    return m_playlistInterface;
}
