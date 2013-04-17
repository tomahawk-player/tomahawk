/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2012       Leo Franchi            <lfranchi@kde.org>
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

#include "TreeItemDelegate.h"

#include <QApplication>
#include <QPainter>
#include <QAbstractItemView>
#include <QHeaderView>
#include <QMouseEvent>

#include "Query.h"
#include "Result.h"

#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"
#include "utils/Closure.h"
#include "utils/PixmapDelegateFader.h"

#include "PlayableItem.h"
#include "TreeProxyModel.h"
#include "TreeView.h"
#include "ViewManager.h"
#include "Typedefs.h"


TreeItemDelegate::TreeItemDelegate( TreeView* parent, TreeProxyModel* proxy )
    : QStyledItemDelegate( (QObject*)parent )
    , m_view( parent )
    , m_model( proxy )
{
}


QSize
TreeItemDelegate::sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    QSize size;

    if ( index.isValid() )
    {
        Tomahawk::ModelTypes type = (Tomahawk::ModelTypes)index.data( PlayableProxyModel::TypeRole ).toInt();
        switch ( type )
        {
            case Tomahawk::TypeAlbum:
            {
                size.setHeight( option.fontMetrics.height() * 3 );
                return size;
            }

            case Tomahawk::TypeQuery:
            case Tomahawk::TypeResult:
            {
                size.setHeight( option.fontMetrics.height() * 1.6 );
                return size;
            }

            default:
                break;
        }
    }

    // artist per default
    size.setHeight( option.fontMetrics.height() * 4 );
    return size;
}


void
TreeItemDelegate::paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    PlayableItem* item = m_model->sourceModel()->itemFromIndex( m_model->mapToSource( index ) );
    if ( !item )
        return;

    QTextOption textOption( Qt::AlignVCenter | (Qt::Alignment)index.data( Qt::TextAlignmentRole ).toUInt() );
    textOption.setWrapMode( QTextOption::NoWrap );

    QString text;
    if ( !item->artist().isNull() )
    {
        text = item->artist()->name();
    }
    else if ( !item->album().isNull() )
    {
        text = item->album()->name();
    }
    else if ( !item->result().isNull() || !item->query().isNull() )
    {
        float opacity = item->result().isNull() ? 0.0 : item->result()->score();
        opacity = qMax( (float)0.3, opacity );
        QColor textColor = TomahawkUtils::alphaBlend( option.palette.color( QPalette::Foreground ), option.palette.color( QPalette::Background ), opacity );

        {
            QStyleOptionViewItemV4 o = option;
            initStyleOption( &o, QModelIndex() );

            painter->save();
            o.palette.setColor( QPalette::Text, textColor );

            if ( o.state & QStyle::State_Selected && o.state & QStyle::State_Active )
            {
                o.palette.setColor( QPalette::Text, o.palette.color( QPalette::HighlightedText ) );
            }

            if ( item->isPlaying() )
            {
                textColor = TomahawkUtils::Colors::NOW_PLAYING_ITEM_TEXT;
                o.palette.setColor( QPalette::Highlight, TomahawkUtils::Colors::NOW_PLAYING_ITEM );
                o.palette.setColor( QPalette::Text, TomahawkUtils::Colors::NOW_PLAYING_ITEM_TEXT );
                o.state |= QStyle::State_Selected;
            }

            int oldX = 0;
            if ( m_view->header()->visualIndex( index.column() ) == 0 )
            {
                oldX = o.rect.x();
                o.rect.setX( 0 );
            }
            qApp->style()->drawControl( QStyle::CE_ItemViewItem, &o, painter );
            if ( oldX > 0 )
                o.rect.setX( oldX );

            if ( m_hoveringOver == index && !index.data().toString().isEmpty() && index.column() == 0 )
            {
                o.rect.setWidth( o.rect.width() - o.rect.height() );
                QRect arrowRect( o.rect.x() + o.rect.width(), o.rect.y() + 1, o.rect.height() - 2, o.rect.height() - 2 );

                QPixmap infoIcon = TomahawkUtils::defaultPixmap( TomahawkUtils::InfoIcon, TomahawkUtils::Original, arrowRect.size() );
                painter->drawPixmap( arrowRect, infoIcon );

                m_infoButtonRects[ index ] = arrowRect;
            }

            {
                QRect r = o.rect.adjusted( 3, 0, 0, 0 );

                // Paint Now Playing Speaker Icon
                if ( item->isPlaying() && m_view->header()->visualIndex( index.column() ) == 0 )
                {
                    const int pixMargin = 1;
                    const int pixHeight = r.height() - pixMargin * 2;
                    QRect npr = r.adjusted( pixMargin, pixMargin, pixHeight - r.width() + pixMargin, -pixMargin );
                    painter->drawPixmap( npr, TomahawkUtils::defaultPixmap( TomahawkUtils::NowPlayingSpeaker, TomahawkUtils::Original, npr.size() ) );
                    r.adjust( pixHeight + 6, 0, 0, 0 );
                }

                painter->setPen( o.palette.text().color() );

                QString text = painter->fontMetrics().elidedText( index.data().toString(), Qt::ElideRight, r.width() - 3 );
                painter->drawText( r.adjusted( 0, 1, 0, 0 ), text, textOption );
            }
            painter->restore();
        }

        return;
    }
    else
        return;

    if ( text.trimmed().isEmpty() )
        text = tr( "Unknown" );

    QStyleOptionViewItemV4 opt = option;
    initStyleOption( &opt, QModelIndex() );
    qApp->style()->drawControl( QStyle::CE_ItemViewItem, &opt, painter );

    if ( option.state & QStyle::State_Selected )
    {
        opt.palette.setColor( QPalette::Text, opt.palette.color( QPalette::HighlightedText ) );
    }

    QRect arrowRect( m_view->viewport()->width() - option.rect.height(), option.rect.y() + 1, option.rect.height() - 2, option.rect.height() - 2 );
    if ( m_hoveringOver.row() == index.row() && m_hoveringOver.parent() == index.parent() )
    {
        QPixmap infoIcon = TomahawkUtils::defaultPixmap( TomahawkUtils::InfoIcon, TomahawkUtils::Original, arrowRect.size() );
        painter->drawPixmap( arrowRect, infoIcon );

        m_infoButtonRects[ index ] = arrowRect;
    }

    if ( index.column() > 0 )
        return;

    painter->save();
    painter->setRenderHint( QPainter::Antialiasing );
    painter->setPen( opt.palette.color( QPalette::Text ) );

    QRect r = option.rect.adjusted( 4, 4, -option.rect.width() + option.rect.height() - 4, -4 );
//    painter->drawPixmap( r, QPixmap( RESPATH "images/cover-shadow.png" ) );

    if ( !m_pixmaps.contains( index ) )
    {
        if ( !item->album().isNull() )
        {
            m_pixmaps.insert( index, QSharedPointer< Tomahawk::PixmapDelegateFader >( new Tomahawk::PixmapDelegateFader( item->album(), r.size(), TomahawkUtils::Original, false ) ) );
            _detail::Closure* closure = NewClosure( m_pixmaps[ index ], SIGNAL( repaintRequest() ), const_cast<TreeItemDelegate*>(this), SLOT( doUpdateIndex( const QPersistentModelIndex& ) ), QPersistentModelIndex( index ) );
            closure->setAutoDelete( false );
        }
        else if ( !item->artist().isNull() )
        {
            m_pixmaps.insert( index, QSharedPointer< Tomahawk::PixmapDelegateFader >( new Tomahawk::PixmapDelegateFader( item->artist(), r.size(), TomahawkUtils::Original, false ) ) );
            _detail::Closure* closure = NewClosure( m_pixmaps[ index ], SIGNAL( repaintRequest() ), const_cast<TreeItemDelegate*>(this), SLOT( doUpdateIndex( const QPersistentModelIndex& ) ), QPersistentModelIndex( index ) );
            closure->setAutoDelete( false );
        }
    }

    const QPixmap cover = m_pixmaps[ index ]->currentPixmap();
    painter->drawPixmap( r, cover );

    r = option.rect.adjusted( option.rect.height(), 6, -4, -option.rect.height() + 22 );
    text = painter->fontMetrics().elidedText( text, Qt::ElideRight, r.width() );
    painter->drawText( r, text, textOption );

    painter->restore();
}


void
TreeItemDelegate::doUpdateIndex( const QPersistentModelIndex& index )
{
    emit updateIndex( index );
}


bool
TreeItemDelegate::editorEvent( QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index )
{
    Q_UNUSED( model );
    Q_UNUSED( option );

    if ( event->type() != QEvent::MouseButtonRelease &&
         event->type() != QEvent::MouseMove &&
         event->type() != QEvent::MouseButtonPress &&
         event->type() != QEvent::Leave )
        return false;

    bool hoveringInfo = false;
    if ( m_infoButtonRects.contains( index ) )
    {
        const QRect infoRect = m_infoButtonRects[ index ];
        const QMouseEvent* ev = static_cast< QMouseEvent* >( event );
        hoveringInfo = infoRect.contains( ev->pos() );
    }

    if ( event->type() == QEvent::MouseMove )
    {
        if ( hoveringInfo )
            m_view->setCursor( Qt::PointingHandCursor );
        else
            m_view->setCursor( Qt::ArrowCursor );

        if ( m_hoveringOver != index )
        {
            PlayableItem* item = m_model->sourceModel()->itemFromIndex( m_model->mapToSource( index ) );
            item->requestRepaint();
            m_hoveringOver = index;
            emit updateIndex( m_hoveringOver );
        }

        event->accept();
        return true;
    }

    // reset mouse cursor. we switch to a pointing hand cursor when hovering an info button
    m_view->setCursor( Qt::ArrowCursor );

    if ( hoveringInfo )
    {
        if ( event->type() == QEvent::MouseButtonRelease )
        {
            PlayableItem* item = m_model->sourceModel()->itemFromIndex( m_model->mapToSource( index ) );
            if ( !item )
                return false;

            if ( item->query() )
            {
                ViewManager::instance()->show( item->query()->displayQuery() );
            }
            else if ( item->artist() )
            {
                ViewManager::instance()->show( item->artist() );
            }
            else if ( item->album() )
            {
                ViewManager::instance()->show( item->album() );
            }

            event->accept();
            return true;
        }
        else if ( event->type() == QEvent::MouseButtonPress )
        {
            // Stop the whole item from having a down click action as we just want the info button to be clicked
            event->accept();
            return true;
        }
    }

    return false;
}


void
TreeItemDelegate::resetHoverIndex()
{
    m_hoveringOver = QModelIndex();
    m_infoButtonRects.clear();
}
