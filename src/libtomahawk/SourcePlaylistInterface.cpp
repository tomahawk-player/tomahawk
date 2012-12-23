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

#include "SourcePlaylistInterface.h"

#include "Source.h"
#include "Pipeline.h"
#include "audio/AudioEngine.h"

#include "utils/Logger.h"

using namespace Tomahawk;


SourcePlaylistInterface::SourcePlaylistInterface( Tomahawk::Source* source, Tomahawk::PlaylistModes::LatchMode latchMode )
    : PlaylistInterface()
    , m_source( source )
    , m_currentItem( 0 )
    , m_gotNextItem( false )
{
    setLatchMode( latchMode );

    if ( !m_source.isNull() )
        connect( m_source.data(), SIGNAL( playbackStarted( const Tomahawk::query_ptr& ) ), SLOT( onSourcePlaybackStarted( const Tomahawk::query_ptr& ) ) );

    if ( AudioEngine::instance() )
        connect( AudioEngine::instance(), SIGNAL( paused() ), SLOT( audioPaused() ) );
}


SourcePlaylistInterface::~SourcePlaylistInterface()
{
    m_source.clear();
}


void
SourcePlaylistInterface::setCurrentIndex( qint64 index )
{
    if ( index == 1 )
        m_gotNextItem = false;
}


qint64
SourcePlaylistInterface::indexOfResult( const Tomahawk::result_ptr& result ) const
{
    if ( nextResult() == result )
        return 1;
    else
        return -1;
}


qint64
SourcePlaylistInterface::siblingIndex( int itemsAway, qint64 rootIndex ) const
{
    Q_UNUSED( itemsAway );
    Q_UNUSED( rootIndex );

    if ( nextResult() )
        return 1;
    else
        return -1;
}


Tomahawk::result_ptr
SourcePlaylistInterface::nextResult() const
{
    if ( !sourceValid() )
    {
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Source no longer valid";
        m_currentItem = Tomahawk::result_ptr();
        return m_currentItem;
    }
    else if ( !hasNextResult() )
    {
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "This song was already fetched or the source isn't playing anything";
        return Tomahawk::result_ptr();
    }

    if ( m_source.data()->currentTrack()->numResults() )
    {
        m_currentItem = m_source.data()->currentTrack()->results().first();
    }
    else
        m_currentItem = result_ptr();

    return m_currentItem;
}


result_ptr
SourcePlaylistInterface::currentItem() const
{
    return m_currentItem;
}


bool
SourcePlaylistInterface::sourceValid() const
{
    tDebug( LOGEXTRA ) << Q_FUNC_INFO;
    if ( m_source.isNull() || m_source.data()->currentTrack().isNull() )
        return false;

    return true;
}


bool
SourcePlaylistInterface::hasNextResult() const
{
    if ( !sourceValid() )
        return false;

    return m_gotNextItem;
}


QList<Tomahawk::query_ptr>
SourcePlaylistInterface::tracks() const
{
    QList<Tomahawk::query_ptr> tracks;
    if ( nextResult() )
        tracks << nextResult()->toQuery();

    return tracks;
}


QWeakPointer< Tomahawk::Source >
SourcePlaylistInterface::source() const
{
    return m_source;
}


void
SourcePlaylistInterface::reset()
{
    if ( m_currentItem.isNull() )
        m_gotNextItem = false;
    else
        m_gotNextItem = true;
}


void
SourcePlaylistInterface::onSourcePlaybackStarted( const Tomahawk::query_ptr& query )
{
    tDebug( LOGEXTRA ) << Q_FUNC_INFO;
    connect( query.data(), SIGNAL( resolvingFinished( bool ) ), SLOT( resolvingFinished( bool ) ) );
    Pipeline::instance()->resolve( query );
    m_gotNextItem = false;
}


void
SourcePlaylistInterface::resolvingFinished( bool hasResults )
{
    tDebug( LOGEXTRA ) << Q_FUNC_INFO << "Has results?" << ( hasResults ? "true" : "false" );
    if ( hasResults )
    {
        m_gotNextItem = true;
    }

    emit nextTrackAvailable( hasResults );
}


Tomahawk::query_ptr
SourcePlaylistInterface::queryAt( qint64 index ) const
{
    if ( index == 1 )
    {
        Tomahawk::result_ptr res = nextResult();
        return res->toQuery();
    }
    else
        return Tomahawk::query_ptr();
}


Tomahawk::result_ptr
SourcePlaylistInterface::resultAt( qint64 index ) const
{
    if ( index == 1 )
        return nextResult();
    else
        return Tomahawk::result_ptr();
}
