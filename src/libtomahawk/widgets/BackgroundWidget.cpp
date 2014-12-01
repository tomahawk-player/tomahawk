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

#include "BackgroundWidget.h"

#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"

#include <QWidget>
#include <QPixmap>
#include <QPaintEvent>
#include <QPainter>

using namespace Tomahawk;


BackgroundWidget::BackgroundWidget( QWidget* parent )
    : QWidget( parent )
    , m_blurred( false )
{
    setAutoFillBackground( false );
    setBackgroundColor( Qt::transparent );
}


BackgroundWidget::~BackgroundWidget()
{
}


void
BackgroundWidget::setBackground( const QPixmap& p, bool blurred, bool blackWhite )
{
    m_blurred = blurred;
    if ( blurred )
    {
        m_background = QPixmap::fromImage( TomahawkUtils::blurred( p.toImage(), p.rect(), 10, false, blackWhite ) );
    }
    else
    {
        m_background = p;
    }

    m_backgroundSlice = QPixmap();
    repaint();
}


void
BackgroundWidget::setBackgroundColor( const QColor& c )
{
    m_backgroundColor = c;
}


void
BackgroundWidget::resizeEvent( QResizeEvent* event )
{
    QWidget::resizeEvent( event );
    m_backgroundSlice = QPixmap();
}


void
BackgroundWidget::paintEvent( QPaintEvent* event )
{
    QPainter painter( this );

    if ( m_backgroundSlice.isNull() && !m_background.isNull() )
    {
        m_backgroundSlice = m_background.scaledToWidth( contentsRect().width(), Qt::SmoothTransformation );
        m_backgroundSlice = m_backgroundSlice.copy( 0, m_backgroundSlice.height() / 2 - contentsRect().height() / 2, m_backgroundSlice.width(), contentsRect().height() );
    }

    if ( !m_backgroundSlice.isNull() )
    {
        painter.drawPixmap( event->rect(), m_backgroundSlice.copy( event->rect() ) );

        if ( m_blurred )
        {
            painter.save();
            painter.setPen( Qt::transparent );
            painter.setBrush( Qt::black );
            painter.setOpacity( 0.25 );
            painter.drawRect( event->rect() );
            painter.restore();
        }
    }
    else
    {
        painter.save();
        painter.setPen( m_backgroundColor );
        painter.setBrush( m_backgroundColor );
        painter.drawRect( event->rect() );
        painter.restore();
    }

    QWidget::paintEvent( event );
}
