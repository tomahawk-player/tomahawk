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

#include "QueueProxyModel.h"

#include "audio/AudioEngine.h"
#include "playlist/TrackView.h"
#include "PlayableItem.h"
#include "ViewManager.h"
#include "Source.h"
#include "utils/Logger.h"

using namespace Tomahawk;


QueueProxyModel::QueueProxyModel( TrackView* parent )
    : PlayableProxyModel( parent )
{
    connect( this, SIGNAL( indexPlayable( QModelIndex ) ), SLOT( onIndexChanged( QModelIndex ) ) );
    connect( this, SIGNAL( indexResolved( QModelIndex ) ), SLOT( onIndexChanged( QModelIndex ) ) );
    connect( parent, SIGNAL( itemActivated( QModelIndex ) ), SLOT( onIndexActivated( QModelIndex ) ) );
    connect( AudioEngine::instance(), SIGNAL( loading( Tomahawk::result_ptr ) ), SLOT( onPlaybackStarted( Tomahawk::result_ptr ) ) );
}


QueueProxyModel::~QueueProxyModel()
{
}


void
QueueProxyModel::onIndexChanged( const QModelIndex& index )
{
    PlayableItem* item = itemFromIndex( mapToSource( index ) );
    if ( item && item->query() )
    {
        tDebug() << item->query()->toString() << item->query()->resolvingFinished() << item->query()->playable();
    }
    if ( !item || !item->query() || ( item->query()->resolvingFinished() && !item->query()->playable() ) )
    {
        removeIndex( index );
    }
}


void
QueueProxyModel::onPlaybackStarted( const Tomahawk::result_ptr& result )
{
    for ( int i = 0; i < rowCount(); i++ )
    {
        QModelIndex idx = index( i, 0 );
        PlayableItem* item = itemFromIndex( mapToSource( idx ) );
        if ( item && item->query() && ( item->query()->results().contains( result ) ||
                                        item->query()->equals( result->toQuery() ) ) )
        {
            removeIndex( idx );
            if ( !rowCount() )
                ViewManager::instance()->hideQueue();
        }
    }
}


void
QueueProxyModel::onIndexActivated( const QModelIndex& index )
{
    setCurrentIndex( QModelIndex() );
    removeIndex( index );

    if ( !rowCount() )
        ViewManager::instance()->hideQueue();
}
