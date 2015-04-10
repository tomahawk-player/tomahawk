/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Teo Mrnjavac <teo@kde.org>
 *   Copyright 2014, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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


#include "InboxView.h"
#include "InboxModel.h"
#include "ContextView.h"
#include "PlayableProxyModel.h"
#include "ContextMenu.h"
#include "playlist/TrackItemDelegate.h"
#include "ViewManager.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"


InboxView::InboxView( QWidget* parent ) :
    TrackView( parent )
{
    proxyModel()->setStyle( PlayableProxyModel::SingleColumn );

    TrackView::setGuid( "inbox" );
    setHeaderHidden( true );
    setUniformRowHeights( false );
    setIndentation( 0 );
}


void
InboxView::deleteSelectedItems()
{
    InboxModel* inboxModel = qobject_cast< InboxModel* >( model() );
    if ( inboxModel != 0 )
    {
        proxyModel()->removeIndexes( selectedIndexes() );
    }
}


void
InboxView::onMenuTriggered( int action )
{
    if ( action == Tomahawk::ContextMenu::ActionMarkListened )
    {
        tDebug() << Q_FUNC_INFO << "Mark as Listened";
        InboxModel* inboxModel = qobject_cast< InboxModel* >( model() );
        if ( inboxModel != 0 )
        {
            QModelIndexList sourceIndexes;
            foreach ( const QModelIndex& index, selectedIndexes() )
            {
                if ( index.column() )
                    continue;

                sourceIndexes << proxyModel()->mapToSource( index );
            }
            inboxModel->markAsListened( sourceIndexes );
        }
    }
    else
        TrackView::onMenuTriggered( action );
}


InboxPage::InboxPage( QWidget* parent )
    : PlaylistViewPage( parent )
{
    InboxView* inboxView = new InboxView( this );
    view()->setCaption( tr( "Inbox Details" ) );

    setPixmap( TomahawkUtils::defaultPixmap( TomahawkUtils::Inbox ) );

    TrackItemDelegate* delegate = new TrackItemDelegate( TrackItemDelegate::Inbox, inboxView, inboxView->proxyModel() );
    inboxView->setPlaylistItemDelegate( delegate );

    view()->setTrackView( inboxView );
    inboxView->setPlayableModel( ViewManager::instance()->inboxModel() );
    inboxView->setEmptyTip( tr( "Your friends have not shared any recommendations with you yet. Connect with them and share your musical gems!" ) );
}
