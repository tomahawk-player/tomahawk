/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2014-2015, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "TrackItemDelegate.h"

#include <QApplication>
#include <QPainter>
#include <QDateTime>

#include "Query.h"
#include "Result.h"
#include "Artist.h"
#include "Source.h"
#include "SourceList.h"

#include "TrackView.h"
#include "PlayableModel.h"
#include "PlayableItem.h"
#include "PlayableProxyModel.h"
#include "TrackView.h"
#include "ViewHeader.h"

#include "audio/AudioEngine.h"
#include "utils/ImageRegistry.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"


using namespace Tomahawk;


TrackItemDelegate::TrackItemDelegate( DisplayMode mode, TrackView* parent, PlayableProxyModel* proxy )
    : PlaylistItemDelegate( parent, proxy )
    , m_view( parent )
    , m_model( proxy )
    , m_mode( mode )
{
}


QSize
TrackItemDelegate::sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    QSize size;

    PlayableItem* item = m_model->itemFromIndex( m_model->mapToSource( index ) );
    Q_ASSERT( item );

    const int rowHeight = option.fontMetrics.height() + 5;
    if ( item->source() )
    {
        if ( index.row() == 0 )
        {
            size.setHeight( rowHeight * 3 );
        }
        else
        {
            size.setHeight( rowHeight * 4.5 );
        }
    }
    else
    {
        size.setHeight( rowHeight * 2.5 );
    }

    return size;
}


void
TrackItemDelegate::paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    if ( m_view->header()->visualIndex( index.column() ) > 0 )
        return;

    painter->setRenderHint( QPainter::TextAntialiasing );

    PlayableItem* item = m_model->itemFromIndex( m_model->mapToSource( index ) );
    Q_ASSERT( item );

    QStyleOptionViewItemV4 opt = option;
    prepareStyleOption( &opt, index, item );

    if ( item->source() )
    {
        drawSource( painter, opt, index, opt.rect, item );
    }

    if ( item->query() )
    {
        //FIXME
        bool isUnlistened = true;
        if ( m_mode == Inbox )
        {
            QList< Tomahawk::SocialAction > socialActions = item->query()->queryTrack()->allSocialActions();
            foreach ( const Tomahawk::SocialAction& sa, socialActions )
            {
                if ( sa.action.toString() == "Inbox" && sa.value.toBool() == false )
                {
                    isUnlistened = false;
                    break;
                }
            }
        }

        drawTrack( painter, opt, index, opt.rect, item );
    }
}


void
TrackItemDelegate::modelChanged()
{
    PlaylistItemDelegate::modelChanged();
}
