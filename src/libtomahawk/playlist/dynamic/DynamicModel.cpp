/****************************************************************************************
 * Copyright (c) 2011 Leo Franchi <lfranchi@kde.org>                                    *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "playlist/dynamic/DynamicModel.h"
#include "GeneratorInterface.h"
#include "audio/audioengine.h"
#include <pipeline.h>

using namespace Tomahawk;

DynamicModel::DynamicModel( QObject* parent )
    : PlaylistModel( parent )
    , m_startOnResolved( false )
    , m_onDemandRunning( false )
    , m_changeOnNext( false )
    , m_searchingForNext( false )
    , m_filterUnresolvable( true )
    , m_currentAttempts( 0 )
    , m_lastResolvedRow( 0 )
{
    
}

DynamicModel::~DynamicModel()
{

}

void 
DynamicModel::loadPlaylist( const Tomahawk::dynplaylist_ptr& playlist )
{
    if( !m_playlist.isNull() ) {
        disconnect( m_playlist->generator().data(), SIGNAL( nextTrackGenerated( Tomahawk::query_ptr ) ), this, SLOT( newTrackGenerated( Tomahawk::query_ptr ) ) );
    }
    m_playlist = playlist;
    
    if( m_playlist->mode() == OnDemand )
        setFilterUnresolvable( true );
    
    connect( m_playlist->generator().data(), SIGNAL( nextTrackGenerated( Tomahawk::query_ptr ) ), this, SLOT( newTrackGenerated( Tomahawk::query_ptr ) ) );
    PlaylistModel::loadPlaylist( m_playlist, m_playlist->mode() == Static );
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
    m_startOnResolved = false; // not anymore---user clicks a track to start it
}

void 
DynamicModel::newTrackGenerated( const Tomahawk::query_ptr& query )
{
    if( m_onDemandRunning ) {
        connect( query.data(), SIGNAL( resolvingFinished( bool ) ), this, SLOT( trackResolveFinished( bool ) ) );
        connect( query.data(), SIGNAL( solvedStateChanged( bool ) ), this, SLOT( trackResolved( bool ) ) );
    
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
DynamicModel::trackResolved( bool resolved )
{   
    if( !resolved )
        return;
    
    Query* q = qobject_cast<Query*>(sender());
    qDebug() << "Got successful resolved track:" << q->track() << q->artist() << m_lastResolvedRow << m_currentAttempts;
    if( m_startOnResolved ) { // on first start
        m_startOnResolved = false;
        AudioEngine::instance()->play();
    }
    
    if( m_currentAttempts > 0 ) {
        qDebug() << "EMITTING AN ASK FOR COLLAPSE:" << m_lastResolvedRow << m_currentAttempts;
        emit collapseFromTo( m_lastResolvedRow, m_currentAttempts );
    }
    m_currentAttempts = 0;
    m_searchingForNext = false;

    emit checkForOverflow();
}

void 
DynamicModel::trackResolveFinished( bool success )
{
    if( !success ) { // if it was successful, we've already gotten a trackResolved() signal
        Query* q = qobject_cast<Query*>(sender());
        qDebug() << "Got not resolved track:" << q->track() << q->artist() << m_lastResolvedRow << m_currentAttempts;
        m_currentAttempts++;
        if( m_currentAttempts < 20 ) {
            qDebug() << "FETCHING MORE!";
            m_playlist->generator()->fetchNext();
        } else {
            emit trackGenerationFailure( tr( "Could not find a playable track.\n\nPlease change the filters or try again." ) );
        }
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
    if( m_filterUnresolvable ) { // wait till we get them resolved
        m_limitResolvedTo = limitResolvedTo;
        filterUnresolved( entries );
    } else {
        addToPlaylist( entries, m_playlist->mode() == OnDemand ); // if ondemand, we're previewing, so clear old
    }
}

void 
DynamicModel::filterUnresolved( const QList< query_ptr >& entries )
{
    m_toResolveList = entries;
    
    foreach( const query_ptr& q, entries ) {
        connect( q.data(), SIGNAL( resolvingFinished( bool ) ), this, SLOT( filteringTrackResolved( bool ) ) );
        Pipeline::instance()->resolve( q );
    }    
}

void 
DynamicModel::filteringTrackResolved( bool successful )
{    
    // arg, we don't have the query_ptr, just the Query
    Query* q = qobject_cast< Query* >( sender() );
    Q_ASSERT( q );
    
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
    
    if( successful )
        m_resolvedList << realptr;
    
    if( m_toResolveList.isEmpty() || m_resolvedList.size() == m_limitResolvedTo ) { // done, add to playlist
        if( m_limitResolvedTo < m_resolvedList.count() ) // limit to how many we were asked for
            m_resolvedList = m_resolvedList.mid( 0, m_limitResolvedTo );
        
        addToPlaylist( m_resolvedList, true );
        m_toResolveList.clear();
        m_resolvedList.clear();
    }       
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
