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

#include "albumitemdelegate.h"

#include <QApplication>
#include <QPainter>
#include <QAbstractItemView>

#include "query.h"
#include "result.h"

#include "utils/tomahawkutils.h"
#include "utils/logger.h"

#include "playlist/albumitem.h"
#include "playlist/albumproxymodel.h"


AlbumItemDelegate::AlbumItemDelegate( QAbstractItemView* parent, AlbumProxyModel* proxy )
    : QStyledItemDelegate( (QObject*)parent )
    , m_view( parent )
    , m_model( proxy )
{
    m_shadowPixmap = QPixmap( RESPATH "images/cover-shadow.png" );
    m_defaultCover = QPixmap( RESPATH "images/no-album-art-placeholder.png" );
}


QSize
AlbumItemDelegate::sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    QSize size = QStyledItemDelegate::sizeHint( option, index );
    return size;
}


void
AlbumItemDelegate::paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    AlbumItem* item = m_model->sourceModel()->itemFromIndex( m_model->mapToSource( index ) );
    if ( !item )
        return;

    QStyleOptionViewItemV4 opt = option;
    initStyleOption( &opt, QModelIndex() );
    qApp->style()->drawControl( QStyle::CE_ItemViewItem, &opt, painter );

    painter->save();

//    painter->setRenderHint( QPainter::Antialiasing );
//    painter->drawPixmap( option.rect.adjusted( 4, 4, -4, -38 ), m_shadowPixmap );

    QPixmap cover = item->cover.isNull() ? m_defaultCover : item->cover;
    QRect r = option.rect.adjusted( 6, 5, -6, -41 );

    if ( option.state & QStyle::State_Selected )
    {
#ifndef Q_OS_MAC
        opt.palette.setColor( QPalette::Text, opt.palette.color( QPalette::HighlightedText ) );
#else
        painter->save();
        painter->setRenderHint( QPainter::Antialiasing );

        QPainterPath border;
        border.addRoundedRect( r.adjusted( -2, -2, 2, 2 ), 3, 3 );
        QPen borderPen( QColor( 86, 170, 243 ) );
        borderPen.setWidth( 5 );
        painter->setPen( borderPen );
        painter->drawPath( border );

        painter->restore();
#endif
    }

    QPixmap scover;
    if ( m_cache.contains( cover.cacheKey() ) )
    {
        scover = m_cache.value( cover.cacheKey() );
    }
    else
    {
        scover = cover.scaled( r.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation );
        m_cache.insert( cover.cacheKey(), scover );
    }
    painter->drawPixmap( r, scover );

    painter->setPen( opt.palette.color( QPalette::Text ) );
    QTextOption to;
    to.setWrapMode( QTextOption::NoWrap );

    QString text;
    QFont font = opt.font;
    QFont boldFont = opt.font;
    boldFont.setBold( true );

    QRect textRect = option.rect.adjusted( 0, option.rect.height() - 32, 0, -2 );

    bool oneLiner = ( textRect.height() / 2 < painter->fontMetrics().boundingRect( item->album()->name() ).height() ||
                      textRect.height() / 2 < painter->fontMetrics().boundingRect( item->album()->artist()->name() ).height() );

    if ( oneLiner )
    {
        to.setAlignment( Qt::AlignHCenter | Qt::AlignVCenter );
        text = painter->fontMetrics().elidedText( item->album()->name(), Qt::ElideRight, textRect.width() - 3 );
        painter->drawText( textRect, text, to );
    }
    else
    {
        to.setAlignment( Qt::AlignHCenter | Qt::AlignTop );
        text = painter->fontMetrics().elidedText( item->album()->name(), Qt::ElideRight, textRect.width() - 3 );
        painter->drawText( textRect, text, to );

        painter->setFont( boldFont );
        to.setAlignment( Qt::AlignHCenter | Qt::AlignBottom );
        text = painter->fontMetrics().elidedText( item->album()->artist()->name(), Qt::ElideRight, textRect.width() - 3 );
        painter->drawText( textRect, text, to );
    }

    painter->restore();
}
