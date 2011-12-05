/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2011, Michael Zanetti <mzanetti@kde.org>
 *   Copyright 2011, Leo Franchi <lfranchi@kde.org>
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

#include "settingslistdelegate.h"
#include "utils/logger.h"

#include <QPainter>
#include <QIcon>
#include <QApplication>

SettingsListDelegate::SettingsListDelegate(QObject *parent) :
    QStyledItemDelegate(parent)
{
}


void SettingsListDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    painter->save();

    QStyleOptionViewItemV4 opt = option;
    initStyleOption( &opt, QModelIndex() );
    qApp->style()->drawControl( QStyle::CE_ItemViewItem, &opt, painter );

#if defined(Q_WS_MAC) || defined(Q_WS_WIN)
    // On mac draw our own selection rect as we don't get one from osx (around the whole icon or around just text)
    if ( opt.state & QStyle::State_Selected )
    {
        painter->setRenderHint( QPainter::Antialiasing );

        QPainterPath p;
        p.addRoundedRect( opt.rect.adjusted( 2, 1, -1, -1 ), 5, 5 );

        QColor fill( 214, 214, 214 );
        QColor border( 107, 107, 107 );
        painter->setPen( border );
        painter->drawPath( p );
        painter->fillPath( p, fill );
    }
#else
    if ( ( option.state & QStyle::State_Selected ) == QStyle::State_Selected )
    {
        painter->setPen( option.palette.color( QPalette::HighlightedText ) );
    }
#endif

    int horizontalOffset = ( option.rect.width() - option.decorationSize.width() ) /2;
    QRect iconRect = option.rect.adjusted( horizontalOffset, 6, -horizontalOffset, -option.rect.height() + 6 + option.decorationSize.height() );
    QPixmap avatar = index.data( Qt::DecorationRole ).value<QIcon>().pixmap( iconRect.size() );
    painter->drawPixmap( iconRect, avatar );

    QRect textRect = option.rect.adjusted( 6, iconRect.height() + 8, -6, 0 );
    QString text = painter->fontMetrics().elidedText( index.data( Qt::DisplayRole ).toString(), Qt::ElideRight, textRect.width() );
    QTextOption to( Qt::AlignHCenter );
    painter->drawText( textRect, text, to );

    painter->restore();
}
