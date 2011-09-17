/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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

#include "JobStatusDelegate.h"

#include "JobStatusModel.h"
#include "utils/logger.h"

#include <QPainter>
#include <QApplication>

#define ROW_HEIGHT 20
#define PADDING 2
JobStatusDelegate::JobStatusDelegate( QObject* parent )
    : QStyledItemDelegate ( parent )
{

}

JobStatusDelegate::~JobStatusDelegate()
{

}


void
JobStatusDelegate::paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    QStyleOptionViewItemV4 opt = option;
    initStyleOption( &opt, index );
    QFontMetrics fm( opt.font );

    opt.state &= ~QStyle::State_MouseOver;
    QApplication::style()->drawPrimitive( QStyle::PE_PanelItemViewItem, &opt, painter, opt.widget );

//     painter->drawLine( opt.rect.topLeft(), opt.rect.topRight() );

    painter->setRenderHint( QPainter::Antialiasing );
    const QRect iconRect( PADDING, PADDING + opt.rect.y(), ROW_HEIGHT - 2*PADDING, ROW_HEIGHT - 2*PADDING );
    QPixmap p = index.data( Qt::DecorationRole ).value< QPixmap >();
    p = p.scaled( iconRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation );
    painter->drawPixmap( iconRect, p );

    // draw right column if there is one
    const QString rCol = index.data( JobStatusModel::RightColumnRole ).toString();
    int rightEdge = opt.rect.right();
    if ( !rCol.isEmpty() )
    {
        const int w = fm.width( rCol );
        const QRect rRect( opt.rect.right() - PADDING - w, PADDING + opt.rect.y(), w, opt.rect.height() - 2*PADDING );
        painter->drawText( rRect, Qt::AlignCenter, rCol );

        rightEdge = rRect.left();
    }

    const int mainW = rightEdge - PADDING - iconRect.right();
    QString mainText = index.data( Qt::DisplayRole ).toString();
    mainText = fm.elidedText( mainText, Qt::ElideRight, mainW  );
    painter->drawText( QRect( iconRect.right() + 2*PADDING, PADDING + opt.rect.y(), mainW, opt.rect.height() - 2*PADDING ), Qt::AlignLeft | Qt::AlignVCenter, mainText );
}

QSize
JobStatusDelegate::sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
//     return QStyledItemDelegate::sizeHint( option, index );
    const int w = QStyledItemDelegate::sizeHint ( option, index ).width();
    return QSize( w, ROW_HEIGHT );
}

