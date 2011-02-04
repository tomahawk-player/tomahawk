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
#include "StationModelItem.h"
#include "audio/audioengine.h"

using namespace Tomahawk;

DynamicModel::DynamicModel( QObject* parent )
    : PlaylistModel( parent )
    , m_startOnResolved( false )
    , m_onDemandRunning( false )
    , m_currentAttempts( 0 )
{

}

DynamicModel::~DynamicModel()
{

}

void 
DynamicModel::loadPlaylist( const Tomahawk::dynplaylist_ptr& playlist )
{
    m_playlist = playlist;
    
    
    connect( m_playlist->generator().data(), SIGNAL( nextTrackGenerated( Tomahawk::query_ptr ) ), this, SLOT( newTrackGenerated( Tomahawk::query_ptr ) ) );
    PlaylistModel::loadPlaylist( m_playlist );
}

void 
DynamicModel::startOnDemand()
{
    m_playlist->generator()->startOnDemand();
    
    m_onDemandRunning = true;
    m_startOnResolved = true;
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
}


void 
DynamicModel::trackResolved()
{   
    m_currentAttempts = 0;
    
    if( m_startOnResolved ) { // on first start
        m_startOnResolved = false;
        AudioEngine::instance()->play();
    }
}

void 
DynamicModel::trackResolveFinished( bool success )
{
    if( !success ) { // if it was successful, we've already gotten a trackResolved() signal
        m_currentAttempts++;
        if( m_currentAttempts < 100 ) {
            m_playlist->generator()->fetchNext();
        }
    }
}


void 
DynamicModel::newTrackLoading()
{
    if( m_onDemandRunning && m_currentAttempts == 0 ) { // if we're in dynamic mode and we're also currently idle
        m_playlist->generator()->fetchNext();
    }
}
