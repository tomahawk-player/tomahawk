/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include "PlaylistItemDelegate.h"

#include <QApplication>
#include <QPainter>

#include "Query.h"
#include "Result.h"
#include "Artist.h"
#include "Source.h"
#include "SourceList.h"

#include "PlayableModel.h"
#include "PlayableItem.h"
#include "PlayableProxyModel.h"
#include "TrackView.h"
#include "ViewHeader.h"

#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"

using namespace Tomahawk;


PlaylistItemDelegate::PlaylistItemDelegate( TrackView* parent, PlayableProxyModel* proxy )
    : QStyledItemDelegate( (QObject*)parent )
    , m_view( parent )
    , m_model( proxy )
{
    m_topOption = QTextOption( Qt::AlignTop );
    m_topOption.setWrapMode( QTextOption::NoWrap );

    m_bottomOption = QTextOption( Qt::AlignBottom );
    m_bottomOption.setWrapMode( QTextOption::NoWrap );
}


void
PlaylistItemDelegate::updateRowSize( const QModelIndex& index )
{
    emit sizeHintChanged( index );
}


QSize
PlaylistItemDelegate::sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    QSize size = QStyledItemDelegate::sizeHint( option, index );

    {
        if ( m_model->style() == PlayableProxyModel::Short || m_model->style() == PlayableProxyModel::ShortWithAvatars )
        {
            int rowHeight = option.fontMetrics.height() + 8;
            size.setHeight( rowHeight * 2 );
        }
        else if ( m_model->style() == PlayableProxyModel::Detailed )
        {
            int rowHeight = option.fontMetrics.height() * 1.6;
            size.setHeight( rowHeight );
        }
    }

    return size;
}


QWidget*
PlaylistItemDelegate::createEditor( QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    Q_UNUSED( parent );
    Q_UNUSED( option );
    Q_UNUSED( index );
    return 0;
}


void
PlaylistItemDelegate::prepareStyleOption( QStyleOptionViewItemV4* option, const QModelIndex& index, PlayableItem* item ) const
{
    initStyleOption( option, index );

    TomahawkUtils::prepareStyleOption( option, index, item );
}


void
PlaylistItemDelegate::paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    int style = index.data( PlayableProxyModel::StyleRole ).toInt();
    switch ( style )
    {
        case PlayableProxyModel::Detailed:
            paintDetailed( painter, option, index );
            break;

        case PlayableProxyModel::Short:
            paintShort( painter, option, index );
            break;
        case PlayableProxyModel::ShortWithAvatars:
            paintShort( painter, option, index, true );
            break;
    }
}


void
PlaylistItemDelegate::paintShort( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index, bool useAvatars ) const
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
    QString artist = q->artist();
    QString track = q->track();
    QPixmap pixmap;
    QString upperText, lowerText;
    source_ptr source = item->query()->playedBy().first;

    if ( source.isNull() )
    {
        upperText = track;
        lowerText = artist;
    }
    else
    {
        upperText = QString( "%1 - %2" ).arg( artist ).arg( track );
        QString playtime = TomahawkUtils::ageToString( QDateTime::fromTime_t( item->query()->playedBy().second ), true );

        if ( source == SourceList::instance()->getLocal() )
            lowerText = QString( tr( "played %1 by you" ) ).arg( playtime );
        else
            lowerText = QString( tr( "played %1 by %2" ) ).arg( playtime ).arg( source->friendlyName() );
    }

    painter->save();
    {
        QRect r = opt.rect.adjusted( 3, 6, 0, -6 );

        // Paint Now Playing Speaker Icon
        if ( item->isPlaying() )
        {
            const int pixMargin = 2;
            const int pixHeight = r.height() - pixMargin * 2;
            QRect npr = r.adjusted( pixMargin, pixMargin + 1, pixHeight - r.width() + pixMargin, -pixMargin + 1 );
            painter->drawPixmap( npr, TomahawkUtils::defaultPixmap( TomahawkUtils::NowPlayingSpeaker, TomahawkUtils::Original, npr.size() ) );
            r.adjust( pixHeight + 8, 0, 0, 0 );
        }

        painter->setPen( opt.palette.text().color() );

        QRect ir = r.adjusted( 4, 0, -option.rect.width() + option.rect.height() - 8 + r.left(), 0 );

        if ( useAvatars )
        {
            if ( !source.isNull() )
                pixmap = source->avatar( TomahawkUtils::RoundedCorners, ir.size() );
        }
        else
            pixmap = item->query()->cover( ir.size(), false );

        if ( pixmap.isNull() )
        {
            if ( !useAvatars )
                pixmap = TomahawkUtils::defaultPixmap( TomahawkUtils::DefaultTrackImage, TomahawkUtils::Original, ir.size() );
            else
                pixmap = TomahawkUtils::defaultPixmap( TomahawkUtils::DefaultSourceAvatar, TomahawkUtils::RoundedCorners, ir.size() );
        }

        painter->drawPixmap( ir, pixmap );

        QFont boldFont = opt.font;
        boldFont.setBold( true );

        r.adjust( ir.width() + 12, 0, -12, 0 );
        painter->setFont( boldFont );
        QString text = painter->fontMetrics().elidedText( upperText, Qt::ElideRight, r.width() );
        painter->drawText( r.adjusted( 0, 1, 0, 0 ), text, m_topOption );

        painter->setFont( opt.font );
        if ( !( option.state & QStyle::State_Selected || item->isPlaying() ) )
            painter->setPen( Qt::gray );

        text = painter->fontMetrics().elidedText( lowerText, Qt::ElideRight, r.width() );
        painter->drawText( r.adjusted( 0, 1, 0, 0 ), text, m_bottomOption );
    }
    painter->restore();
}


void
PlaylistItemDelegate::paintDetailed( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    PlayableItem* item = m_model->itemFromIndex( m_model->mapToSource( index ) );
    Q_ASSERT( item );

    QTextOption textOption( Qt::AlignVCenter | (Qt::Alignment)index.data( Qt::TextAlignmentRole ).toUInt() );
    textOption.setWrapMode( QTextOption::NoWrap );

    QStyleOptionViewItemV4 opt = option;
    prepareStyleOption( &opt, index, item );
    opt.text.clear();
    qApp->style()->drawControl( QStyle::CE_ItemViewItem, &opt, painter );

    if ( m_view->hoveredIndex().row() == index.row() && m_view->hoveredIndex().column() == index.column() && !index.data().toString().isEmpty() &&
       ( index.column() == PlayableModel::Artist || index.column() == PlayableModel::Album || index.column() == PlayableModel::Track ) )
    {
        opt.rect.setWidth( opt.rect.width() - opt.rect.height() - 2 );
        QRect arrowRect( opt.rect.x() + opt.rect.width(), opt.rect.y() + 1, opt.rect.height() - 2, opt.rect.height() - 2 );

        QPixmap infoIcon = TomahawkUtils::defaultPixmap( TomahawkUtils::InfoIcon, TomahawkUtils::Original, arrowRect.size() );
        painter->drawPixmap( arrowRect, infoIcon );
    }

    painter->save();

/*    if ( index.column() == PlayableModel::Score )
    {
        QColor barColor( 167, 183, 211 ); // This matches the sidebar (sourcetreeview.cpp:672)
        if ( opt.state & QStyle::State_Selected && !item->isPlaying() )
            painter->setPen( Qt::white );
        else
            painter->setPen( barColor );

        QRect r = opt.rect.adjusted( 3, 3, -6, -4 );
        painter->drawRect( r );

        QRect fillR = r;
        int fillerWidth = (int)( index.data().toFloat() * (float)fillR.width() );
        fillR.adjust( 0, 0, -( fillR.width() - fillerWidth ), 0 );

        if ( opt.state & QStyle::State_Selected && !item->isPlaying() )
            painter->setBrush( TomahawkUtils::Colors::NOW_PLAYING_ITEM.lighter() );
        else
            painter->setBrush( barColor );

        painter->drawRect( fillR );
    }
    else */ if ( item->isPlaying() )
    {
        QRect r = opt.rect.adjusted( 3, 0, 0, 0 );

        // Paint Now Playing Speaker Icon
        if ( m_view->header()->visualIndex( index.column() ) == 0 )
        {
            const int pixMargin = 1;
            const int pixHeight = r.height() - pixMargin * 2;
            QRect npr = r.adjusted( pixMargin, pixMargin, pixHeight - r.width() + pixMargin, -pixMargin );
            painter->drawPixmap( npr, TomahawkUtils::defaultPixmap( TomahawkUtils::NowPlayingSpeaker, TomahawkUtils::Original, npr.size() ) );
            r.adjust( pixHeight + 6, 0, 0, 0 );
        }

        painter->setPen( opt.palette.text().color() );
        QString text = painter->fontMetrics().elidedText( index.data().toString(), Qt::ElideRight, r.width() - 3 );
        painter->drawText( r.adjusted( 0, 1, 0, 0 ), text, textOption );
    }
    else
    {
        painter->setPen( opt.palette.text().color() );
        QString text = painter->fontMetrics().elidedText( index.data().toString(), Qt::ElideRight, opt.rect.width() - 6 );
        painter->drawText( opt.rect.adjusted( 3, 1, -3, 0 ), text, textOption );
    }

    painter->restore();
}
