/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "AlbumItemDelegate.h"

#include <QApplication>
#include <QPainter>
#include <QAbstractTextDocumentLayout>

#include "Query.h"
#include "Result.h"
#include "Artist.h"
#include "Source.h"
#include "SourceList.h"

#include "PlaylistView.h"
#include "PlayableModel.h"
#include "PlayableItem.h"
#include "PlayableProxyModel.h"
#include "TrackView.h"
#include "ViewHeader.h"

#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"
#include <utils/PixmapDelegateFader.h>
#include <utils/Closure.h>

using namespace Tomahawk;


AlbumItemDelegate::AlbumItemDelegate( TrackView* parent, PlayableProxyModel* proxy, bool showArtist )
    : PlaylistItemDelegate( parent, proxy )
    , m_view( parent )
    , m_model( proxy )
    , m_showArtist( showArtist )
{
}


QSize
AlbumItemDelegate::sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    QSize size = QStyledItemDelegate::sizeHint( option, index );

    int rowHeight = option.fontMetrics.height() + 8;
    size.setHeight( rowHeight * 1.5 );

    return size;
}


void
AlbumItemDelegate::paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    PlayableItem* item = m_model->itemFromIndex( m_model->mapToSource( index ) );
    Q_ASSERT( item );

    QStyleOptionViewItemV4 opt = option;
    prepareStyleOption( &opt, index, item );
    opt.text.clear();

    qApp->style()->drawControl( QStyle::CE_ItemViewItem, &opt, painter );
    if ( m_view->header()->visualIndex( index.column() ) > 0 )
        return;

    const track_ptr& track = item->query()->track();
    QString lowerText;

    painter->save();
    {
        QRect r = opt.rect.adjusted( 4, 6, -12, -6 );

        // Paint Now Playing Speaker Icon
        if ( item->isPlaying() )
        {
            const int pixHeight = r.height();
            QRect npr = r.adjusted( 0, 0, pixHeight - r.width(), 0 );
            painter->drawPixmap( npr, TomahawkUtils::defaultPixmap( TomahawkUtils::NowPlayingSpeaker, TomahawkUtils::Original, npr.size() ) );
            r.adjust( pixHeight, 0, 0, 0 );
        }

        painter->setFont( m_bigBoldFont );
        painter->setPen( opt.palette.foreground().color() );

        QRect leftRect = r;
        QRect figureRect = r.adjusted( 4, 0, 0, 0 );
        figureRect.setWidth( QFontMetrics( painter->font() ).width( "888" ) );

        if ( hoveringOver() == index && index.column() == 0 )
        {
            drawInfoButton( painter, figureRect.adjusted( 1, 0, 0, 0 ), index, 1.0 );
        }
        else
        {
            painter->drawText( figureRect, QString::number( index.row() + 1 ), QTextOption( Qt::AlignCenter ) );
        }

        leftRect = r;
        leftRect.adjust( figureRect.width() + 12, 0, 0, 0 );
        QRect rightRect = r.adjusted( r.width() - m_smallBoldFontMetrics.width( TomahawkUtils::timeToString( track->duration() ) ), 0, 0, 0 );
        {
            const QRect leftRectBefore = leftRect;
            leftRect = drawSourceIcon( painter, leftRect, item, 1.0 );
            rightRect.moveLeft( rightRect.left() - ( leftRectBefore.width() - leftRect.width() ) );
            leftRect.setWidth( leftRect.width() - rightRect.width() );
        }

        QString rawText = track->track();
        if ( m_showArtist )
        {
            rawText = QString( "%1 - %2" ).arg( rawText ).arg( track->artist() );
        }
        const QString text = painter->fontMetrics().elidedText( rawText, Qt::ElideRight, leftRect.width() );
        painter->setPen( opt.palette.text().color() );
        painter->drawText( leftRect, text, m_centerOption );

        if ( track->duration() > 0 )
        {
            painter->setFont( m_smallBoldFont );
            painter->drawText( rightRect, TomahawkUtils::timeToString( track->duration() ), m_centerRightOption );
        }
    }
    painter->restore();
}
