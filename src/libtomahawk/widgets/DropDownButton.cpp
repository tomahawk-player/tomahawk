/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2015, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "DropDownButton.h"

#include <QMouseEvent>

#include "Artist.h"
#include "Album.h"
#include "Query.h"
#include "utils/TomahawkStyle.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"

using namespace Tomahawk;


DropDownButton::DropDownButton( QWidget* parent )
    : QComboBox( parent )
{
}


DropDownButton::~DropDownButton()
{
}


void
DropDownButton::paintEvent( QPaintEvent* event )
{
    //    QComboBox::paintEvent( event );
    QPainter p( this );
    drawPrimitive( &p, contentsRect(), currentText() );
}


void
DropDownButton::drawPrimitive( QPainter* p, const QRect& rect, const QString& text )
{
    p->save();

    p->setRenderHint( QPainter::TextAntialiasing );
    QRect r = rect.adjusted( 2, 2, -2, -2 );

    p->setPen( TomahawkStyle::PLAYLIST_BUTTON_BACKGROUND.darker() );
    p->setBrush( TomahawkStyle::PLAYLIST_BUTTON_BACKGROUND );
    p->drawRect( r );

    // paint divider
    p->setPen( TomahawkStyle::PLAYLIST_BUTTON_FOREGROUND );
    p->drawLine( QPoint( r.right() - 24, r.top() + 3 ), QPoint( r.right() - 24, r.bottom() - 3 ) );

    // paint label
    const QFontMetrics fm( p->font() );
    r.adjust( 0, 0, -24, 0 ); // center-align left of the divider
    p->drawText( r, Qt::AlignCenter, fm.elidedText( text, Qt::ElideRight, r.width() ) );

    p->restore();
}


void
DropDownButton::mousePressEvent( QMouseEvent* event )
{
    if ( event->pos().x() > width() - 24 )
    {
        QComboBox::mousePressEvent( event );
        return;
    }

    event->accept();
    emit clicked();
}
