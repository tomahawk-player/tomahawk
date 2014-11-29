/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2010-2013, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "TemporaryPageItem.h"

#include "utils/LinkGenerator.h"
#include "ViewManager.h"
#include "viewpages/AlbumViewPage.h"
#include "viewpages/ArtistViewPage.h"
#include "viewpages/TrackViewPage.h"
#include "viewpages/SearchViewPage.h"
#include "utils/ImageRegistry.h"
#include "utils/Closure.h"
#include "utils/Logger.h"

#include <QAction>

namespace {
    enum LinkType {
        ArtistLink,
        AlbumLink,
        TrackLink
    };
}

using namespace Tomahawk;

TemporaryPageItem::TemporaryPageItem( SourcesModel* mdl, SourceTreeItem* parent, ViewPage* page, int sortValue )
    : SourceTreeItem( mdl, parent, SourcesModel::TemporaryPage )
    , m_page( page )
    , m_icon( ImageRegistry::instance()->icon( RESPATH "images/playlist-icon.svg" ) )
    , m_sortValue( sortValue )
{
    QAction* action = 0;

    if ( dynamic_cast< ArtistInfoWidget* >( page ) )
    {
        action = new QAction( tr( "Copy Artist Link" ), this );
        action->setProperty( "linkType", (int)ArtistLink );

        m_icon = ImageRegistry::instance()->icon( RESPATH "images/artist-icon.svg" );
    }
    else if ( dynamic_cast< AlbumInfoWidget* >( page ) )
    {
        action = new QAction( tr( "Copy Album Link" ), this );
        action->setProperty( "linkType", (int)AlbumLink );

        m_icon = ImageRegistry::instance()->icon( RESPATH "images/album-icon.svg" );
    }
    else if ( dynamic_cast< TrackInfoWidget* >( page ) )
    {
        action = new QAction( tr( "Copy Track Link" ), this );
        action->setProperty( "linkType", (int)TrackLink );

        m_icon = ImageRegistry::instance()->icon( RESPATH "images/track-icon.svg" );
    }
    else if ( dynamic_cast< SearchWidget* >( page ) )
    {
        m_icon = ImageRegistry::instance()->icon( RESPATH "images/search-icon.svg" );
    }

    if ( action )
    {
        m_customActions << action;
        NewClosure( action, SIGNAL( triggered() ), this, SLOT( linkActionTriggered( QAction* ) ), action );
    }

    model()->linkSourceItemToPage( this, page );
}


TemporaryPageItem::~TemporaryPageItem()
{
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
TemporaryPageItem::linkActionTriggered( QAction* action )
{
    Q_ASSERT( action );
    if ( !action )
        return;

    const LinkType type = (LinkType)action->property( "linkType" ).toInt();
    switch( type )
    {
        case ArtistLink:
        {
            ArtistInfoWidget* aPage = dynamic_cast< ArtistInfoWidget* >( m_page );
            Q_ASSERT( aPage );
            Utils::LinkGenerator::instance()->copyOpenLink( aPage->artist() );

            break;
        }
        case AlbumLink:
        {
            AlbumInfoWidget* aPage = dynamic_cast< AlbumInfoWidget* >( m_page );
            Q_ASSERT( aPage );
            Utils::LinkGenerator::instance()->copyOpenLink( aPage->album() );

            break;
        }
        case TrackLink:
        {
            TrackInfoWidget* tPage = dynamic_cast< TrackInfoWidget* >( m_page );
            Q_ASSERT( tPage );
            Utils::LinkGenerator::instance()->copyToClipboard( tPage->query() );

            break;
        }
    }
}


QList< QAction* >
TemporaryPageItem::customActions() const
{
    return m_customActions;
}


ViewPage*
TemporaryPageItem::page() const
{
    return m_page;
}


bool
TemporaryPageItem::isBeingPlayed() const
{
    return m_page->isBeingPlayed();
}


void
TemporaryPageItem::removeFromList()
{
    SourceTreeItem::removeFromList();

    ViewManager::instance()->destroyPage( m_page );
}
