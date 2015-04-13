/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2012, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2011, Leo Franchi <lfranchi@kde.org>
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

#include "PlaylistInterface.h"
#include "Result.h"
#include "Pipeline.h"
#include "Source.h"
#include "utils/Logger.h"

using namespace Tomahawk;


PlaylistInterface::PlaylistInterface()
    : QObject()
    , m_latchMode( PlaylistModes::StayOnSong )
    , m_prevAvail( false )
    , m_nextAvail( false )
    , m_currentIndex( -1 )
    , m_finished( false )
    , m_foundFirstTrack( false )
{
    m_id = uuid();
}


PlaylistInterface::~PlaylistInterface()
{
}


result_ptr
PlaylistInterface::previousResult() const
{
     return siblingResult( -1 );
}


result_ptr
PlaylistInterface::nextResult() const
{
     return siblingResult( 1 );
}


qint64
PlaylistInterface::siblingResultIndex( int itemsAway, qint64 rootIndex ) const
{
    qint64 idx = siblingIndex( itemsAway, rootIndex );
    QList< qint64 > safetyCheck;

    // If safetyCheck contains idx, this means the interface keeps returning the same items and we won't discover anything new - abort
    // This can happen in repeat / random mode e.g.
    while ( idx >= 0 && !safetyCheck.contains( idx ) )
    {
        safetyCheck << idx;
        Tomahawk::query_ptr query = queryAt( idx );

        if ( query && query->playable() )
        {
            return idx;
        }

        idx = siblingIndex( itemsAway < 0 ? -1 : 1, idx );
    }

    return -1;
}


Tomahawk::result_ptr
PlaylistInterface::siblingResult( int itemsAway, qint64 rootIndex ) const
{
    qint64 idx = siblingResultIndex( itemsAway, rootIndex );
    if ( idx >= 0 )
    {
        Tomahawk::query_ptr query = queryAt( idx );

        if ( query && query->playable() )
        {
            return query->results().first();
        }
    }

    return Tomahawk::result_ptr();
}


Tomahawk::result_ptr
PlaylistInterface::setSiblingResult( int itemsAway, qint64 rootIndex )
{
    qint64 idx = siblingResultIndex( itemsAway, rootIndex );
    if ( idx >= 0 )
    {
        Tomahawk::query_ptr query = queryAt( idx );

        if ( query && query->playable() )
        {
            setCurrentIndex( idx );
            return query->results().first();
        }
    }

    return Tomahawk::result_ptr();
}


int
PlaylistInterface::posOfResult( const Tomahawk::result_ptr& result ) const
{
    const QList< Tomahawk::query_ptr > queries = tracks();

    int res = 0;
    foreach ( const Tomahawk::query_ptr& query, queries )
    {
        if ( query && query->numResults() && query->results().contains( result ) )
            return res;

        res++;
    }

    return -1;
}


int
PlaylistInterface::posOfQuery( const Tomahawk::query_ptr& query ) const
{
    const QList< Tomahawk::query_ptr > queries = tracks();

    int res = 0;
    foreach ( const Tomahawk::query_ptr& q, queries )
    {
        if ( query == q )
            return res;

        res++;
    }

    return -1;
}


QList<Tomahawk::query_ptr>
PlaylistInterface::filterTracks( const QList<Tomahawk::query_ptr>& queries )
{
    QList<Tomahawk::query_ptr> result;

    for ( int i = 0; i < queries.count(); i++ )
    {
        bool picked = true;
        const query_ptr q1 = queries.at( i );

        for ( int j = 0; j < result.count(); j++ )
        {
            if ( !picked )
                break;

            const query_ptr& q2 = result.at( j );

            if ( q1->track() == q2->track() )
            {
                picked = false;
            }
        }

        if ( picked )
        {
            result << q1;
        }
    }

    Pipeline::instance()->resolve( result );
    return result;
}


bool
PlaylistInterface::hasNextResult() const
{
    Tomahawk::result_ptr r = siblingResult( 1 );
    return ( r && r->isOnline() );
}


bool
PlaylistInterface::hasPreviousResult() const
{
    Tomahawk::result_ptr r = siblingResult( -1 );
    return ( r && r->isOnline() );
}


void
PlaylistInterface::onItemsChanged()
{
    if ( QThread::currentThread() != thread() )
    {
        QMetaObject::invokeMethod( this, "onItemsChanged", Qt::QueuedConnection );
        return;
    }

    Tomahawk::result_ptr prevResult = siblingResult( -1, m_currentIndex );
    Tomahawk::result_ptr nextResult = siblingResult( 1, m_currentIndex );

    {
        bool avail = prevResult && prevResult->playable();
        if ( avail != m_prevAvail )
        {
            m_prevAvail = avail;
            emit previousTrackAvailable( avail );
        }
    }

    {
        bool avail = nextResult && nextResult->playable();
        if ( avail != m_nextAvail )
        {
            m_nextAvail = avail;
            emit nextTrackAvailable( avail );
        }
    }
}


void
PlaylistInterface::setCurrentIndex( qint64 index )
{
    m_currentIndex = index;

    emit currentIndexChanged();
    onItemsChanged();
}


void
PlaylistInterface::startLoading()
{
    foreach ( const Tomahawk::query_ptr& query, tracks() )
    {
        disconnect( query.data(), SIGNAL( playableStateChanged( bool ) ), this, SLOT( onItemsChanged() ) );
        disconnect( query.data(), SIGNAL( resolvingFinished( bool ) ), this, SLOT( onQueryResolved() ) );
    }

    m_finished = false;
}


void
PlaylistInterface::finishLoading()
{
    foreach ( const Tomahawk::query_ptr& query, tracks() )
    {
        connect( query.data(), SIGNAL( playableStateChanged( bool ) ), SLOT( onItemsChanged() ), Qt::UniqueConnection );
        connect( query.data(), SIGNAL( resolvingFinished( bool ) ), SLOT( onQueryResolved() ), Qt::UniqueConnection );
    }

    m_finished = true;
    emit finishedLoading();
}


void
PlaylistInterface::onQueryResolved()
{
    if ( m_foundFirstTrack )
        return;

    // We're looking for the first playable track, but want to make sure the
    // second track doesn't start playing before the first really finished resolving
    foreach ( const Tomahawk::query_ptr& query, tracks() )
    {
        if ( !query->resolvingFinished() )
        {
            // Wait for this track to be resolved
            return;
        }

        if ( query->playable() )
        {
            // We have a playable track, all previous tracks are resolved but not playable
            break;
        }
    }

    // We have either found the first playable track or none of the tracks are playable
    m_foundFirstTrack = true;
    emit foundFirstPlayableTrack();

    foreach ( const Tomahawk::query_ptr& query, tracks() )
    {
        disconnect( query.data(), SIGNAL( resolvingFinished( bool ) ), this, SLOT( onQueryResolved() ) );
    }
}
