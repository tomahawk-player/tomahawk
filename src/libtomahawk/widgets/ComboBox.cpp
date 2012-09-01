/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2011, Casey Link <unnamedrambler@gmail.com>
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

#include "ComboBox.h"

#include "utils/StyleHelper.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"

#include <QStyle>
#include <QTextOption>
#include <QStylePainter>
#include <QStyleOptionComboBox>


ComboBox::ComboBox( QWidget* parent )
    : QComboBox( parent )
{
}


ComboBox::~ComboBox()
{
}


QSize
ComboBox::sizeHint() const
{
    return QComboBox::sizeHint() + QSize( 8, 0 );
}


void
ComboBox::paintEvent( QPaintEvent* )
{
    QStylePainter p( this );
    p.setPen( palette().color( QPalette::Text ) );
    QStyleOptionComboBox cb;
    initStyleOption( &cb );
    QRect r = cb.rect;
    r.setHeight( TomahawkUtils::defaultFontHeight() + 8 );

    StyleHelper::horizontalHeader( &p, r );

    if ( cb.state & QStyle::State_MouseOver )
    {
        p.save();
        QRect highlightRect( r );
        QSize shrink( 3, 4 );
        QSize hS( highlightRect.size() );
        hS -= shrink;
        highlightRect.setSize( hS );
        highlightRect.translate( 0, 2 );
        p.setRenderHint( QPainter::Antialiasing );
        p.setBrush( StyleHelper::headerHighlightColor() );
        p.drawRoundedRect( highlightRect, 10.0, 10.0 );
        p.restore();
    }

    p.save();
    QTextOption to( Qt::AlignVCenter );
    r.adjust( 8, 0, -8, 0 );
    p.setPen( Qt::white );
    p.setBrush( StyleHelper::headerTextColor() );
    p.drawText( r, cb.currentText, to );

    bool reverse = cb.direction == Qt::RightToLeft;
    int menuButtonWidth = 12;
    int left = !reverse ? r.right() - menuButtonWidth : r.left();
    int right = !reverse ? r.right() : r.left() + menuButtonWidth;
    QRect arrowRect( ( left + right ) / 2 + ( reverse ? 6 : -6 ), r.center().y() - 3, 9, 9 ); //FIXME: no consts please

    QStyleOption arrowOpt = cb;
    arrowOpt.rect = arrowRect;
    StyleHelper::drawArrow( QStyle::PE_IndicatorArrowDown, &p, &arrowOpt );
    p.restore();
}
