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

#include "query.h"
#include "result.h"

#include "utils/tomahawkutils.h"

#include "treemodelitem.h"
#include "treeproxymodel.h"


TreeItemDelegate::TreeItemDelegate( QAbstractItemView* parent, TreeProxyModel* proxy )
    : QStyledItemDelegate( (QObject*)parent )
    , m_view( parent )
    , m_model( proxy )
{
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
        return QStyledItemDelegate::paint( painter, option, index );
    }

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

    QRect r = option.rect.adjusted( 4, 4, -option.rect.width() + option.rect.height() - 8, -4 );
//    painter->drawPixmap( option.rect.adjusted( 4, 4, -4, -38 ), QPixmap( RESPATH "images/cover-shadow.png" ) );
    painter->drawPixmap( r, item->cover );

    QTextOption to;
    to.setAlignment( Qt::AlignVCenter );
    QFont font = opt.font;
    QFont boldFont = opt.font;
    boldFont.setBold( true );

    r = option.rect.adjusted( option.rect.height(), 6, -4, -option.rect.height() + 22 );
    text = painter->fontMetrics().elidedText( text, Qt::ElideRight, r.width() );
    painter->drawText( r, text, to );

    painter->setFont( boldFont );

    painter->restore();
}
