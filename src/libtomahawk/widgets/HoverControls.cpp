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

#include "HoverControls.h"

#include "widgets/ImageButton.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/ImageRegistry.h"
#include "utils/Logger.h"

#include <QWidget>
#include <QPixmap>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>

using namespace Tomahawk;


HoverControls::HoverControls( QWidget* parent )
    : QWidget( parent )
    , m_hovering( false )
{
    setAutoFillBackground( false );
    setMouseTracking( true );
}


HoverControls::~HoverControls()
{
}


void
HoverControls::resizeEvent( QResizeEvent* event )
{
    QWidget::resizeEvent( event );
}


void
HoverControls::paintEvent( QPaintEvent* /* event */ )
{
    QPainter painter( this );
    painter.setRenderHint( QPainter::Antialiasing );
    painter.setOpacity( 0.7 );

    QPen pen = QPen( Qt::white );
    pen.setWidth( 2 );
    painter.setPen( Qt::transparent );
    painter.setBrush( Qt::transparent );

    QRect figRect = contentsRect().adjusted( 18, 2, -18, -2 );
    if ( m_hovering )
    {
        painter.setBrush( Qt::white );
        painter.drawRect( figRect );
    }

    painter.setPen( pen );

    // circles look bad. make it an oval
    const int bulgeWidth = 16;
    // number of pixels to begin, counting inwards from figRect.x() and figRect.width().
    // 0 means start at each end, negative means start inside the rect
    const int offset = 0;

    QPainterPath ppath;
    ppath.moveTo( QPoint( figRect.x() + offset, figRect.y() ) );
    QRect leftArcRect( figRect.x() + offset - bulgeWidth, figRect.y(), 2*bulgeWidth, figRect.height() );
    ppath.arcTo( leftArcRect, 90, 180 );
    painter.drawPath( ppath );

    ppath = QPainterPath();
    ppath.moveTo( figRect.x() + figRect.width() - offset, figRect.y() + figRect.height() );
    leftArcRect = QRect( figRect.x() + figRect.width() - offset - bulgeWidth, figRect.y(), 2*bulgeWidth, figRect.height() );
    ppath.arcTo( leftArcRect, -90, 180 );
    painter.drawPath( ppath );

    ppath = QPainterPath();
    ppath.moveTo( QPoint( figRect.x() + offset + 2, figRect.y() ) );
    ppath.lineTo( QPoint( figRect.x() + figRect.width() - offset - 2, figRect.y() ) );
    painter.drawPath( ppath );

    ppath = QPainterPath();
    ppath.moveTo( QPoint( figRect.x() + offset + 2, figRect.y() + figRect.height() ) );
    ppath.lineTo( QPoint( figRect.x() + figRect.width() - offset - 2, figRect.y() + figRect.height() ) );
    painter.drawPath( ppath );

    QColor col = Qt::white;
    if ( m_hovering )
        col = Qt::black;
    QPixmap px = ImageRegistry::instance()->pixmap( RESPATH "images/play.svg", QSize( 16, 16 ), TomahawkUtils::Original, 1.0, col );
    painter.drawPixmap( figRect.center() - QPoint( 7, 7 ), px );
}


void
HoverControls::mouseReleaseEvent( QMouseEvent* event )
{
    QWidget::mouseReleaseEvent( event );

    if ( event->button() == Qt::LeftButton )
        emit play();
}


void
HoverControls::mouseMoveEvent( QMouseEvent* event )
{
    QWidget::mouseMoveEvent( event );

    if ( !m_hovering )
    {
        m_hovering = true;
        repaint();
    }
}


void
HoverControls::leaveEvent( QEvent* event )
{
    QWidget::leaveEvent( event );

    m_hovering = false;
    repaint();
}
