/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
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

#include "MetaPlaylistInterface.h"
#include "Result.h"
#include "Pipeline.h"
#include "Source.h"
#include "utils/Logger.h"

using namespace Tomahawk;


MetaPlaylistInterface::MetaPlaylistInterface()
    : Tomahawk::PlaylistInterface()
{
}


MetaPlaylistInterface::~MetaPlaylistInterface()
{
}


void
MetaPlaylistInterface::addChildInterface( const Tomahawk::playlistinterface_ptr& interface )
{
    m_childInterfaces << interface;

    if ( m_childInterfaces.count() == 1 )
    {
        connect( interface.data(), SIGNAL( repeatModeChanged( Tomahawk::PlaylistModes::RepeatMode ) ),
                                   SIGNAL( repeatModeChanged( Tomahawk::PlaylistModes::RepeatMode ) ) );

        connect( interface.data(), SIGNAL( shuffleModeChanged( bool ) ),
                                   SIGNAL( shuffleModeChanged( bool ) ) );
    }
}


QList< Tomahawk::query_ptr >
MetaPlaylistInterface::tracks() const
{
    if ( m_childInterfaces.count() )
        return m_childInterfaces.first()->tracks();
    else
        return QList< Tomahawk::query_ptr >();
}


int
MetaPlaylistInterface::trackCount() const
{
    if ( m_childInterfaces.count() )
        return m_childInterfaces.first()->trackCount();
    else
        return 0;
}


result_ptr
MetaPlaylistInterface::currentItem() const
{
    if ( m_childInterfaces.count() )
        return m_childInterfaces.first()->currentItem();
    else
        return Tomahawk::result_ptr();
}


qint64
MetaPlaylistInterface::siblingIndex( int itemsAway, qint64 rootIndex ) const
{
    if ( m_childInterfaces.count() )
        return m_childInterfaces.first()->siblingIndex( itemsAway, rootIndex );
    else
        return -1;
}


Tomahawk::query_ptr
MetaPlaylistInterface::queryAt( qint64 index ) const
{
    if ( m_childInterfaces.count() )
        return m_childInterfaces.first()->queryAt( index );
    else
        return Tomahawk::query_ptr();
}


Tomahawk::result_ptr
MetaPlaylistInterface::resultAt( qint64 index ) const
{
    if ( m_childInterfaces.count() )
        return m_childInterfaces.first()->resultAt( index );
    else
        return Tomahawk::result_ptr();
}


qint64
MetaPlaylistInterface::indexOfResult( const Tomahawk::result_ptr& result ) const
{
    if ( m_childInterfaces.count() )
        return m_childInterfaces.first()->indexOfResult( result );
    else
        return -1;
}


qint64
MetaPlaylistInterface::indexOfQuery( const Tomahawk::query_ptr& query ) const
{
    if ( m_childInterfaces.count() )
        return m_childInterfaces.first()->indexOfQuery( query );
    else
        return -1;
}


PlaylistModes::RepeatMode
MetaPlaylistInterface::repeatMode() const
{
    if ( m_childInterfaces.count() )
        return m_childInterfaces.first()->repeatMode();
    else
        return PlaylistModes::NoRepeat;
}


bool
MetaPlaylistInterface::shuffled() const
{
    if ( m_childInterfaces.count() )
        return m_childInterfaces.first()->shuffled();
    else
        return false;
}


bool
MetaPlaylistInterface::hasChildInterface( const Tomahawk::playlistinterface_ptr& interface )
{
    foreach ( const Tomahawk::playlistinterface_ptr& iface, m_childInterfaces )
    {
        if ( iface == interface || iface->hasChildInterface( interface ) )
            return true;
    }

    return false;
}


void
MetaPlaylistInterface::setRepeatMode( PlaylistModes::RepeatMode mode )
{
    if ( m_childInterfaces.count() )
        return m_childInterfaces.first()->setRepeatMode( mode );
}


void
MetaPlaylistInterface::setShuffled( bool enabled )
{
    if ( m_childInterfaces.count() )
        return m_childInterfaces.first()->setShuffled( enabled );
}
