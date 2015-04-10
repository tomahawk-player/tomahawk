/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2014, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "QueueView.h"

#include "playlist/TrackView.h"
#include "playlist/ContextView.h"
#include "playlist/PlaylistModel.h"
#include "playlist/QueueProxyModel.h"
#include "playlist/TrackItemDelegate.h"
#include "utils/Logger.h"
#include "Pipeline.h"
#include "Source.h"
#include "SourceList.h"
#include "TomahawkSettings.h"
#include "utils/TomahawkUtilsGui.h"

using namespace Tomahawk;


QueueView::QueueView( QWidget* parent )
    : PlaylistViewPage( parent )
{
    view()->setCaption( tr( "Queue Details" ) );

    view()->trackView()->setProxyModel( new QueueProxyModel( view()->trackView() ) );
    view()->trackView()->proxyModel()->setStyle( PlayableProxyModel::SingleColumn );
    view()->trackView()->setHeaderHidden( true );
    view()->trackView()->setUniformRowHeights( false );

    PlaylistModel* queueModel = new PlaylistModel( view()->trackView() );
    queueModel->setAcceptPlayableQueriesOnly( true );
    queueModel->setReadOnly( false );
    queueModel->setTitle( tr( "Queue" ) );
    setPixmap( TomahawkUtils::defaultPixmap( TomahawkUtils::Queue ) );

    view()->trackView()->setPlayableModel( queueModel );
    view()->setEmptyTip( tr( "The queue is currently empty. Drop something to enqueue it!" ) );

    TrackItemDelegate* delegate = new TrackItemDelegate( TrackItemDelegate::LovedTracks, view()->trackView(), view()->trackView()->proxyModel() );
    view()->trackView()->setPlaylistItemDelegate( delegate );

    if ( Pipeline::instance()->isRunning() && SourceList::instance()->isReady() )
    {
        restoreState();
    }
    else
    {
        connect( SourceList::instance(), SIGNAL( ready() ), SLOT( restoreState() ) );
        connect( Pipeline::instance(), SIGNAL( running() ), SLOT( restoreState() ) );
    }
}


QueueView::~QueueView()
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO;
    saveState();
}


void
QueueView::restoreState()
{
    if ( !Pipeline::instance()->isRunning() || !SourceList::instance()->isReady() )
        return;

    QVariantList vl = TomahawkSettings::instance()->queueState().toList();
    QList< query_ptr > ql;

    foreach ( const QVariant& v, vl )
    {
        QVariantMap map = v.toMap();
        query_ptr q = Query::get( map["artist"].toString(), map["track"].toString(), map["album"].toString() );
        ql << q;
    }

    if ( !ql.isEmpty() )
    {
        view()->trackView()->model()->appendQueries( ql );
    }
}


void
QueueView::saveState()
{
    QVariantList vl;
    foreach ( const query_ptr& query, view()->trackView()->model()->queries() )
    {
        vl << query->toVariant();
    }

    TomahawkSettings::instance()->setQueueState( vl );
}
