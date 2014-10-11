/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2014,      Uwe L. Korn <uwelk@xhochy.com>
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

#include "ClickableLabel.h"

#include <QApplication>
#include <QMouseEvent>
#include <QPainter>


ClickableLabel::ClickableLabel( QWidget* parent )
  : QLabel( parent )
  , m_pressed( false )
  , m_moved( false )
  , m_opacity( 0.5 )
{
    setCursor( Qt::PointingHandCursor );
}


ClickableLabel::~ClickableLabel()
{
}


void ClickableLabel::setOpacity( float opacity )
{
    m_opacity = opacity;
    repaint();
}


void
ClickableLabel::mousePressEvent( QMouseEvent* event )
{
    QLabel::mousePressEvent( event );

    if ( !m_moved )
    {
        m_time.start();

        m_pressed = true;
        m_dragPoint = event->pos();
    }
}


void
ClickableLabel::mouseReleaseEvent( QMouseEvent* event )
{
    QLabel::mouseReleaseEvent( event );

    if ( event->button() == Qt::LeftButton &&
         !m_moved && m_time.elapsed() < qApp->doubleClickInterval() )
    {
        emit clicked();
    }

    m_pressed = false;
    m_moved = false;
}


void
ClickableLabel::mouseMoveEvent( QMouseEvent* event )
{
    if ( m_pressed )
    {
        QPoint delta = m_dragPoint - event->pos();
        if ( abs( delta.y() ) > 3 )
        {
            m_moved = true;
            emit resized( delta );
        }
    }
}


void
ClickableLabel::paintEvent( QPaintEvent* /* event */ )
{
    QPainter painter( this );
    painter.setRenderHint( QPainter::TextAntialiasing );
    painter.setOpacity( m_opacity );

    const QString elidedText = fontMetrics().elidedText( text(), Qt::ElideRight, contentsRect().width() );
    painter.drawText( contentsRect(), alignment(), elidedText );
}
