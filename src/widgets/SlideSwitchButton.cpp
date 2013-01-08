/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012 Teo Mrnjavac <teo@kde.org>
 *   Copyright 2012 Leo Franchi <lfranchi@kde.org>
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

#include "SlideSwitchButton.h"

#include "utils/TomahawkUtils.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPropertyAnimation>
#include <QStyleOptionButton>
#include <QPixmap>

namespace {
    // Width to height ratio (70x20)
    const qreal ASPECT_RATIO = 3.5;
    // Knob is originally 32x20
    const qreal KNOB_ASPECT_RATIO = 1.6;

    // Markers for when to show text, and when to snap to either end during a drag
    const qreal LEFT_THRESHOLD = 0.3;
    const qreal RIGHT_THRESHOLD = 0.7;

    const int ROUNDING_RADIUS = 4;
}

SlideSwitchButton::SlideSwitchButton( QWidget* parent )
    : QPushButton( parent )
    , m_checkedText( tr( "On" ) )
    , m_uncheckedText( tr( "Off" ) )
{
    init();
}

SlideSwitchButton::SlideSwitchButton( const QString& checkedText,
                                      const QString& uncheckedText,
                                      QWidget* parent )
    : QPushButton( parent )
    , m_checkedText( checkedText )
    , m_uncheckedText( uncheckedText )
{
    init();
}

void
SlideSwitchButton::init()
{
    setCheckable( true );
#ifndef Q_OS_MAC
    setMouseTracking( true );
#endif

    m_backCheckedColorTop = QColor( 8, 54, 134 );
    m_backCheckedColorBottom = QColor( 118, 172, 240 );
    m_backUncheckedColorTop = QColor( 128, 128, 128 );
    m_backUncheckedColorBottom = QColor( 179, 179, 179 );

    m_baseColorTop = m_backUncheckedColorTop;
    m_baseColorBottom = m_backUncheckedColorBottom;

    m_textColor = QColor( "#606060" );
    setFocusPolicy( Qt::NoFocus );

    m_knobX = 0.;

    m_textFont = font();
    m_textFont.setBold( true );
    m_textFont.setCapitalization( QFont::AllUppercase );


    connect( this, SIGNAL( toggled( bool ) ),
             this, SLOT( onCheckedStateChanged() ) );

    createKnob();
}

QSize
SlideSwitchButton::sizeHint() const
{
#ifndef Q_OS_MAC
    const QSize size = QPushButton::sizeHint();
#else
    // Don't believe the hype. OS X doesn't play nice.
    const QSize size( 70, 20 );
#endif

    return QSize( ASPECT_RATIO * size.height(), size.height() );
}


void
SlideSwitchButton::mousePressEvent( QMouseEvent* e )
{
    if ( m_knob.rect().translated( m_knobX * ( width() - m_knob.width() ), 0 ).contains( e->pos() ) )
        m_mouseDownPos = e->pos();
    else
        m_mouseDownPos = QPoint();

    QPushButton::mousePressEvent( e );
}


void
SlideSwitchButton::mouseReleaseEvent( QMouseEvent* e )
{
    const int delta = m_mouseDownPos.isNull() ? 0 : e->pos().x() - m_mouseDownPos.x();
    m_mouseDownPos = QPoint();

    // Only act as a real button if the user didn't drag
    if ( qAbs( delta ) < 3 )
    {
        QPushButton::mouseReleaseEvent( e );
        return;
    }

    if ( m_knobX < LEFT_THRESHOLD )
        setChecked( false );
    else if ( m_knobX > RIGHT_THRESHOLD )
        setChecked( true );

    QPropertyAnimation* dragEndAnimation = new QPropertyAnimation( this, "knobX" );
    dragEndAnimation->setDuration( 50 );

    dragEndAnimation->setStartValue( m_knobX );
    dragEndAnimation->setEndValue( isChecked() ? 1 : 0 );
    dragEndAnimation->start( QAbstractAnimation::DeleteWhenStopped );
}


void
SlideSwitchButton::mouseMoveEvent( QMouseEvent* e )
{
    if ( m_mouseDownPos.isNull() )
        return;

    e->accept();

    const int rightEdge = width() - m_knob.width();
    const int delta = e->pos().x() - m_mouseDownPos.x();
    const int knobStart = isChecked() ? rightEdge : 0;
    const int newX = ( knobStart + delta );

    if ( newX < 0 || newX > rightEdge ) // out of bounds
        return;

    m_knobX = newX / (qreal)rightEdge;
    repaint();
}


void
SlideSwitchButton::paintEvent( QPaintEvent* event )
{
    Q_UNUSED( event );

    QPainter painter( this );
    painter.setRenderHint( QPainter::Antialiasing );

    QPalette palette;

    QStyleOptionButton option;
    initStyleOption( &option );

    QLinearGradient gradient( 0, 0, 0, 1 );
    gradient.setCoordinateMode( QGradient::ObjectBoundingMode );
    gradient.setColorAt( 0, m_baseColorTop );
    gradient.setColorAt( 1, m_baseColorBottom );
    painter.setBrush( gradient );

    QPainterPath borderPath;
    const QRect borderRect = QRect( 0, 0, width(), height() );
    borderPath.addRoundedRect( borderRect, ROUNDING_RADIUS, ROUNDING_RADIUS );
    painter.fillPath( borderPath, gradient );

    painter.drawPixmap( m_knobX * ( width() - m_knob.width() ), 0, m_knob );

#ifndef Q_OS_MAC
    if ( option.state &  QStyle::State_MouseOver )
    {
        painter.setBrush( QBrush() );
        painter.setPen( palette.color( QPalette::Highlight ) );
        //TODO: should the whole thing be highlighted or just the knob?
        painter.drawRoundedRect( borderRect, ROUNDING_RADIUS, ROUNDING_RADIUS );
    }
#endif

    if ( LEFT_THRESHOLD < m_knobX && m_knobX < RIGHT_THRESHOLD )
        return;

    //let's draw some text...
    if ( m_baseColorTop == m_backUncheckedColorTop )
        painter.setPen( m_textColor );
    else
        painter.setPen( Qt::white );

    painter.setFont( m_textFont );
    const QRectF textRect( m_knobX < LEFT_THRESHOLD ? m_knob.width() : 0, 0, width() - m_knob.width(), height() );
    painter.drawText( textRect, Qt::AlignCenter, m_knobX < LEFT_THRESHOLD ? m_uncheckedText : m_checkedText );
}

void
SlideSwitchButton::onCheckedStateChanged()
{
    if ( !m_knobAnimation.isNull() )
        m_knobAnimation.data()->stop();

    m_knobAnimation = QPointer<QPropertyAnimation>( new QPropertyAnimation( this, "knobX" ) );
    m_knobAnimation.data()->setDuration( 50 );

    m_knobAnimation.data()->setStartValue( m_knobX );
    m_knobAnimation.data()->setEndValue( isChecked() ? 1 : 0 );

    m_knobAnimation.data()->start( QAbstractAnimation::DeleteWhenStopped );
}


void
SlideSwitchButton::setBackChecked( bool state )
{
    if ( state != m_backChecked )
    {
        if ( !m_backTopAnimation.isNull() )
            m_backTopAnimation.data()->stop();
        if ( !m_backBottomAnimation.isNull() )
            m_backBottomAnimation.data()->stop();

        m_backChecked = state;
        m_backTopAnimation = QPointer<QPropertyAnimation>( new QPropertyAnimation( this, "baseColorTop" ) );
        m_backBottomAnimation = QPointer<QPropertyAnimation>( new QPropertyAnimation( this, "baseColorBottom" ) );
        m_backTopAnimation.data()->setDuration( 300 );
        m_backBottomAnimation.data()->setDuration( 300 );

        m_backTopAnimation.data()->setStartValue( state ? m_backUncheckedColorTop : m_backCheckedColorTop );
        m_backTopAnimation.data()->setEndValue( state ? m_backCheckedColorTop : m_backUncheckedColorTop );
        m_backBottomAnimation.data()->setStartValue( state ? m_backUncheckedColorBottom : m_backCheckedColorBottom );
        m_backBottomAnimation.data()->setEndValue( state ? m_backCheckedColorBottom : m_backUncheckedColorBottom );

        m_backTopAnimation.data()->start( QAbstractAnimation::DeleteWhenStopped );
        m_backBottomAnimation.data()->start( QAbstractAnimation::DeleteWhenStopped );
    }
}

bool
SlideSwitchButton::backChecked() const
{
    return m_backChecked;
}


void
SlideSwitchButton::createKnob()
{
    const qreal knobWidth = sizeHint().height() * KNOB_ASPECT_RATIO;
    m_knob = QPixmap( QSize( knobWidth, sizeHint().height() ) );
    m_knob.fill( Qt::transparent );

    QPainter p( &m_knob );
    p.setRenderHint( QPainter::Antialiasing );

    QLinearGradient gradient( 0, 0, 0, 1 );
    gradient.setCoordinateMode( QGradient::ObjectBoundingMode );
    gradient.setColorAt( 0, Qt::white );
    gradient.setColorAt( 1, QColor( 223, 223, 223 ) );

    p.setBrush( gradient );
    p.setPen( QColor( 152, 152, 152 ) );

    p.drawRoundedRect( m_knob.rect(), ROUNDING_RADIUS-1, ROUNDING_RADIUS-1 );
}

QSize
SlideSwitchButton::minimumSizeHint() const
{
    return sizeHint();
}


void
SlideSwitchButton::setKnobX(qreal x)
{
    m_knobX = x;
    repaint();
}


qreal
SlideSwitchButton::knobX() const
{
    return m_knobX;
}


void
SlideSwitchButton::setBaseColorTop(const QColor& color)
{
    m_baseColorTop = color;
    repaint();
}


QColor
SlideSwitchButton::baseColorTop() const
{
    return m_baseColorTop;
}


void
SlideSwitchButton::setBaseColorBottom(const QColor& color)
{
    m_baseColorBottom = color;
}


QColor
SlideSwitchButton::baseColorBottom() const
{
    return m_baseColorBottom;
}
