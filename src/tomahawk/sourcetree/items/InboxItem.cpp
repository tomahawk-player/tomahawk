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


#include "InboxItem.h"

#include "utils/ImageRegistry.h"
#include "ViewManager.h"
#include "ViewPage.h"
#include "playlist/InboxModel.h"

#include <QString>
#include <QIcon>


InboxItem::InboxItem( SourcesModel* model, SourceTreeItem* parent )
    : SourceTreeItem( model, parent, SourcesModel::Inbox )
    , m_sortValue( -150 )
{
    m_text = tr( "Inbox" );
    m_icon = ImageRegistry::instance()->icon( RESPATH "images/inbox.svg" );
}


InboxItem::~InboxItem()
{}


QString
InboxItem::text() const
{
    return m_text;
}


QIcon
InboxItem::icon() const
{
    return m_icon;
}


int
InboxItem::peerSortValue() const
{
    return m_sortValue;
}


void
InboxItem::setSortValue( int value )
{
    m_sortValue = value;
}


int
InboxItem::unlistenedCount() const
{
    return ViewManager::instance()->inboxModel()->unlistenedCount();
}


void
InboxItem::activate()
{
    Tomahawk::ViewPage* page = ViewManager::instance()->showInboxPage();
    model()->linkSourceItemToPage( this, page );
}
