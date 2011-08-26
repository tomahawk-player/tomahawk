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

#include "stylehelper.h"

#include <QPainter>


QColor StyleHelper::headerUpperColor()
{
    return QColor( 80, 80, 80 );
}

QColor StyleHelper::headerLowerColor()
{
    return QColor( 72, 72, 72 );
}

QColor StyleHelper::headerHighlightColor()
{
    return QColor( "#333" );
}

void StyleHelper::horizontalHeader(QPainter *painter, const QRect &r)
{
    QRect upperHalf( 0, 0, r.width(), r.height() / 2 );
    QRect lowerHalf( 0, upperHalf.height(), r.width(), r.height() );
    painter->fillRect( upperHalf, StyleHelper::headerUpperColor() );
    painter->fillRect( lowerHalf, StyleHelper::headerLowerColor() );

    {
        QColor lineColor( 100, 100, 100 );
        QLine line( 0, 0, r.width(), 0 );
        painter->setPen( lineColor );
        painter->drawLine( line );
    }
    {
        QColor lineColor( 30, 30, 30 );
        QLine line( 0, r.height() - 1, r.width(), r.height() - 1 );
        painter->setPen( lineColor );
        painter->drawLine( line );
    }
}

QColor StyleHelper::headerTextColor()
{
    return Qt::white;
}
