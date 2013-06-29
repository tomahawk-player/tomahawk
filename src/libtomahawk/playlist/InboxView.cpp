/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Teo Mrnjavac <teo@kde.org>
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
#include "PlayableProxyModel.h"
#include "ContextMenu.h"
#include "utils/Logger.h"

InboxView::InboxView(QWidget *parent) :
    TrackView(parent)
{
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
