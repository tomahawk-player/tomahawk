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

using namespace Tomahawk;


void
WikipediaContext::setQuery( const Tomahawk::query_ptr& query )
{
    if ( !m_query.isNull() && query->artist() == m_query->artist() )
        return;

    m_query = query;
    webView()->load( QString( "http://en.wikipedia.org/w/index.php?printable=yes&title=%1" ).arg( query->artist() ) );
}


void
LastfmContext::setQuery( const Tomahawk::query_ptr& query )
{
    if ( !m_query.isNull() && query->artist() == m_query->artist() )
        return;

    m_query = query;
    webView()->load( QString( "http://last.fm/music/%1" ).arg( query->artist() ) );
}
