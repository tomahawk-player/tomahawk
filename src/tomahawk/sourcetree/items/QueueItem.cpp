/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
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


#include "QueueItem.h"

#include "utils/ImageRegistry.h"
#include "playlist/TrackView.h"
#include "playlist/PlayableProxyModel.h"
#include "ViewManager.h"
#include "ViewPage.h"

#include <QString>
#include <QIcon>


QueueItem::QueueItem( SourcesModel* model, SourceTreeItem* parent )
    : SourceTreeItem( model, parent, SourcesModel::Queue )
    , m_sortValue( -150 )
{
    m_text = tr( "Queue" );
    m_icon = ImageRegistry::instance()->icon( RESPATH "images/queue.svg" );
}


QueueItem::~QueueItem()
{}


QString
QueueItem::text() const
{
    return m_text;
}


QIcon
QueueItem::icon() const
{
    return m_icon;
}


int
QueueItem::peerSortValue() const
{
    return m_sortValue;
}


void
QueueItem::setSortValue( int value )
{
    m_sortValue = value;
}


int
QueueItem::unlistenedCount() const
{
    return ViewManager::instance()->queue()->trackView()->proxyModel()->rowCount();
}


void
QueueItem::activate()
{
    Tomahawk::ViewPage* page = ViewManager::instance()->showQueuePage();
    model()->linkSourceItemToPage( this, page );
}
