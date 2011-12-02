/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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

#include "temporarypageitem.h"
#include "viewmanager.h"
#include "widgets/infowidgets/AlbumInfoWidget.h"
#include "widgets/infowidgets/ArtistInfoWidget.h"
#include "widgets/searchwidget.h"

using namespace Tomahawk;

TemporaryPageItem::TemporaryPageItem ( SourcesModel* mdl, SourceTreeItem* parent, ViewPage* page, int sortValue )
    : SourceTreeItem( mdl, parent, SourcesModel::TemporaryPage )
    , m_page( page )
    , m_icon( QIcon( RESPATH "images/playlist-icon.png" ) )
    , m_sortValue( sortValue )
{
    if ( dynamic_cast< ArtistInfoWidget* >( page ) )
        m_icon = QIcon( RESPATH "images/artist-icon.png" );
    else if ( dynamic_cast< AlbumInfoWidget* >( page ) )
        m_icon = QIcon( RESPATH "images/album-icon.png" );
    else if ( dynamic_cast< SearchWidget* >( page ) )
        m_icon = QIcon( RESPATH "images/search-icon.png" );

    model()->linkSourceItemToPage( this, page );
}


QString
TemporaryPageItem::text() const
{
    return m_page->title();
}


void
TemporaryPageItem::activate()
{
    ViewManager::instance()->show( m_page );
}


QIcon
TemporaryPageItem::icon() const
{
    return m_icon;
}


int
TemporaryPageItem::peerSortValue() const
{
    return m_sortValue;
}


int
TemporaryPageItem::IDValue() const
{
    return m_sortValue;
}


void
TemporaryPageItem::removeFromList()
{
    ViewManager::instance()->removeFromHistory( m_page );

    model()->removeSourceItemLink( this );

    int idx = parent()->children().indexOf( this );
    parent()->beginRowsRemoved( idx, idx );
    parent()->removeChild( this );
    parent()->endRowsRemoved();

    emit removed();

    deleteLater();
}
