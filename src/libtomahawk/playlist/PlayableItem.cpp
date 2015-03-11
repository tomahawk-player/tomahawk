/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2012, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "PlayableItem.h"

#include "utils/Logger.h"
#include "utils/TomahawkUtils.h"

#include "Artist.h"
#include "Album.h"
#include "PlaylistEntry.h"
#include "Query.h"
#include "Result.h"
#include "Source.h"

using namespace Tomahawk;


PlayableItem::~PlayableItem()
{
    // Don't use qDeleteAll here! The children will remove themselves
    // from the list when they get deleted and the qDeleteAll iterator
    // will fail badly!
    for ( int i = children.count() - 1; i >= 0; i-- )
        delete children.at( i );

    if ( m_parent && index.isValid() )
    {
        m_parent->children.removeAt( index.row() );
    }
}


PlayableItem::PlayableItem( PlayableItem* parent )
    : QObject( parent )
    , m_parent( parent )
{
    init();
}


PlayableItem::PlayableItem( const Tomahawk::album_ptr& album, PlayableItem* parent, int row )
    : QObject( parent )
    , m_album( album )
    , m_parent( parent )
{
    init( row );

    connect( album.data(), SIGNAL( updated() ), SIGNAL( dataChanged() ) );
}


PlayableItem::PlayableItem( const Tomahawk::artist_ptr& artist, PlayableItem* parent, int row )
    : QObject( parent )
    , m_artist( artist )
    , m_parent( parent )
{
    init( row );

    connect( artist.data(), SIGNAL( updated() ), SIGNAL( dataChanged() ) );
}


PlayableItem::PlayableItem( const Tomahawk::result_ptr& result, PlayableItem* parent, int row )
    : QObject( parent )
    , m_result( result )
    , m_parent( parent )
{
    init( row );

    connect( result.data(), SIGNAL( updated() ), SIGNAL( dataChanged() ) );
}


PlayableItem::PlayableItem( const Tomahawk::query_ptr& query, PlayableItem* parent, int row )
    : QObject( parent )
    , m_query( query )
    , m_parent( parent )
{
    init( row );
}


PlayableItem::PlayableItem( const Tomahawk::plentry_ptr& entry, PlayableItem* parent, int row )
    : QObject( parent )
    , m_entry( entry )
    , m_query( entry->query() )
    , m_parent( parent )
{
    init( row );
}


PlayableItem::PlayableItem( const Tomahawk::source_ptr& source, PlayableItem* parent, int row )
    : QObject( parent )
    , m_source( source )
    , m_parent( parent )
{
    init( row );
}


void
PlayableItem::init( int row )
{
    track_ptr track;
    if ( m_query )
    {
        connect( m_query.data(), SIGNAL( resultsChanged() ), SLOT( onResultsChanged() ) );
        track = m_query->track();
    }
    else if ( m_result )
    {
        track = m_result->track();
    }

    if ( track )
    {
        connect( track.data(), SIGNAL( socialActionsLoaded() ), SIGNAL( dataChanged() ) );
        connect( track.data(), SIGNAL( attributesLoaded() ), SIGNAL( dataChanged() ) );
        connect( track.data(), SIGNAL( updated() ), SIGNAL( dataChanged() ) );
    }

    if ( m_parent )
    {
        if ( row < 0 )
        {
            m_parent->children.append( this );
        }
        else
        {
            m_parent->children.insert( row, this );
        }
    }

    if ( m_query )
    {
        onResultsChanged();
    }
}


void
PlayableItem::onResultsChanged()
{
    if ( m_query && !m_query->results().isEmpty() )
    {
        m_result = m_query->results().first();
        connect( m_result.data(), SIGNAL( updated() ), SIGNAL( dataChanged() ), Qt::UniqueConnection );
    }
    else
        m_result = result_ptr();

    emit dataChanged();
}


QString
PlayableItem::name() const
{
    if ( m_artist )
    {
        return m_artist->name();
    }
    else if ( m_album )
    {
        return m_album->name();
    }
    else if ( m_result )
    {
        return m_result->track()->track();
    }
    else if ( m_query )
    {
        return m_query->track()->track();
    }

    Q_ASSERT( false );
    return QString();
}


QString
PlayableItem::artistName() const
{
    if ( m_result )
    {
        return m_result->track()->artist();
    }
    else if ( m_query )
    {
        return m_query->track()->artist();
    }

    return QString();
}


QString
PlayableItem::albumName() const
{
    if ( m_result )
    {
        return m_result->track()->album();
    }
    else if ( m_query )
    {
        return m_query->track()->album();
    }

    return QString();
}


const Tomahawk::result_ptr&
PlayableItem::result() const
{
    if ( !m_result && m_query )
    {
        if ( m_query->numResults() )
            return m_query->results().first();
    }

    return m_result;
}


Tomahawk::PlaybackLog
PlayableItem::playbackLog() const
{
    return m_playbackLog;
}


void
PlayableItem::setPlaybackLog( const Tomahawk::PlaybackLog& log )
{
    m_playbackLog = log;
}
