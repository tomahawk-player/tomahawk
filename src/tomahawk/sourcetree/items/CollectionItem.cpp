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


#include "CollectionItem.h"

#include "utils/ImageRegistry.h"
#include "playlist/TrackView.h"
#include "playlist/PlayableProxyModel.h"
#include "ViewManager.h"
#include "ViewPage.h"

#include <QString>
#include <QIcon>


CollectionItem::CollectionItem( SourcesModel* model, SourceTreeItem* parent, const Tomahawk::collection_ptr& collection )
    : SourceTreeItem( model, parent, SourcesModel::Collection )
    , m_sortValue( -150 )
    , m_collection( collection )
{
    m_text = tr( "Collection" );
    m_icon = ImageRegistry::instance()->icon( RESPATH "images/collection.svg" );
}


CollectionItem::~CollectionItem()
{}


QString
CollectionItem::text() const
{
    return m_text;
}


QIcon
CollectionItem::icon() const
{
    return m_icon;
}


int
CollectionItem::peerSortValue() const
{
    return m_sortValue;
}


void
CollectionItem::setSortValue( int value )
{
    m_sortValue = value;
}


int
CollectionItem::trackCount() const
{
    return m_collection->trackCount();
}


void
CollectionItem::activate()
{
    Tomahawk::ViewPage* page = ViewManager::instance()->show( m_collection );
    model()->linkSourceItemToPage( this, page );
}
