/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "ToggleButton.h"

#include "widgets/HeaderLabel.h"
#include "utils/stylehelper.h"

#include <QStylePainter>
#include <QStyleOptionButton>


ToggleButton::ToggleButton( QWidget* parent )
    : QPushButton( parent )
{
    QFont f( font() );
    f.setBold( true );
    f.setPixelSize( 11 );
    setFont( f );

    HeaderLabel* hl = new HeaderLabel( (QWidget*)0 );
    setFixedHeight( hl->sizeHint().height() + 8 );
    delete hl;

    setCheckable( true );
    setCursor( Qt::PointingHandCursor );
}


ToggleButton::~ToggleButton()
{
}


void
ToggleButton::setText( const QString& s )
{
    QPushButton::setText( s );
    setFixedWidth( fontMetrics().width( text() ) + 32 );
}


void
ToggleButton::paintEvent( QPaintEvent* event )
{
    QStylePainter p( this );

    p.save();
    QStyleOptionButton cb;
    initStyleOption( &cb );
    QRect r = cb.rect;
    StyleHelper::horizontalHeader( &p, r );
    p.restore();

    p.save();
    p.setRenderHint( QPainter::Antialiasing );
    p.setPen( Qt::white );

    {
        QRect highlightRect( r );
        QSize shrink( 2, 4 );
        QSize hS( highlightRect.size() );
        hS -= shrink;
        highlightRect.setSize( hS );
        highlightRect.translate( 0, 2 );

        if ( isChecked() )
        {
            p.setBrush( StyleHelper::headerHighlightColor() );
        }
        else if ( cb.state & QStyle::State_MouseOver )
        {
            p.setBrush( StyleHelper::headerLowerColor() );
        }
        else
        {
            p.setBrush( StyleHelper::headerUpperColor() );
        }

        p.drawRoundedRect( highlightRect, 10.0, 10.0 );
    }

    QTextOption to( Qt::AlignCenter );
    r.adjust( 8, 0, -8, 0 );
    p.setBrush( StyleHelper::headerTextColor() );
    p.drawText( r, cb.text, to );

    p.restore();
}
