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

#include "treeitemdelegate.h"

#include <QApplication>
#include <QDebug>
#include <QPainter>
#include <QAbstractItemView>
#include <QHeaderView>

#include "query.h"
#include "result.h"

#include "utils/tomahawkutils.h"

#include "treemodelitem.h"
#include "treeproxymodel.h"
#include "artistview.h"


TreeItemDelegate::TreeItemDelegate( ArtistView* parent, TreeProxyModel* proxy )
    : QStyledItemDelegate( (QObject*)parent )
    , m_view( parent )
    , m_model( proxy )
{
    m_nowPlayingIcon = QPixmap( RESPATH "images/now-playing-speaker.png" );
}


QSize
TreeItemDelegate::sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    QSize size = QStyledItemDelegate::sizeHint( option, index );
    return size;
}


void
TreeItemDelegate::paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    TreeModelItem* item = m_model->sourceModel()->itemFromIndex( m_model->mapToSource( index ) );
    if ( !item )
        return;

    QString text;
    if ( !item->artist().isNull() )
    {
        text = item->artist()->name();
    }
    else if ( !item->album().isNull() )
    {
        text = item->album()->name();
    }
    else if ( !item->result().isNull() )
    {
        float opacity = item->result()->score();
        opacity = qMax( (float)0.3, opacity );
        QColor textColor = TomahawkUtils::alphaBlend( option.palette.color( QPalette::Foreground ), option.palette.color( QPalette::Background ), opacity );

        if ( const QStyleOptionViewItem *vioption = qstyleoption_cast<const QStyleOptionViewItem *>(&option))
        {
            QStyleOptionViewItemV4 o( *vioption );
            o.palette.setColor( QPalette::Text, textColor );

            if ( item->isPlaying() )
            {
                o.palette.setColor( QPalette::Highlight, o.palette.color( QPalette::Mid ) );
                o.palette.setColor( QPalette::Text, o.palette.color( QPalette::HighlightedText ) );
                o.state |= QStyle::State_Selected;
            }

            qApp->style()->drawControl( QStyle::CE_ItemViewItem, &o, painter );

            {
                QRect r = o.rect.adjusted( 3, 0, 0, 0 );

                // Paint Now Playing Speaker Icon
                if ( item->isPlaying() && m_view->header()->visualIndex( index.column() ) == 0 )
                {
                    r.adjust( 0, 0, 0, -3 );
                    painter->drawPixmap( r.adjusted( 3, 1, 18 - r.width(), 1 ), m_nowPlayingIcon );
                    r.adjust( 25, 0, 0, 3 );
                }

                painter->setPen( o.palette.text().color() );

                QTextOption to( Qt::AlignVCenter );
                QString text = painter->fontMetrics().elidedText( index.data().toString(), Qt::ElideRight, r.width() - 3 );
                painter->drawText( r.adjusted( 0, 1, 0, 0 ), text, to );
            }
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

    if ( index.column() > 0 )
        return;

    painter->save();
    painter->setRenderHint( QPainter::Antialiasing );
    painter->setPen( opt.palette.color( QPalette::Text ) );

    QRect r = option.rect.adjusted( 4, 4, -option.rect.width() + option.rect.height() - 4, -4 );
//    painter->drawPixmap( r, QPixmap( RESPATH "images/cover-shadow.png" ) );
    painter->drawPixmap( r, item->cover );

    QTextOption to;
    to.setAlignment( Qt::AlignVCenter );

    r = option.rect.adjusted( option.rect.height(), 6, -4, -option.rect.height() + 22 );
    text = painter->fontMetrics().elidedText( text, Qt::ElideRight, r.width() );
    painter->drawText( r, text, to );

    painter->restore();
}
