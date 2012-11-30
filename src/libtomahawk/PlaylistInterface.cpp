/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
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
#include "utils/Logger.h"
#include "Result.h"
#include "Pipeline.h"
#include "Source.h"

using namespace Tomahawk;


PlaylistInterface::PlaylistInterface ()
    : QObject()
    , m_latchMode( PlaylistModes::StayOnSong )
    , m_finished( false )
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


Tomahawk::result_ptr
PlaylistInterface::siblingResult( int itemsAway ) const
{
    qint64 idx = siblingIndex( itemsAway );
    qint64 safetyCheck = 0;

    // If safetyCheck equals idx, this means the interface keeps returning the same item and we won't discover anything new - abort
    // This can happen in repeat / random mode e.g.
    while ( idx >= 0 && safetyCheck != idx )
    {
        safetyCheck = idx;
        Tomahawk::query_ptr query = queryAt( idx );
        if ( query && query->numResults() )
        {
            return query->results().first();
        }

        if ( itemsAway < 0 )
            itemsAway--;
        else
            itemsAway++;

        idx = siblingIndex( itemsAway );
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
            query_ptr q = Query::get( q1->artist(), q1->track(), q1->album(), uuid(), false );
            q->setAlbumPos( q1->results().first()->albumpos() );
            q->setDiscNumber( q1->discnumber() );
            result << q;
        }
    }

    Pipeline::instance()->resolve( result );
    return result;
}


bool
PlaylistInterface::hasNextResult() const
{
    return ( siblingResult( 1 ) );
}


bool
PlaylistInterface::hasPreviousResult() const
{
    return ( siblingResult( -1 ) );
}
