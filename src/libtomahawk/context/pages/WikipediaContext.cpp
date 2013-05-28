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

#include "WikipediaContext.h"
#include "Source.h"
#include "Track.h"

using namespace Tomahawk;


void
WikipediaContext::setArtist( const Tomahawk::artist_ptr& artist )
{
    if ( artist.isNull() )
        return;
    if ( !m_artist.isNull() && m_artist->name() == artist->name() )
        return;

    m_artist = artist;

    QString lang = QLocale::system().name().split("_").first();
    webView()->load( QString( "http://%1.wikipedia.org/w/index.php?useformat=mobile&title=%2" ).arg( lang ).arg( m_artist->name() ) );
}


void
WikipediaContext::setAlbum( const Tomahawk::album_ptr& album )
{
    if ( album.isNull() )
        return;

    setArtist( album->artist() );
}


void
WikipediaContext::setQuery( const Tomahawk::query_ptr& query )
{
    if ( query.isNull() )
        return;

    setArtist( query->track()->artistPtr() );
}


void
LastfmContext::setArtist( const Tomahawk::artist_ptr& artist )
{
    if ( artist.isNull() )
        return;
    if ( !m_artist.isNull() && m_artist->name() == artist->name() )
        return;

    m_artist = artist;

    webView()->load( QString( "http://last.fm/music/%1" ).arg( m_artist->name() ) );
}


void
LastfmContext::setAlbum( const Tomahawk::album_ptr& album )
{
    if ( album.isNull() )
        return;

    setArtist( album->artist() );
}


void
LastfmContext::setQuery( const Tomahawk::query_ptr& query )
{
    if ( query.isNull() )
        return;

    setArtist( query->track()->artistPtr() );
}
