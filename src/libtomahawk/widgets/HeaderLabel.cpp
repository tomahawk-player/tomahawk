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

#include "HeaderLabel.h"

#include <QDebug>
#include <QPainter>

#define FONT_SIZE 16


HeaderLabel::HeaderLabel( QWidget* parent )
    : QLabel( parent )
    , m_parent( parent )
{
    QFont f( font() );
    f.setBold( true );
    f.setPointSize( 11 );

#ifdef Q_WS_MAC
    f.setPointSize( f.pointSize() - 2 );
#endif

    setFont( f );
    setFixedHeight( sizeHint().height() + 6 );
    qDebug() << "FOOBAR:" << minimumSize();
}


HeaderLabel::~HeaderLabel()
{
}


QSize
HeaderLabel::sizeHint() const
{
    return QLabel::sizeHint();
}


void
HeaderLabel::paintEvent( QPaintEvent* event )
{
    QPainter p( this );
    QRect r = contentsRect();

//    p.setRenderHint( QPainter::Antialiasing );

    QRect upperHalf( 0, 0, r.width(), r.height() / 2 );
    QRect lowerHalf( 0, upperHalf.height(), r.width(), r.height() );
    p.fillRect( upperHalf, QColor( 80, 80, 80 ) );
    p.fillRect( lowerHalf, QColor( 72, 72, 72 ) );

    {
        QColor lineColor( 100, 100, 100 );
        QLine line( 0, 0, r.width(), 0 );
        p.setPen( lineColor );
        p.drawLine( line );
    }
    {
        QColor lineColor( 30, 30, 30 );
        QLine line( 0, r.height() - 1, r.width(), r.height() - 1 );
        p.setPen( lineColor );
        p.drawLine( line );
    }

    r.adjust( 8, 3, -8, -3 );
    p.setPen( Qt::white );
    p.drawText( r, text() );
}
