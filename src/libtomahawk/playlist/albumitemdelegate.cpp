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

#include "artist.h"
#include "query.h"
#include "result.h"

#include "utils/tomahawkutils.h"
#include "utils/logger.h"

#include "playlist/albumitem.h"
#include "playlist/albumproxymodel.h"
#include <QMouseEvent>
#include <viewmanager.h>


AlbumItemDelegate::AlbumItemDelegate( QAbstractItemView* parent, AlbumProxyModel* proxy )
    : QStyledItemDelegate( (QObject*)parent )
    , m_view( parent )
    , m_model( proxy )
{
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
    painter->setRenderHint( QPainter::Antialiasing );

    if ( !( option.state & QStyle::State_Selected ) )
    {
        QRect shadowRect = option.rect.adjusted( 5, 4, -5, -40 );
        painter->setPen( QColor( 90, 90, 90 ) );
        painter->drawRoundedRect( shadowRect, 0.5, 0.5 );

        QPen shadowPen( QColor( 30, 30, 30 ) );
        shadowPen.setWidth( 0.4 );
        painter->drawLine( shadowRect.bottomLeft() + QPoint( -1, 2 ), shadowRect.bottomRight() + QPoint( 1, 2 ) );

        shadowPen.setColor( QColor( 160, 160, 160 ) );
        painter->setPen( shadowPen );
        painter->drawLine( shadowRect.topLeft() + QPoint( -1, 2 ), shadowRect.bottomLeft() + QPoint( -1, 2 ) );
        painter->drawLine( shadowRect.topRight() + QPoint( 2, 2 ), shadowRect.bottomRight() + QPoint( 2, 2 ) );
        painter->drawLine( shadowRect.bottomLeft() + QPoint( 0, 3 ), shadowRect.bottomRight() + QPoint( 0, 3 ) );

        shadowPen.setColor( QColor( 180, 180, 180 ) );
        painter->setPen( shadowPen );
        painter->drawLine( shadowRect.topLeft() + QPoint( -2, 3 ), shadowRect.bottomLeft() + QPoint( -2, 1 ) );
        painter->drawLine( shadowRect.topRight() + QPoint( 3, 3 ), shadowRect.bottomRight() + QPoint( 3, 1 ) );
        painter->drawLine( shadowRect.bottomLeft() + QPoint( 0, 4 ), shadowRect.bottomRight() + QPoint( 0, 4 ) );
    }

    QPixmap cover = item->cover.isNull() ? m_defaultCover : item->cover;
    QRect r = option.rect.adjusted( 6, 5, -6, -41 );

    if ( option.state & QStyle::State_Selected )
    {
#if defined(Q_WS_MAC) || defined(Q_WS_WIN)
        painter->save();
        painter->setRenderHint( QPainter::Antialiasing );

        QPainterPath border;
        border.addRoundedRect( r.adjusted( -2, -2, 2, 2 ), 3, 3 );
        QPen borderPen( QColor( 86, 170, 243 ) );
        borderPen.setWidth( 5 );
        painter->setPen( borderPen );
        painter->drawPath( border );

        painter->restore();
#else
        opt.palette.setColor( QPalette::Text, opt.palette.color( QPalette::HighlightedText ) );
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

    bool oneLiner = false;
    if ( item->album()->artist().isNull() )
        oneLiner = true;
    else
        oneLiner = ( textRect.height() / 2 < painter->fontMetrics().boundingRect( item->album()->name() ).height() ||
                     textRect.height() / 2 < painter->fontMetrics().boundingRect( item->album()->artist()->name() ).height() );

    if ( oneLiner )
    {
        to.setAlignment( Qt::AlignHCenter | Qt::AlignVCenter );
        text = painter->fontMetrics().elidedText( item->album()->name(), Qt::ElideRight, textRect.width() - 3 );
        painter->drawText( textRect, text, to );
    }
    else
    {
        painter->setFont( boldFont );
        to.setAlignment( Qt::AlignHCenter | Qt::AlignTop );
        text = painter->fontMetrics().elidedText( item->album()->name(), Qt::ElideRight, textRect.width() - 3 );
        painter->drawText( textRect, text, to );

        // If the user is hovering over an artist rect, draw a background so she knows it's clickable
        QRect r = textRect;
        r.setTop( r.bottom() - painter->fontMetrics().height() );
        if ( m_hoveringOver == index )
            TomahawkUtils::drawQueryBackground( painter, opt.palette, r, 1.5 );

#ifdef Q_WS_MAC
        painter->setPen( opt.palette.color( QPalette::Dark ).darker( 200 ) );
#else
        painter->setPen( opt.palette.color( QPalette::Dark ) );
#endif
        to.setAlignment( Qt::AlignHCenter | Qt::AlignBottom );
        text = painter->fontMetrics().elidedText( item->album()->artist()->name(), Qt::ElideRight, textRect.width() - 3 );
        painter->drawText( textRect, text, to );
        // Calculate rect of artist on-hover button click area

        m_artistNameRects[ index ] = r;
    }

    painter->restore();
}

bool
AlbumItemDelegate::editorEvent( QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index )
{
    Q_UNUSED( option );

    if ( event->type() != QEvent::MouseButtonRelease &&
         event->type() != QEvent::MouseMove &&
         event->type() != QEvent::MouseButtonPress )
        return false;

    if ( m_artistNameRects.contains( index ) )
    {
        QMouseEvent* ev = static_cast< QMouseEvent* >( event );
        QRect artistNameRect = m_artistNameRects[ index ];
        if ( artistNameRect.contains( ev->pos() ) )
        {
            if ( event->type() == QEvent::MouseMove )
            {
                if ( m_hoveringOver != index )
                {
                    QModelIndex old = m_hoveringOver;
                    m_hoveringOver = index;
                    emit updateIndex( old );
                    emit updateIndex( index );
                }

                return true;
            }
            else if ( event->type() == QEvent::MouseButtonRelease )
            {
                AlbumItem* item = m_model->sourceModel()->itemFromIndex( m_model->mapToSource( index ) );
                if ( !item || item->album().isNull() || item->album()->artist().isNull() )
                    return false;

                ViewManager::instance()->show( item->album()->artist() );

                return true;
            } else if ( event->type() == QEvent::MouseButtonPress )
            {
                // Stop the whole album from having a down click action as we just want the artist name to be clicked
                return true;
            }
        }
    }

    if ( m_hoveringOver.isValid() )
    {
        QModelIndex old = m_hoveringOver;
        m_hoveringOver = QPersistentModelIndex();
        emit updateIndex( old );
    }

    return false;
}
