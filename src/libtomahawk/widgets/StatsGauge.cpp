/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "StatsGauge.h"

#include "utils/TomahawkStyle.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"

#include <QEasingCurve>
#include <QPainter>
#include <QPaintEvent>
#include <QPropertyAnimation>
#include <QLayout>
#include <QString>


StatsGauge::StatsGauge( QWidget* parent )
    : QProgressBar( parent )
{
    QProgressBar::setValue( 0 );
    QProgressBar::setMaximum( 0 );

    m_sizeHint = QSize( 200, 240 );
    resize( m_sizeHint );
    setFixedSize( m_sizeHint );
}


void
StatsGauge::paintEvent( QPaintEvent* event )
{
    QPainter p( this );
    p.setRenderHint( QPainter::Antialiasing );
    p.setClipRect( event->rect() );

    QSize gaugeSize = m_sizeHint - QSize( 0, 40 );

    QPen pen( TomahawkStyle::NOW_PLAYING_ITEM.lighter() );
    pen.setWidth( 16 );
    p.setPen( pen );

    int fullCircle = 16 * 360;
    p.drawArc( QRect( 12, 12, gaugeSize.width() - 24, gaugeSize.height() - 24 ), 4*360, (int)( -1.0 * (float)fullCircle * ( 1.0 - (float)value() / (float)maximum() ) ) );

    pen = QPen( TomahawkStyle::NOW_PLAYING_ITEM.darker() );
    pen.setWidth( 6 );
    p.setPen( pen );

    QBrush brush( QColor( "#252020" ) );
    p.setBrush( brush );
    p.drawEllipse( QRect( 28, 28, gaugeSize.width() - 56, gaugeSize.height() - 56 ) );

    pen = QPen( Qt::white );
    p.setPen( pen );
    QFont font = p.font();
    font.setWeight( QFont::Black );
    font.setPixelSize( 60 );
    p.setFont( font );

    QRect textRect( 0, gaugeSize.height() / 2 - 14, gaugeSize.width(), 62 );
    p.drawText( textRect, Qt::AlignCenter, value() > 0 ? QString::number( value() ) : "-" );

    pen = QPen( QColor( "#8b8b8b" ) );
    p.setPen( pen );
    font = p.font();
    font.setWeight( QFont::Black );
    font.setPixelSize( 16 );
    p.setFont( font );

    textRect = QRect( 0, gaugeSize.height() / 2 - 32, gaugeSize.width(), 20 );
    p.drawText( textRect, Qt::AlignCenter, maximum() > 0 ? QString( "out of %1" ).arg( maximum() ) : "-" );

    if ( !m_text.isEmpty() )
    {
        pen = QPen( Qt::white );
        p.setPen( pen );
        font = p.font();
        font.setWeight( QFont::Black );
        font.setPixelSize( 18 );
        p.setFont( font );

        QColor figColor( "#3e3e3e" );
        p.setBrush( figColor );

        QFontMetrics fm( font );
        int textWidth = fm.width( m_text );
        textRect = QRect( m_sizeHint.width() / 2 - ( textWidth / 2 + 12 ), m_sizeHint.height() - 32, textWidth + 24, 28 );

        TomahawkUtils::drawBackgroundAndNumbers( &p, m_text, textRect );
    }
}


void
StatsGauge::setValue( int v )
{
    QPropertyAnimation* a = new QPropertyAnimation( (QProgressBar*)this, "value" );
    a->setEasingCurve( QEasingCurve( QEasingCurve::InOutQuad ) );
    a->setStartValue( value() );
    a->setEndValue( v );
    a->setDuration( 2000 );

    connect( a, SIGNAL( finished() ), a, SLOT( deleteLater() ) );
    a->start();
}


void
StatsGauge::setText( const QString& text )
{
    m_text = text;
    repaint();
}