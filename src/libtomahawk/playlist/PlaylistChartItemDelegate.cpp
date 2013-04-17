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

#include "PlaylistChartItemDelegate.h"

#include <QApplication>
#include <QPainter>

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


PlaylistChartItemDelegate::PlaylistChartItemDelegate( TrackView* parent, PlayableProxyModel* proxy )
    : PlaylistItemDelegate( parent, proxy )
    , m_view( parent )
    , m_model( proxy )
{
    m_topOption = QTextOption( Qt::AlignTop );
    m_topOption.setWrapMode( QTextOption::NoWrap );

    m_centerOption = QTextOption( Qt::AlignCenter );
    m_centerOption.setWrapMode( QTextOption::NoWrap );

    m_centerRightOption = QTextOption( Qt::AlignVCenter | Qt::AlignRight );
    m_centerRightOption.setWrapMode( QTextOption::NoWrap );

    m_bottomOption = QTextOption( Qt::AlignBottom );
    m_bottomOption.setWrapMode( QTextOption::NoWrap );

    connect( proxy, SIGNAL( modelReset() ), SLOT( modelChanged() ) );
    connect( parent, SIGNAL( modelChanged() ), SLOT( modelChanged() ) );
}


QSize
PlaylistChartItemDelegate::sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    QSize size = QStyledItemDelegate::sizeHint( option, index );

    int stretch = 0;
    switch ( index.row() )
    {
        case 0:
            stretch = 6;
            break;
        case 1:
            stretch = 5;
            break;
        case 2:
            stretch = 4;
            break;

        default:
            if ( index.row() < 10 )
                stretch = 3;
            else
                stretch = 2;
    }

    int rowHeight = option.fontMetrics.height() + 8;
    size.setHeight( rowHeight * stretch );

    return size;
}


void
PlaylistChartItemDelegate::paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
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
    unsigned int duration = q->duration();
    QString artist = q->artist();
    QString track = q->track();
    QPixmap avatar;
    QString upperText, lowerText;

    painter->save();
    {
        QRect r = opt.rect.adjusted( 4, 6, 0, -6 );

        // Paint Now Playing Speaker Icon
        if ( item->isPlaying() )
        {
            const int pixMargin = 4;
            const int pixHeight = r.height() - pixMargin * 2;
            QRect npr = r.adjusted( pixMargin, pixMargin + 1, pixHeight - r.width() + pixMargin, -pixMargin + 1 );
            painter->drawPixmap( npr, TomahawkUtils::defaultPixmap( TomahawkUtils::NowPlayingSpeaker, TomahawkUtils::Original, npr.size() ) );
            r.adjust( pixHeight + 8, 0, 0, 0 );
        }

        QFont figureFont = opt.font;
        figureFont.setPointSize( TomahawkUtils::defaultFontSize() + 6 );
        figureFont.setWeight( 99 );

        QFont boldFont = opt.font;
        boldFont.setPointSize( TomahawkUtils::defaultFontSize() + 1 );
        boldFont.setWeight( 99 );

        QFont smallBoldFont = opt.font;
        smallBoldFont.setPointSize( TomahawkUtils::defaultFontSize() );
        smallBoldFont.setWeight( 60 );

        QFont durationFont = opt.font;
        durationFont.setPointSize( TomahawkUtils::defaultFontSize() );
        durationFont.setWeight( 80 );
        QFontMetrics durationFontMetrics( durationFont );

        if ( index.row() == 0 )
        {
            figureFont.setPointSize( TomahawkUtils::defaultFontSize() + 33 );
            boldFont.setPointSize( TomahawkUtils::defaultFontSize() + 13 );
            smallBoldFont.setPointSize( TomahawkUtils::defaultFontSize() + 9 );
        }
        else if ( index.row() == 1 )
        {
            figureFont.setPointSize( TomahawkUtils::defaultFontSize() + 17 );
            boldFont.setPointSize( TomahawkUtils::defaultFontSize() + 9 );
            smallBoldFont.setPointSize( TomahawkUtils::defaultFontSize() + 4 );
        }
        else if ( index.row() == 2 )
        {
            figureFont.setPointSize( TomahawkUtils::defaultFontSize() + 9 );
            boldFont.setPointSize( TomahawkUtils::defaultFontSize() + 4 );
            smallBoldFont.setPointSize( TomahawkUtils::defaultFontSize() + 2 );
        }
        else if ( index.row() >= 10 )
        {
            boldFont.setPointSize( TomahawkUtils::defaultFontSize() );
            smallBoldFont.setPointSize( TomahawkUtils::defaultFontSize() - 1 );
        }

        QRect figureRect = r.adjusted( 0, 0, -option.rect.width() + 60 - 6 + r.left(), 0 );
        painter->setFont( figureFont );
        painter->setPen( option.palette.text().color().lighter( 450 ) );
        painter->drawText( figureRect, QString::number( index.row() + 1 ), m_centerOption );
        painter->setPen( opt.palette.text().color() );

        QRect pixmapRect = r.adjusted( figureRect.width() + 6, 0, -option.rect.width() + figureRect.width() + option.rect.height() - 6 + r.left(), 0 );

        if ( !m_pixmaps.contains( index ) )
        {
            m_pixmaps.insert( index, QSharedPointer< Tomahawk::PixmapDelegateFader >( new Tomahawk::PixmapDelegateFader( item->query(), pixmapRect.size(), TomahawkUtils::Original, false ) ) );
            _detail::Closure* closure = NewClosure( m_pixmaps[ index ], SIGNAL( repaintRequest() ), const_cast<PlaylistChartItemDelegate*>(this), SLOT( doUpdateIndex( const QPersistentModelIndex& ) ), QPersistentModelIndex( index ) );
            closure->setAutoDelete( false );
        }

        const QPixmap pixmap = m_pixmaps[ index ]->currentPixmap();
        painter->drawPixmap( pixmapRect, pixmap );

        r.adjust( pixmapRect.width() + figureRect.width() + 18, 1, -28, 0 );
        QRect rightRect = r.adjusted( r.width() - durationFontMetrics.width( TomahawkUtils::timeToString( duration ) ), 0, 0, 0 );
        QRect leftRect = r.adjusted( 0, 0, -( rightRect.width() + 8 ), 0 );

/*        const int sourceIconSize = r.height();

        if ( hoveringOver() == index && index.column() == 0 )
        {
            const QPixmap infoIcon = TomahawkUtils::defaultPixmap( TomahawkUtils::InfoIcon, TomahawkUtils::Original, QSize( sourceIconSize, sourceIconSize ) );
            QRect arrowRect = QRect( rightRect.right() - sourceIconSize, r.center().y() - sourceIconSize / 2, infoIcon.width(), infoIcon.height() );
            painter->drawPixmap( arrowRect, infoIcon );

            setInfoButtonRect( index, arrowRect );
            rightRect.moveLeft( rightRect.left() - infoIcon.width() - 8 );
            leftRect.adjust( 0, 0, -( infoIcon.width() + 8 ), 0 );
        }
        else if ( q->numResults() && !q->results().first()->sourceIcon( TomahawkUtils::RoundedCorners, QSize( sourceIconSize, sourceIconSize ) ).isNull() )
        {
            const QPixmap sourceIcon = q->results().first()->sourceIcon( TomahawkUtils::RoundedCorners, QSize( sourceIconSize, sourceIconSize ) );
            painter->setOpacity( 0.8 );
            painter->drawPixmap( QRect( rightRect.right() - sourceIconSize, r.center().y() - sourceIconSize / 2, sourceIcon.width(), sourceIcon.height() ), sourceIcon );
            painter->setOpacity( 1.0 );
            rightRect.moveLeft( rightRect.left() - sourceIcon.width() - 8 );
        }*/

        painter->setFont( boldFont );
        QString text = painter->fontMetrics().elidedText( track, Qt::ElideRight, leftRect.width() );
        painter->drawText( leftRect, text, m_topOption );

        painter->setFont( smallBoldFont );
        text = painter->fontMetrics().elidedText( artist, Qt::ElideRight, leftRect.width() );
        painter->drawText( index.row() >= 10 ? leftRect : leftRect.adjusted( 0, painter->fontMetrics().height() + 6, 0, 0 ), text, index.row() >= 10 ? m_bottomOption : m_topOption );

        if ( duration > 0 )
        {
            painter->setPen( opt.palette.text().color() );
            painter->setFont( durationFont );
            painter->drawText( rightRect, TomahawkUtils::timeToString( duration ), m_centerRightOption );
        }
    }
    painter->restore();
}


void
PlaylistChartItemDelegate::doUpdateIndex( const QPersistentModelIndex& idx )
{
    if ( idx.isValid() )
        emit updateIndex( idx );
}


void
PlaylistChartItemDelegate::modelChanged()
{
    m_pixmaps.clear();
}

