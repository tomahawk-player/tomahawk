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

#include "utils/Logger.h"
#include "utils/StyleHelper.h"
#include "utils/TomahawkUtilsGui.h"


static const int s_defaultFontSize = 12;

HeaderLabel::HeaderLabel( QWidget* parent )
    : QLabel( parent )
    , m_parent( parent )
{
    QFont f( font() );
    f.setBold( true );
    f.setPixelSize( s_defaultFontSize );

    setFont( f );
    setFixedHeight( TomahawkUtils::headerHeight() );
}


HeaderLabel::~HeaderLabel()
{
}


QSize
HeaderLabel::sizeHint() const
{
    return QLabel::sizeHint();
}

int
HeaderLabel::defaultFontSize()
{
    return s_defaultFontSize;
}


void
HeaderLabel::mousePressEvent( QMouseEvent* event )
{
    QFrame::mousePressEvent( event );
    m_time.start();
}


void
HeaderLabel::mouseReleaseEvent( QMouseEvent* event )
{
    QFrame::mouseReleaseEvent( event );
    if ( m_time.elapsed() < qApp->doubleClickInterval() )
        emit clicked();
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
