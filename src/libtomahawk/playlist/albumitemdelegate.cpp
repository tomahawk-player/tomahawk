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
#include <QDebug>
#include <QPainter>
#include <QAbstractItemView>

#include "query.h"
#include "result.h"

#include "utils/tomahawkutils.h"

#include "playlist/albumitem.h"
#include "playlist/albumproxymodel.h"


AlbumItemDelegate::AlbumItemDelegate( QAbstractItemView* parent, AlbumProxyModel* proxy )
    : QStyledItemDelegate( (QObject*)parent )
    , m_view( parent )
    , m_model( proxy )
{
    m_shadowPixmap = QPixmap( RESPATH "images/cover-shadow.png" );
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

    if ( option.state & QStyle::State_Selected )
    {
        opt.palette.setColor( QPalette::Text, opt.palette.color( QPalette::HighlightedText ) );
    }

    painter->save();
//    painter->setRenderHint( QPainter::Antialiasing );
    painter->setPen( opt.palette.color( QPalette::Text ) );

//    painter->drawPixmap( option.rect.adjusted( 4, 4, -4, -38 ), m_shadowPixmap );
    painter->drawPixmap( option.rect.adjusted( 6, 4, -6, -41 ), item->cover );

    QTextOption to;
    to.setAlignment( Qt::AlignHCenter );
    QFont font = opt.font;
    QFont boldFont = opt.font;
    boldFont.setBold( true );

    painter->drawText( option.rect.adjusted( 0, option.rect.height() - 16, 0, -2 ), item->album()->artist()->name(), to );

    painter->setFont( boldFont );
    painter->drawText( option.rect.adjusted( 0, option.rect.height() - 32, 0, -18 ), item->album()->name(), to );

    painter->restore();
}
