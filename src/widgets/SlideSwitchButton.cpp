/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012 Teo Mrnjavac <teo@kde.org>
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

#include <QPainter>
#include <QPropertyAnimation>
#include <QStyleOptionButton>

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

    m_backCheckedColor = QColor( 167, 183, 211 );
    m_backUncheckedColor = QColor( "#d9d9d9" );

    m_baseColor = m_backUncheckedColor;
    m_textColor = QColor( "#606060" );
    setFocusPolicy( Qt::NoFocus );

    m_knobX = 0.;

    m_textFont = font();
    m_textFont.setBold( true );
    m_textFont.setCapitalization( QFont::AllUppercase );


    connect( this, SIGNAL( toggled( bool ) ),
             this, SLOT( onCheckedStateChanged() ) );
}

QSize
SlideSwitchButton::sizeHint()
{
    QSize size = QPushButton::sizeHint();
    size.rheight() += 6; //margins
    QFontMetrics fm( m_textFont );
    int maxTextLength = qMax( fm.boundingRect( m_checkedText ).width(),
                              fm.boundingRect( m_uncheckedText ).width() );
    size.rwidth() = contentsMargins().left() + contentsMargins().right()
                  + 2 /*a bit of margin*/ + maxTextLength + ( height() - 4 ) * 1.25;
    return size;
}


void
SlideSwitchButton::paintEvent( QPaintEvent* event )
{
    QPainter painter( this );

    painter.initFrom( this );
    painter.setPen( m_baseColor );

    QStyleOptionButton option;
    option.initFrom( this );

    QPalette palette;

    if ( option.state &  QStyle::State_MouseOver )
        painter.setPen( palette.color( QPalette::Highlight ) );
    //TODO: should the whole thing be highlighted or just the knob?

    QLinearGradient gradient( 0, 0, 0, 1 );
    gradient.setCoordinateMode( QGradient::ObjectBoundingMode );
    gradient.setColorAt( 0, m_baseColor.lighter( 95 ) );
    gradient.setColorAt( 1, m_baseColor.lighter( 115 ) );


    painter.setBrush( QBrush( gradient ) );
    painter.setRenderHints( QPainter::Antialiasing, true );
    painter.drawRoundedRect( QRect( 0, 0, width() - 0, height() - 0 ),
                             5, 5 );

    //knob
    QRect knobRect( 2, 2, ( height() - 4 ) * 1.25, height() - 4 );
    knobRect.moveTo( QPoint( 2 + m_knobX * ( width() - 4 - knobRect.width() ), knobRect.y() ) );
    QLinearGradient knobGradient( 0, 0, 0, 1 );
    knobGradient.setCoordinateMode( QGradient::ObjectBoundingMode );
    knobGradient.setColorAt( 0, m_backUncheckedColor.lighter( 115 ) );
    knobGradient.setColorAt( 1, m_backUncheckedColor.lighter( 95 ) );
    painter.setBrush( QBrush( knobGradient ) );

    painter.drawRoundedRect( knobRect, 1, 1 );

//    if we ever want to try with primitives...
//    QStyleOptionButton option;
//    option.initFrom( this );
//    option.rect = QRect( m_knobX + 1, 1, ( height() - 2 ) * 1.25, height() - 2 );
//    style()->drawPrimitive( QStyle::PE_PanelButtonBevel, &option, &painter, this );

    //let's draw some text...
    QRect textRect = rect().adjusted( 4, 3, -4, -3 );
    painter.setFont( m_textFont );
    painter.setPen( m_textColor );
    if ( m_knobX == 0. )
    {
        //draw on right
        painter.drawText( textRect, Qt::AlignRight | Qt::AlignVCenter, m_uncheckedText );
    }
    else if ( m_knobX == 1. )
    {
        //draw on left
        painter.drawText( textRect, Qt::AlignLeft | Qt::AlignVCenter, m_checkedText );
    }
    //otherwise don't draw because the knob is being animated

    painter.end();
}

void
SlideSwitchButton::onCheckedStateChanged()
{
    QPropertyAnimation *animation = new QPropertyAnimation( this, "knobX" );
    animation->setDuration( 50 );

    if ( isChecked() )
    {
        animation->setStartValue( 0. );
        animation->setEndValue( 1. );
    }
    else
    {
        animation->setStartValue( 1. );
        animation->setEndValue( 0. );
    }
    animation->start( QAbstractAnimation::DeleteWhenStopped );
}


void
SlideSwitchButton::setBackChecked( bool state )
{
    if ( state != m_backChecked )
    {
        m_backChecked = state;
        QPropertyAnimation *animation = new QPropertyAnimation( this, "baseColor" );
        animation->setDuration( 300 );
        if ( state )
        {
            animation->setStartValue( m_backUncheckedColor );
            animation->setEndValue( m_backCheckedColor );
        }
        else
        {
            animation->setStartValue( m_backCheckedColor );
            animation->setEndValue( m_backUncheckedColor );
        }
        animation->start( QAbstractAnimation::DeleteWhenStopped );
    }
}

bool
SlideSwitchButton::backChecked() const
{
    return m_backChecked;
}
