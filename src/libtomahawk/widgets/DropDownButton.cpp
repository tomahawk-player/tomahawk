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

    setupPainter( &p );

    drawPrimitive( &p, contentsRect(), currentText(), true, true );
}


void
DropDownButton::drawPrimitive( QPainter* p, const QRect& rect, const QString& text, bool hovering, bool itemsAvailable )
{
    p->save();

    setupPainter( p );

    p->setRenderHint( QPainter::TextAntialiasing );
    QRect r = rect.adjusted( 2, 2, -2, -2 );

    QColor bgColor = hovering ? TomahawkStyle::PLAYLIST_BUTTON_HOVER_BACKGROUND : TomahawkStyle::PLAYLIST_BUTTON_BACKGROUND;

    p->setOpacity( 1.0 );
    p->setPen( bgColor );
    p->setBrush( bgColor );
    p->drawRect( r );
    p->setPen( TomahawkStyle::PLAYLIST_BUTTON_FOREGROUND );
    
    int dropdownWidth = 0;

    if ( itemsAvailable )
    {
        // paint divider
        p->drawLine( QPoint( r.right() - 24, r.top() + 3 ), QPoint( r.right() - 24, r.bottom() - 3 ) );

        // paint drop-down arrow
        p->save();
        QPainterPath dropPath;
        dropPath.moveTo( QPointF( r.right() - 14, float(r.top()) + float(r.height()) * 0.5 - 1.5 ) );
        QPointF currentPosition = dropPath.currentPosition();
        dropPath.lineTo( currentPosition.x() + 6, currentPosition.y() );
        dropPath.lineTo( currentPosition.x() + 3, currentPosition.y() + 3 );
        dropPath.closeSubpath();
        p->setPen( TomahawkStyle::PLAYLIST_BUTTON_FOREGROUND );
        p->setBrush( TomahawkStyle::PLAYLIST_BUTTON_FOREGROUND );
        p->setRenderHint( QPainter::Antialiasing, false );
        p->drawPath( dropPath );
        p->restore();

        dropdownWidth = 24;
    }

    // paint label
    const QFontMetrics fm( p->font() );
    r.adjust( 0, 0, -dropdownWidth, 0 ); // center-align left of the divider
    p->drawText( r, Qt::AlignCenter, fm.elidedText( text.toUpper(), Qt::ElideRight, r.width() ) );

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


void
DropDownButton::setupPainter( QPainter* p )
{
    QFont f = p->font();
    f.setPointSize( TomahawkUtils::defaultFontSize() - 1 );
    f.setBold( true );
    p->setFont( f );
}
