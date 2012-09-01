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

#include "OverlayButton.h"

#include <QPainter>
#include <QPropertyAnimation>
#include <QAbstractScrollArea>
#include <QScrollBar>

#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"

#define CORNER_ROUNDNESS 8.0
#define FADING_DURATION 500
#define OPACITY 0.70


OverlayButton::OverlayButton( QWidget* parent )
    : QPushButton( parent ) // this is on purpose!
    , m_opacity( 0.0 )
    , m_parent( parent )
{
    resize( 0, 28 );
    setAttribute( Qt::WA_TranslucentBackground, true );

    setOpacity( m_opacity );

    m_timer.setSingleShot( true );
    connect( &m_timer, SIGNAL( timeout() ), this, SLOT( hide() ) );
}


OverlayButton::~OverlayButton()
{
}


void
OverlayButton::setOpacity( qreal opacity )
{
    m_opacity = opacity;

    if ( m_opacity == 0.00 && !isHidden() )
    {
        QPushButton::hide();
    }
    else if ( m_opacity > 0.00 && isHidden() )
    {
        QPushButton::show();
    }

    repaint();
}


void
OverlayButton::setText( const QString& text )
{
    m_text = text;

    QFont f( font() );
    f.setPointSize( TomahawkUtils::defaultFontSize() + 3 );
    f.setBold( true );

    QFontMetrics fm( f );
    resize( fm.width( text ) + 24, height() );
}


void
OverlayButton::show( int timeoutSecs )
{
    QPropertyAnimation* animation = new QPropertyAnimation( this, "opacity" );
    animation->setDuration( FADING_DURATION );
    animation->setEndValue( 1.0 );
    animation->start();

    if( timeoutSecs > 0 )
        m_timer.start( timeoutSecs * 1000 );
}


void
OverlayButton::hide()
{
    QPropertyAnimation* animation = new QPropertyAnimation( this, "opacity" );
    animation->setDuration( FADING_DURATION );
    animation->setEndValue( 0.00 );
    animation->start();
}


bool
OverlayButton::shown() const
{
    if ( !isEnabled() )
        return false;

    return m_opacity == OPACITY;
}


void
OverlayButton::paintEvent( QPaintEvent* event )
{
    Q_UNUSED( event );

    int scrollBarWidth = 0;
    QAbstractScrollArea* scrollArea = qobject_cast<QAbstractScrollArea*>( m_parent );
    if ( scrollArea && scrollArea->verticalScrollBar()->isVisible() )
        scrollBarWidth = scrollArea->verticalScrollBar()->width();

    QPoint corner( m_parent->contentsRect().width() - width() - scrollBarWidth - 12, m_parent->height() - height() - 12 );
    move( corner );

    QPainter p( this );
    QRect r = contentsRect();

    p.setBackgroundMode( Qt::TransparentMode );
    p.setRenderHint( QPainter::Antialiasing );
    p.setOpacity( m_opacity );

    QPen pen( palette().dark().color(), .5 );
    p.setPen( pen );
    p.setBrush( QColor( 30, 30, 30, 255.0 * OPACITY ) );

    p.drawRoundedRect( r, CORNER_ROUNDNESS, CORNER_ROUNDNESS );

    QTextOption to( Qt::AlignCenter );
    to.setWrapMode( QTextOption::WrapAtWordBoundaryOrAnywhere );

    QFont f( font() );
    f.setPointSize( TomahawkUtils::defaultFontSize() + 3 );
    f.setBold( true );

    p.setFont( f );
    p.setPen( Qt::white );
    p.drawText( r, text(), to );
}
