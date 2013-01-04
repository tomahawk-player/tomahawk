/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 * 
 *   Copyright 2012 Leo Franchi <lfranchi@kde.org>
 *   Copyright 2012, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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


#include "SingleTrackPlaylistInterface.h"

namespace Tomahawk
{

SingleTrackPlaylistInterface::SingleTrackPlaylistInterface( const Tomahawk::query_ptr& query )
    : PlaylistInterface()
    , m_track( query )
{
}


Tomahawk::result_ptr
SingleTrackPlaylistInterface::currentItem() const
{
    if ( m_track && m_track->numResults() )
        return m_track->results().first();

    return result_ptr();
}


Tomahawk::result_ptr
SingleTrackPlaylistInterface::resultAt( qint64 index ) const
{
    if ( index == 0 && m_track && m_track->numResults() )
        return m_track->results().first();

    return result_ptr();
}


Tomahawk::query_ptr
SingleTrackPlaylistInterface::queryAt( qint64 index ) const
{
    if ( index == 0 )
        return m_track;

    return query_ptr();
}


qint64
SingleTrackPlaylistInterface::indexOfResult( const Tomahawk::result_ptr& result ) const
{
    if ( m_track && m_track->results().contains( result ) )
        return 0;

    return -1;
}


qint64
SingleTrackPlaylistInterface::indexOfQuery( const Tomahawk::query_ptr& query ) const
{
    if ( m_track == query )
        return 0;
    
    return -1;
}


QList< Tomahawk::query_ptr >
SingleTrackPlaylistInterface::tracks() const
{
    QList< query_ptr > ql;

    if ( m_track )
        ql << m_track;

    return ql;
}

}

