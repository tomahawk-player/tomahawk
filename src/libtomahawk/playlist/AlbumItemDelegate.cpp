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


AlbumItemDelegate::AlbumItemDelegate( TrackView* parent, PlayableProxyModel* proxy )
    : PlaylistItemDelegate( parent, proxy )
    , m_view( parent )
    , m_model( proxy )
{
    m_centerOption = QTextOption( Qt::AlignVCenter );
    m_centerOption.setWrapMode( QTextOption::NoWrap );

    m_centerRightOption = QTextOption( Qt::AlignVCenter | Qt::AlignRight );
    m_centerRightOption.setWrapMode( QTextOption::NoWrap );

    connect( proxy, SIGNAL( modelReset() ), this, SLOT( modelChanged() ) );
    connect( parent, SIGNAL( modelChanged() ), this, SLOT( modelChanged() ) );
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

    const query_ptr q = item->query()->displayQuery();
    const QString artist = q->artist();
    const QString album = q->album();
    const QString track = q->track();
    int duration = q->duration();
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

        QFont boldFont = opt.font;
        boldFont.setPointSize( TomahawkUtils::defaultFontSize() + 2 );
        boldFont.setWeight( 99 );
        QFontMetrics boldFontMetrics( boldFont );

        QFont smallBoldFont = opt.font;
        smallBoldFont.setPointSize( TomahawkUtils::defaultFontSize() - 1 );
        smallBoldFont.setBold( true );
        smallBoldFont.setWeight( 60 );
        QFontMetrics smallBoldFontMetrics( smallBoldFont );

        painter->setFont( boldFont );
        painter->setPen( option.palette.text().color().lighter( 450 ) );
        
        QRect figureRect = r.adjusted( 4, 0, 0, 0 );
        figureRect.setWidth( QFontMetrics( painter->font() ).width( "888" ) );
        painter->drawText( figureRect, QString::number( index.row() + 1 ), QTextOption( Qt::AlignCenter ) );

        r.adjust( figureRect.width() + 12, 0, 0, 0 );
        QRect leftRect = r.adjusted( 0, 0, -48, 0 );
        QRect rightRect = r.adjusted( r.width() - smallBoldFontMetrics.width( TomahawkUtils::timeToString( duration ) ), 0, 0, 0 );

        QString text = painter->fontMetrics().elidedText( track, Qt::ElideRight, leftRect.width() );
        painter->setPen( opt.palette.text().color() );
        painter->drawText( leftRect, text, m_centerOption );

        const int sourceIconSize = r.height();
        if ( q->numResults() && !q->results().first()->sourceIcon( TomahawkUtils::RoundedCorners, QSize( sourceIconSize, sourceIconSize ) ).isNull() )
        {
            const QPixmap sourceIcon = q->results().first()->sourceIcon( TomahawkUtils::RoundedCorners, QSize( sourceIconSize, sourceIconSize ) );
            painter->setOpacity( 0.8 );
            painter->drawPixmap( QRect( rightRect.right() - sourceIconSize, r.center().y() - sourceIconSize / 2, sourceIcon.width(), sourceIcon.height() ), sourceIcon );
            painter->setOpacity( 1.0 );
            rightRect.moveLeft( rightRect.left() - sourceIcon.width() - 8 );
        }

        if ( duration > 0 )
        {
            painter->setPen( opt.palette.text().color() );
            painter->setFont( smallBoldFont );
            painter->drawText( rightRect, TomahawkUtils::timeToString( duration ), m_centerRightOption );
        }
    }
    painter->restore();
}


void
AlbumItemDelegate::doUpdateIndex( const QPersistentModelIndex& idx )
{
    if ( idx.isValid() )
        emit updateIndex( idx );
}


void
AlbumItemDelegate::modelChanged()
{
}
