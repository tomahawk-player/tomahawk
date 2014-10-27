/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2014, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "CaptionLabel.h"

#include <QPainter>

#include "utils/Logger.h"
#include "utils/TomahawkStyle.h"
#include "utils/TomahawkUtilsGui.h"


CaptionLabel::CaptionLabel( QWidget* parent )
    : ClickableLabel( parent )
    , m_parent( parent )
    , m_showCloseButton( false )
{
    setMouseTracking( true );

    setShowCloseButton( m_showCloseButton );
}


CaptionLabel::~CaptionLabel()
{
}


QSize
CaptionLabel::sizeHint() const
{
    QFontMetrics fm( font() );
    return QSize( fm.width( text().toUpper() ), fm.height() + 9 );
}


void
CaptionLabel::paintEvent( QPaintEvent* /* event */ )
{
    QRect r = contentsRect();

    QPainter p( this );
    p.setRenderHint( QPainter::TextAntialiasing );
    p.setPen( Qt::black );
    p.setBrush( Qt::black );

    QTextOption to( alignment() );
    p.setOpacity( 0.8 );
    p.drawText( r.adjusted( 0, 0, 0, -8 ), text().toUpper(), to );
    p.setOpacity( 0.15 );

    if ( m_showCloseButton )
    {
        to.setAlignment( alignment() | Qt::AlignRight );
        p.setOpacity( 0.15 );
        p.drawText( r.adjusted( 0, 0, 0, -8 ), tr( "Close" ).toUpper(), to );
    }

    QRect playBar = r.adjusted( 0, r.height() - 2, 0, 0 );
    playBar.setHeight( 2 );
    p.drawRect( playBar );
}


void
CaptionLabel::setShowCloseButton( bool b )
{
    m_showCloseButton = b;
    if ( m_showCloseButton )
    {
        setCursor( Qt::PointingHandCursor );
    }
    else
    {
        setCursor( Qt::ArrowCursor );
    }
}
