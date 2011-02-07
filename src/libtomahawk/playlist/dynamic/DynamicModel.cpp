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

using namespace Tomahawk;

DynamicModel::DynamicModel( QObject* parent )
    : PlaylistModel( parent )
    , m_startOnResolved( false )
    , m_onDemandRunning( false )
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
    
    
    connect( m_playlist->generator().data(), SIGNAL( nextTrackGenerated( Tomahawk::query_ptr ) ), this, SLOT( newTrackGenerated( Tomahawk::query_ptr ) ) );
    PlaylistModel::loadPlaylist( m_playlist, !m_onDemandRunning );
}

void 
DynamicModel::startOnDemand()
{
    connect( AudioEngine::instance(), SIGNAL( loading( Tomahawk::result_ptr ) ), this, SLOT( newTrackLoading() ) );
    
    // delete all the tracks
    clear();
    
    m_playlist->generator()->startOnDemand();
    
    m_onDemandRunning = true;
    m_startOnResolved = true;
    m_currentAttempts = 0;
    m_lastResolvedRow = 0;
}

void 
DynamicModel::newTrackGenerated( const Tomahawk::query_ptr& query )
{
    if( m_onDemandRunning ) {
        connect( query.data(), SIGNAL( resolvingFinished( bool ) ), this, SLOT( trackResolveFinished( bool ) ) );
        connect( query.data(), SIGNAL( resultsAdded( QList<Tomahawk::result_ptr> ) ), this, SLOT( trackResolved() ) );
    
        append( query );
    }
}

void 
DynamicModel::stopOnDemand()
{
    m_onDemandRunning = false;
    AudioEngine::instance()->stop();
    
    disconnect( AudioEngine::instance(), SIGNAL( loading( Tomahawk::result_ptr ) ), this, SLOT( newTrackLoading() ) );
}


void 
DynamicModel::trackResolved()
{   
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
}

void 
DynamicModel::trackResolveFinished( bool success )
{
    if( !success ) { // if it was successful, we've already gotten a trackResolved() signal
        Query* q = qobject_cast<Query*>(sender());
        qDebug() << "Got not resolved track:" << q->track() << q->artist() << m_lastResolvedRow << m_currentAttempts;
        m_currentAttempts++;
        if( m_currentAttempts < 20 ) {
            m_playlist->generator()->fetchNext();
        } else {
            emit trackGenerationFailure( tr( "Could not find a playable track.\n\nPlease change the filters or try again." ) );
        }
    }
}


void 
DynamicModel::newTrackLoading()
{
    if( m_onDemandRunning && m_currentAttempts == 0 ) { // if we're in dynamic mode and we're also currently idle
        m_lastResolvedRow = rowCount( QModelIndex() );
        m_playlist->generator()->fetchNext();
    }
}

void 
DynamicModel::removeIndex(const QModelIndex& index, bool moreToCome)
{
    if ( m_playlist->mode() == Static && isReadOnly() )
        return;
    
    if( m_playlist->mode() == OnDemand )
        TrackModel::removeIndex( index );
    else
        PlaylistModel::removeIndex( index, moreToCome );
    // don't call onPlaylistChanged.
        
    if( !moreToCome )
        m_lastResolvedRow = rowCount( QModelIndex() );
}
