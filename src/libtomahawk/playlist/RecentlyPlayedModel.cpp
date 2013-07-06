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

#include "RecentlyPlayedModel.h"

#include <QMimeData>
#include <QTreeView>

#include "Source.h"
#include "SourceList.h"
#include "database/Database.h"
#include "database/DatabaseCommand_PlaybackHistory.h"
#include "PlayableItem.h"
#include "utils/TomahawkUtils.h"
#include "utils/Logger.h"

#define HISTORY_TRACK_ITEMS 25

using namespace Tomahawk;


RecentlyPlayedModel::RecentlyPlayedModel( QObject* parent, unsigned int maxItems )
    : PlaylistModel( parent )
    , m_limit( maxItems > 0 ? maxItems : HISTORY_TRACK_ITEMS )
{
}


RecentlyPlayedModel::~RecentlyPlayedModel()
{
}


void
RecentlyPlayedModel::loadHistory()
{
    if ( rowCount( QModelIndex() ) )
    {
        clear();
    }
    startLoading();

    DatabaseCommand_PlaybackHistory* cmd = new DatabaseCommand_PlaybackHistory( m_source );
    cmd->setDateFrom( m_dateFrom );
    cmd->setDateTo( m_dateTo );
    cmd->setLimit( m_limit );

    connect( cmd, SIGNAL( tracks( QList<Tomahawk::track_ptr>, QList<Tomahawk::PlaybackLog> ) ),
                    SLOT( appendTracks( QList<Tomahawk::track_ptr>, QList<Tomahawk::PlaybackLog> ) ), Qt::QueuedConnection );

    Database::instance()->enqueue( Tomahawk::dbcmd_ptr( cmd ) );
}


void
RecentlyPlayedModel::onSourcesReady()
{
    Q_ASSERT( m_source.isNull() );

    loadHistory();

    foreach ( const source_ptr& source, SourceList::instance()->sources() )
        onSourceAdded( source );
}


void
RecentlyPlayedModel::setSource( const Tomahawk::source_ptr& source )
{
    m_source = source;
    if ( source.isNull() )
    {
        if ( SourceList::instance()->isReady() )
            onSourcesReady();
        else
            connect( SourceList::instance(), SIGNAL( ready() ), SLOT( onSourcesReady() ) );

        connect( SourceList::instance(), SIGNAL( sourceAdded( Tomahawk::source_ptr ) ), SLOT( onSourceAdded( Tomahawk::source_ptr ) ) );
    }
    else
    {
        onSourceAdded( source );
        loadHistory();
    }
}


void
RecentlyPlayedModel::onSourceAdded( const Tomahawk::source_ptr& source )
{
    connect( source.data(), SIGNAL( playbackFinished( Tomahawk::track_ptr, Tomahawk::PlaybackLog ) ),
                              SLOT( onPlaybackFinished( Tomahawk::track_ptr, Tomahawk::PlaybackLog ) ), Qt::UniqueConnection );
}


void
RecentlyPlayedModel::onPlaybackFinished( const Tomahawk::track_ptr& track, const Tomahawk::PlaybackLog& log )
{
    int count = trackCount();

    if ( count )
    {
        PlayableItem* oldestItem = itemFromIndex( index( count - 1, 0, QModelIndex() ) );
        if ( oldestItem->playbackLog().timestamp >= log.timestamp )
            return;

        PlayableItem* youngestItem = itemFromIndex( index( 0, 0, QModelIndex() ) );
        if ( youngestItem->playbackLog().timestamp <= log.timestamp )
            insertQuery( track->toQuery(), 0, log );
        else
        {
            for ( int i = 0; i < count - 1; i++ )
            {
                PlayableItem* item1 = itemFromIndex( index( i, 0, QModelIndex() ) );
                PlayableItem* item2 = itemFromIndex( index( i + 1, 0, QModelIndex() ) );

                if ( item1->playbackLog().timestamp >= log.timestamp && item2->playbackLog().timestamp <= log.timestamp )
                {
                    insertQuery( track->toQuery(), i + 1, log );
                    break;
                }
            }
        }
    }
    else
        insertQuery( track->toQuery(), 0, log );

    if ( trackCount() > (int)m_limit )
        remove( m_limit );

    ensureResolved();
}


bool
RecentlyPlayedModel::isTemporary() const
{
    return true;
}


void
RecentlyPlayedModel::setDateFrom( const QDate& date )
{
    m_dateFrom = date;
}


void
RecentlyPlayedModel::setDateTo( const QDate& date )
{
    m_dateTo = date;
    loadHistory();
}
