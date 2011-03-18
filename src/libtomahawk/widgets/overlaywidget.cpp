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

#include "overlaywidget.h"

#include <QDebug>
#include <QPainter>
#include <QPropertyAnimation>

#define CORNER_ROUNDNESS 16.0
#define FADING_DURATION 500
#define FONT_SIZE 18
#define OPACITY 0.86


OverlayWidget::OverlayWidget( QWidget* parent )
    : QWidget( parent ) // this is on purpose!
    , m_opacity( 0.00 )
    , m_parent( parent )
{
    resize( 380, 128 );
    setAttribute( Qt::WA_TranslucentBackground, true );

    setOpacity( m_opacity );

    m_timer.setSingleShot( true );
    connect( &m_timer, SIGNAL( timeout() ), this, SLOT( hide() ) );
}


OverlayWidget::~OverlayWidget()
{
}


void
OverlayWidget::setOpacity( qreal opacity )
{
    m_opacity = opacity;

    if ( m_opacity == 0.00 && !isHidden() )
    {
        QWidget::hide();
    }
    else if ( m_opacity > 0.00 && isHidden() )
    {
        QWidget::show();
    }

    repaint();
}


void
OverlayWidget::setText( const QString& text )
{
    m_text = text;
}


void
OverlayWidget::show( int timeoutSecs )
{
    if ( !isEnabled() )
        return;

    QPropertyAnimation* animation = new QPropertyAnimation( this, "opacity" );
    animation->setDuration( FADING_DURATION );
    animation->setEndValue( OPACITY );
    animation->start();

    if( timeoutSecs > 0 )
        m_timer.start( timeoutSecs * 1000 );
}


void
OverlayWidget::hide()
{
    if ( !isEnabled() )
        return;

    QPropertyAnimation* animation = new QPropertyAnimation( this, "opacity" );
    animation->setDuration( FADING_DURATION );
    animation->setEndValue( 0.00 );
    animation->start();
}


bool 
OverlayWidget::shown() const
{
    if ( !isEnabled() )
        return false;

    return m_opacity == OPACITY;
}


void
OverlayWidget::paintEvent( QPaintEvent* event )
{
    QPoint center( ( m_parent->width() - width() ) / 2, ( m_parent->height() - height() ) / 2 );
    move( center );

    QPainter p( this );
    QRect r = contentsRect();

    p.setBackgroundMode( Qt::TransparentMode );
    p.setRenderHint( QPainter::Antialiasing );
    p.setOpacity( m_opacity );

    QPen pen( palette().dark().color(), .5 );
    p.setPen( pen );
    p.setBrush( palette().highlight() );

    p.drawRoundedRect( r, CORNER_ROUNDNESS, CORNER_ROUNDNESS );

    QTextOption to( Qt::AlignCenter );
    to.setWrapMode( QTextOption::WrapAtWordBoundaryOrAnywhere );

    QFont f( font() );
    f.setPixelSize( FONT_SIZE );
    f.setBold( true );

    p.setFont( f );
    p.setPen( palette().highlightedText().color() );
    p.drawText( r.adjusted( 16, 16, -16, -16 ), text(), to );
}
