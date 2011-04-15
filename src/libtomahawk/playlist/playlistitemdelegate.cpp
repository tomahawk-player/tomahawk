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

#include "trackmodelitem.h"
#include "trackproxymodel.h"
#include "trackview.h"
#include "trackheader.h"

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
    Q_UNUSED( parent );
    Q_UNUSED( option );
    Q_UNUSED( index );
    return 0;
}


void
PlaylistItemDelegate::paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    TrackModelItem* item = m_model->itemFromIndex( m_model->mapToSource( index ) );
    if ( !item || item->query().isNull() )
        return;

    float opacity = 0.0;
    if ( item->query()->results().count() )
        opacity = item->query()->results().first()->score();

    opacity = qMax( (float)0.3, opacity );
    QColor textColor = TomahawkUtils::alphaBlend( option.palette.color( QPalette::Foreground ), option.palette.color( QPalette::Background ), opacity );

    if ( item->isPlaying() )
    {
//        painter->setRenderHint( QPainter::Antialiasing );
        painter->save();

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

        painter->restore();
    }
    else
    {
        if ( const QStyleOptionViewItem *vioption = qstyleoption_cast<const QStyleOptionViewItem *>(&option))
        {
            QStyleOptionViewItemV4 o( *vioption );
            o.palette.setColor( QPalette::Text, textColor );
            QStyledItemDelegate::paint( painter, o, index );
        }
        else
            QStyledItemDelegate::paint( painter, option, index );
    }
}
