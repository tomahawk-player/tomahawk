/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "HistoryItem.h"

#include "ViewManager.h"
#include "GenericPageItems.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"
#include "TemporaryPageItem.h"
#include "SourceList.h"

using namespace Tomahawk;

const unsigned int MAX_HISTORY_ITEMS = 15;       // 0 disables this feature


HistoryItem::HistoryItem( SourcesModel* model, SourceTreeItem* parent, const QString& text, int peerSortValue )
    : GroupItem( model, parent, text, peerSortValue )
{
    connect( ViewManager::instance(), SIGNAL( tempPageActivated( Tomahawk::ViewPage* ) ), SLOT( tempPageActivated( Tomahawk::ViewPage* ) ) );
}


HistoryItem::~HistoryItem()
{
}


void
HistoryItem::tempPageActivated( Tomahawk::ViewPage* v )
{
    const int idx = children().count();
    int latest = idx;
    if ( idx )
    {
        latest = children().last()->IDValue();

        foreach ( TemporaryPageItem* page, m_tempItems )
        {
            if ( page->page() == v )
            {
                emit selectRequest( page );
                return;
            }
        }
    }

    // Only keep a certain amount of temporary pages at once
    while ( MAX_HISTORY_ITEMS > 0 && m_tempItems.size() > (int)MAX_HISTORY_ITEMS - 1 )
    {
        TemporaryPageItem* item = m_tempItems.takeFirst();
        QTimer::singleShot( 0, item, SLOT( removeFromList() ) );
    }

    emit beginRowsAdded( idx, idx );

    TemporaryPageItem* tempPage = new TemporaryPageItem( model(), this, v, latest + 1 );
    connect( tempPage, SIGNAL( removed() ), this, SLOT( temporaryPageDestroyed() ) );
    m_tempItems << tempPage;
    endRowsAdded();

    emit selectRequest( tempPage );
}


void
HistoryItem::temporaryPageDestroyed()
{
    TemporaryPageItem* tempPage = qobject_cast< TemporaryPageItem* >( sender() );
    Q_ASSERT( tempPage );
    m_tempItems.removeAll( tempPage );
}
