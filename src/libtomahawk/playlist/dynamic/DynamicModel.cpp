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

#include "playlist/dynamic/DynamicModel.h"
#include "GeneratorInterface.h"
#include "audio/audioengine.h"
#include <pipeline.h>

using namespace Tomahawk;

DynamicModel::DynamicModel( QObject* parent )
    : PlaylistModel( parent )
    , m_onDemandRunning( false )
    , m_changeOnNext( false )
    , m_searchingForNext( false )
    , m_filterUnresolvable( true )
    , m_startingAfterFailed( false )
    , m_currentAttempts( 0 )
    , m_lastResolvedRow( 0 )
{

}

DynamicModel::~DynamicModel()
{

}

void
DynamicModel::loadPlaylist( const Tomahawk::dynplaylist_ptr& playlist, bool loadEntries )
{
    Q_UNUSED( loadEntries );

    if( !m_playlist.isNull() ) {
        disconnect( m_playlist->generator().data(), SIGNAL( nextTrackGenerated( Tomahawk::query_ptr ) ), this, SLOT( newTrackGenerated( Tomahawk::query_ptr ) ) );
    }
    m_playlist = playlist;

    if( m_playlist->mode() == OnDemand )
        setFilterUnresolvable( true );

    connect( m_playlist->generator().data(), SIGNAL( nextTrackGenerated( Tomahawk::query_ptr ) ), this, SLOT( newTrackGenerated( Tomahawk::query_ptr ) ) );
    PlaylistModel::loadPlaylist( m_playlist, m_playlist->mode() == Static );

    if( m_playlist->mode() == OnDemand )
        emit trackCountChanged( rowCount( QModelIndex() ) );
}

QString
DynamicModel::description() const
{
    return m_playlist->generator()->sentenceSummary();
}


void
DynamicModel::startOnDemand()
{
    connect( AudioEngine::instance(), SIGNAL( loading( Tomahawk::result_ptr ) ), this, SLOT( newTrackLoading() ) );

    m_playlist->generator()->startOnDemand();

    m_onDemandRunning = true;
}

void
DynamicModel::newTrackGenerated( const Tomahawk::query_ptr& query )
{
    if( m_onDemandRunning ) {
        connect( query.data(), SIGNAL( resolvingFinished( bool ) ), this, SLOT( trackResolveFinished( bool ) ) );

        append( query );
    }
}

void
DynamicModel::stopOnDemand( bool stopPlaying )
{
    m_onDemandRunning = false;
    if( stopPlaying )
        AudioEngine::instance()->stop();

    disconnect( AudioEngine::instance(), SIGNAL( loading( Tomahawk::result_ptr ) ), this, SLOT( newTrackLoading() ) );
}

void
DynamicModel::changeStation()
{
    if( m_onDemandRunning )
        m_changeOnNext = true;
    else // if we're not running, just start
        m_playlist->generator()->startOnDemand();
}

void
DynamicModel::trackResolveFinished( bool success )
{
    Q_UNUSED( success );

    Query* q = qobject_cast<Query*>(sender());

    if( !q->playable() ) {
        qDebug() << "Got not resolved track:" << q->track() << q->artist() << m_lastResolvedRow << m_currentAttempts;
        m_currentAttempts++;

        int curAttempts = m_startingAfterFailed ? m_currentAttempts - 20 : m_currentAttempts; // if we just failed, m_currentAttempts includes those failures
        if( curAttempts < 20 ) {
            qDebug() << "FETCHING MORE!";
            m_playlist->generator()->fetchNext();
        } else {
            m_startingAfterFailed = true;
            emit trackGenerationFailure( tr( "Could not find a playable track.\n\nPlease change the filters or try again." ) );
        }
    }
    else {
        qDebug() << "Got successful resolved track:" << q->track() << q->artist() << m_lastResolvedRow << m_currentAttempts;

        if( m_currentAttempts > 0 ) {
            qDebug() << "EMITTING AN ASK FOR COLLAPSE:" << m_lastResolvedRow << m_currentAttempts;
            emit collapseFromTo( m_lastResolvedRow, m_currentAttempts );
        }
        m_currentAttempts = 0;
        m_searchingForNext = false;

        emit checkForOverflow();
    }
}


void
DynamicModel::newTrackLoading()
{
    qDebug() << "Got NEW TRACK LOADING signal";
    if( m_changeOnNext ) { // reset instead of getting the next one
        m_lastResolvedRow = rowCount( QModelIndex() );
        m_searchingForNext = true;
        m_playlist->generator()->startOnDemand();
    } else if( m_onDemandRunning && m_currentAttempts == 0 && !m_searchingForNext ) { // if we're in dynamic mode and we're also currently idle
        m_lastResolvedRow = rowCount( QModelIndex() );
        m_searchingForNext = true;
        qDebug() << "IDLE fetching new track!";
        m_playlist->generator()->fetchNext();
    }
}

void
DynamicModel::tracksGenerated( const QList< query_ptr > entries, int limitResolvedTo )
{
    if( m_filterUnresolvable && m_playlist->mode() == OnDemand ) { // wait till we get them resolved (for previewing stations)
        m_limitResolvedTo = limitResolvedTo;
        filterUnresolved( entries );
    } else {
        addToPlaylist( entries, m_playlist->mode() == OnDemand ); // if ondemand, we're previewing, so clear old

        if( m_playlist->mode() == OnDemand ) {
            m_lastResolvedRow = rowCount( QModelIndex() );
        }
    }
}

void
DynamicModel::filterUnresolved( const QList< query_ptr >& entries )
{
    m_toResolveList = entries;

    foreach( const query_ptr& q, entries ) {
        connect( q.data(), SIGNAL( resolvingFinished( bool ) ), this, SLOT( filteringTrackResolved( bool ) ) );
    }
    Pipeline::instance()->resolve( entries, true );
}

void
DynamicModel::filteringTrackResolved( bool successful )
{
    // arg, we don't have the query_ptr, just the Query
    Query* q = qobject_cast< Query* >( sender() );
    Q_ASSERT( q );

    // if meantime the user began the station, abort
    if( m_onDemandRunning ) {
        m_toResolveList.clear();
        m_resolvedList.clear();

        return;
    }

    query_ptr realptr;
    foreach( const query_ptr& qptr, m_toResolveList ) {
        if( qptr.data() == q ) {
            realptr = qptr;
            break;
        }
    }
    if( realptr.isNull() ) // we already finished
        return;

    m_toResolveList.removeAll( realptr );

    if( successful ) {
        m_resolvedList << realptr;

        // append and update internal lastResolvedRow
        addToPlaylist( QList< query_ptr >() << realptr, false );
        if( m_playlist->mode() == OnDemand ) {
            m_lastResolvedRow = rowCount( QModelIndex() );
        }

        if( m_toResolveList.isEmpty() || m_resolvedList.size() == m_limitResolvedTo ) { // done
            m_toResolveList.clear();
            m_resolvedList.clear();

        }
    }

    if( m_toResolveList.isEmpty() && rowCount( QModelIndex() ) == 0 ) // we failed
        emit trackGenerationFailure( tr( "Could not find a playable track.\n\nPlease change the filters or try again." ) );
}


void
DynamicModel::addToPlaylist( const QList< query_ptr >& entries, bool clearFirst )
{
    if( clearFirst )
        clear();

    if( m_playlist->author()->isLocal() && m_playlist->mode() == Static ) {
        m_playlist->addEntries( entries, m_playlist->currentrevision() );
    } else { // read-only, so add tracks only in the GUI, not to the playlist itself
        foreach( const query_ptr& query, entries ) {
            append( query );
        }
    }

    emit tracksAdded();
}


void
DynamicModel::removeIndex(const QModelIndex& idx, bool moreToCome)
{
    if ( m_playlist->mode() == Static && isReadOnly() )
        return;

    if( m_playlist->mode() == OnDemand ) {
        if( !moreToCome && idx == index( rowCount( QModelIndex() ) - 1, 0, QModelIndex() ) ) { // if the user is manually removing the last one, re-add as we're a station
            newTrackLoading();
        }
        TrackModel::removeIndex( idx );
    } else
        PlaylistModel::removeIndex( idx, moreToCome );
    // don't call onPlaylistChanged.

    if( !moreToCome )
        m_lastResolvedRow = rowCount( QModelIndex() );
}
