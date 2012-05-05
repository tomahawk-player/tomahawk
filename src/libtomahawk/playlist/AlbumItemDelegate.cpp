/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2011-2012, Leo Franchi            <lfranchi@kde.org>
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
#include <QAbstractItemView>

#include "Artist.h"
#include "Query.h"
#include "Result.h"

#include "utils/TomahawkUtils.h"
#include "utils/Logger.h"
#include "utils/PixmapDelegateFader.h"
#include <utils/Closure.h>

#include "playlist/AlbumItem.h"
#include "playlist/AlbumProxyModel.h"
#include "AlbumView.h"
#include <QMouseEvent>
#include <ViewManager.h>


AlbumItemDelegate::AlbumItemDelegate( QAbstractItemView* parent, AlbumProxyModel* proxy )
    : QStyledItemDelegate( (QObject*)parent )
    , m_view( parent )
    , m_model( proxy )
{
    if ( m_view && m_view->metaObject()->indexOfSignal( "modelChanged()" ) > -1 )
        connect( m_view, SIGNAL( modelChanged() ), this, SLOT( modelChanged() ) );
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

    QRect r = option.rect.adjusted( 6, 5, -6, -41 );

    QString top, bottom;
    if ( !item->album().isNull() )
    {
        top = item->album()->name();
        
        if ( !item->album()->artist().isNull() )
            bottom = item->album()->artist()->name();
    }
    else if ( !item->artist().isNull() )
    {
        top = item->artist()->name();
    }
    else
    {
        top = item->query()->track();
        bottom = item->query()->artist();
    }

    if ( !m_covers.contains( index ) )
    {
        if ( !item->album().isNull() )
        {
            m_covers.insert( index, QSharedPointer< Tomahawk::PixmapDelegateFader >( new Tomahawk::PixmapDelegateFader( item->album(), r.size(), TomahawkUtils::CoverInCase ) ) );
        }
        else if ( !item->artist().isNull() )
        {
            m_covers.insert( index, QSharedPointer< Tomahawk::PixmapDelegateFader >( new Tomahawk::PixmapDelegateFader( item->artist(), r.size(), TomahawkUtils::CoverInCase ) ) );
        }
        else
        {
            m_covers.insert( index, QSharedPointer< Tomahawk::PixmapDelegateFader >( new Tomahawk::PixmapDelegateFader( item->query(), r.size(), TomahawkUtils::CoverInCase ) ) );
        }

        _detail::Closure* closure = NewClosure( m_covers[ index ], SIGNAL( repaintRequest() ), const_cast<AlbumItemDelegate*>(this), SLOT( doUpdateIndex( QPersistentModelIndex ) ), QPersistentModelIndex( index ) );
        closure->setAutoDelete( false );
    }

    const QPixmap cover = m_covers[ index ]->currentPixmap();

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

    painter->drawPixmap( r, cover );

    painter->setPen( opt.palette.color( QPalette::Text ) );
    QTextOption to;
    to.setWrapMode( QTextOption::NoWrap );

    QString text;
    QFont font = opt.font;
    font.setPixelSize( 11 );
    QFont boldFont = font;
    boldFont.setBold( true );

    QRect textRect = option.rect.adjusted( 0, option.rect.height() - 32, 0, -2 );

    painter->setFont( boldFont );
    bool oneLiner = false;
    if ( bottom.isEmpty() )
        oneLiner = true;
    else
        oneLiner = ( textRect.height() / 2 < painter->fontMetrics().boundingRect( top ).height() ||
                     textRect.height() / 2 < painter->fontMetrics().boundingRect( bottom ).height() );

    if ( oneLiner )
    {
        to.setAlignment( Qt::AlignHCenter | Qt::AlignVCenter );
        text = painter->fontMetrics().elidedText( top, Qt::ElideRight, textRect.width() - 3 );
        painter->drawText( textRect, text, to );
    }
    else
    {
        to.setAlignment( Qt::AlignHCenter | Qt::AlignTop );
        text = painter->fontMetrics().elidedText( top, Qt::ElideRight, textRect.width() - 3 );
        painter->drawText( textRect, text, to );

        // If the user is hovering over an artist rect, draw a background so she knows it's clickable
        QRect r = textRect;
        r.setTop( r.bottom() - painter->fontMetrics().height() );
        r.adjust( 4, 0, -4, -1 );
        if ( m_hoveringOver == index )
        {
            TomahawkUtils::drawQueryBackground( painter, opt.palette, r, 1.1 );
            painter->setPen( opt.palette.color( QPalette::HighlightedText ) );
        }
        else
        {
            if ( !( option.state & QStyle::State_Selected ) )
#ifdef Q_WS_MAC
                painter->setPen( opt.palette.color( QPalette::Dark ).darker( 200 ) );
#else
                painter->setPen( opt.palette.color( QPalette::Dark ) );
#endif
        }

        to.setAlignment( Qt::AlignHCenter | Qt::AlignBottom );
        text = painter->fontMetrics().elidedText( bottom, Qt::ElideRight, textRect.width() - 10 );
        painter->drawText( textRect.adjusted( 5, -1, -5, -1 ), text, to );

        // Calculate rect of artist on-hover button click area
        m_artistNameRects[ index ] = r;
    }

    painter->restore();
}


bool
AlbumItemDelegate::editorEvent( QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index )
{
    Q_UNUSED( model );
    Q_UNUSED( option );

    if ( event->type() != QEvent::MouseButtonRelease &&
         event->type() != QEvent::MouseMove &&
         event->type() != QEvent::MouseButtonPress &&
         event->type() != QEvent::Leave )
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

                event->accept();
                return true;
            }
            else if ( event->type() == QEvent::MouseButtonRelease )
            {
                AlbumItem* item = m_model->sourceModel()->itemFromIndex( m_model->mapToSource( index ) );
                if ( !item || item->album().isNull() || item->album()->artist().isNull() )
                    return false;

                ViewManager::instance()->show( item->album()->artist() );

                event->accept();
                return true;
            }
            else if ( event->type() == QEvent::MouseButtonPress )
            {
                // Stop the whole album from having a down click action as we just want the artist name to be clicked
                event->accept();
                return true;
            }
        }
    }

    whitespaceMouseEvent();

    return false;
}


void
AlbumItemDelegate::whitespaceMouseEvent()
{
    if ( m_hoveringOver.isValid() )
    {
        QModelIndex old = m_hoveringOver;
        m_hoveringOver = QPersistentModelIndex();
        emit updateIndex( old );
    }
}


void
AlbumItemDelegate::modelChanged()
{
    m_artistNameRects.clear();
    m_hoveringOver = QPersistentModelIndex();

    if ( AlbumView* view = qobject_cast< AlbumView* >( m_view ) )
        m_model = view->proxyModel();
}


void
AlbumItemDelegate::doUpdateIndex( const QPersistentModelIndex& idx )
{
    if ( !idx.isValid() )
        return;
    emit updateIndex( idx );
}

