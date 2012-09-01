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

#include <QApplication>
#include <QPainter>
#include <QMouseEvent>

#include "utils/Logger.h"
#include "utils/StyleHelper.h"
#include "utils/TomahawkUtilsGui.h"


HeaderLabel::HeaderLabel( QWidget* parent )
    : QLabel( parent )
    , m_parent( parent )
    , m_pressed( false )
    , m_moved( false )
{
    QFont f( font() );
    f.setBold( true );
    f.setPointSize( TomahawkUtils::defaultFontSize() );
    setFont( f );

    setFixedHeight( TomahawkUtils::defaultFontHeight() * 1.4 );
    setMouseTracking( true );
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
HeaderLabel::mousePressEvent( QMouseEvent* event )
{
    QFrame::mousePressEvent( event );
    
    if ( !m_moved )
    {
        m_time.start();
        
        m_pressed = true;
        m_dragPoint = event->pos();
    }
}


void
HeaderLabel::mouseReleaseEvent( QMouseEvent* event )
{
    QFrame::mouseReleaseEvent( event );

    if ( !m_moved && m_time.elapsed() < qApp->doubleClickInterval() )
        emit clicked();

    m_pressed = false;
    m_moved = false;
}


void
HeaderLabel::mouseMoveEvent( QMouseEvent* event )
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
HeaderLabel::paintEvent( QPaintEvent* /* event */ )
{
    QPainter p( this );
    QRect r = contentsRect();
    StyleHelper::horizontalHeader( &p, r );

    QTextOption to( alignment() | Qt::AlignVCenter );
    r.adjust( 8, 0, -8, 0 );
    p.setPen( StyleHelper::headerTextColor() );
    p.drawText( r, text(), to );
}
