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

#include "playlistitemdelegate.h"

#include <QDebug>
#include <QPainter>

#include "query.h"
#include "result.h"

#include "playlist/plitem.h"
#include "playlist/trackproxymodel.h"
#include "playlist/trackview.h"
#include "playlist/trackheader.h"

#include "utils/tomahawkutils.h"

#define PLAYING_ICON QString( RESPATH "images/now-playing-speaker.png" )


PlaylistItemDelegate::PlaylistItemDelegate( TrackView* parent, TrackProxyModel* proxy )
    : QStyledItemDelegate( (QObject*)parent )
    , m_view( parent )
    , m_model( proxy )
{
    m_nowPlayingIcon = QPixmap( PLAYING_ICON );
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
    return size;
}


QWidget*
PlaylistItemDelegate::createEditor( QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    return 0;
}


void
PlaylistItemDelegate::paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    PlItem* item = m_model->itemFromIndex( m_model->mapToSource( index ) );
    if ( !item || item->query().isNull() )
        return;

    float opacity = 0.0;
    painter->save();
    if ( item->query()->results().count() )
        opacity = item->query()->results().first()->score();

    QColor textcol, bgcol;
    textcol = option.palette.color( QPalette::Foreground );
    bgcol = option.palette.color( QPalette::Background );

    opacity = qMax( (float)0.3, opacity );
    int r = textcol.red(), g = textcol.green(), b = textcol.blue();
    r = opacity * r + ( 1 - opacity ) * bgcol.red();
    g = opacity * g + ( 1 - opacity ) * bgcol.green();
    b = opacity * b + ( 1 - opacity ) * bgcol.blue();
    textcol = QColor( r, g, b );

    if ( item->isPlaying() )
    {
//        painter->setRenderHint( QPainter::Antialiasing );

        {
            QRect r = option.rect.adjusted( 3, 0, 0, 0 );
            if ( m_view->header()->visualIndex( index.column() ) == 0 )
            {
                r.adjust( 0, 0, 0, -3 );
                painter->drawPixmap( r.adjusted( 3, 3, 18 - r.width(), 0 ), m_nowPlayingIcon );
                r.adjust( 22, 0, 0, 3 );
            }

            painter->setPen( option.palette.text().color() );

            QTextOption to( Qt::AlignVCenter );
            QString text = painter->fontMetrics().elidedText( index.data().toString(), Qt::ElideRight, r.width() - 3 );
            painter->drawText( r.adjusted( 0, 1, 0, 0 ), text, to );
        }

//        if ( m_view->header()->visualIndex( index.column() ) == m_view->header()->visibleSectionCount() - 1 )
        {
            QRect r = QRect( 3, option.rect.y() + 1, m_view->viewport()->width() - 6, option.rect.height() - 2 );
            painter->setPen( option.palette.highlight().color() );
            QPen pen = painter->pen();
            pen.setWidth( 1.0 );
            painter->setPen( pen );
            painter->drawRoundedRect( r, 3.0, 3.0 );
        }
    }
    else
    {
        if ( const QStyleOptionViewItem *vioption = qstyleoption_cast<const QStyleOptionViewItem *>(&option))
        {
            QStyleOptionViewItemV4 o( *vioption );
            o.palette.setColor( QPalette::Text, textcol );
            QStyledItemDelegate::paint( painter, o, index );
        }
        else
            QStyledItemDelegate::paint( painter, option, index );
    }

    painter->restore();
}
