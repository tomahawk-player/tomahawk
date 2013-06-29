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
#include <boost/concept_check.hpp>


StatsGauge::StatsGauge( QWidget* parent )
    : QProgressBar( parent )
    , m_percentage( 0 )
    , m_targetValue( 0 )
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

    QPen pen( TomahawkStyle::HEADER_GAUGE_HIGHLIGHT );
    pen.setWidth( 16 );
    p.setPen( pen );

    int fullCircle = 16 * 360;
    p.drawArc( QRect( 12, 12, gaugeSize.width() - 24, gaugeSize.height() - 24 ),
               4 * 360, (int)( -1.0 * (float)fullCircle * ( invertedAppearance() ? ( 1.0 - m_percentage ) : m_percentage ) ) );

    pen = QPen( TomahawkStyle::HEADER_GAUGE_HIGHLIGHT.darker() );
    pen.setWidth( 6 );
    p.setPen( pen );

    QBrush brush( TomahawkStyle::HEADER_GAUGE_BACKGROUND );
    p.setBrush( brush );
    p.drawEllipse( QRect( 28, 28, gaugeSize.width() - 56, gaugeSize.height() - 56 ) );

    pen = QPen( TomahawkStyle::HEADER_GAUGE_TEXT );
    p.setPen( pen );
    QFont font = p.font();
    font.setWeight( QFont::Black );

    if ( value() <= 999 )
        font.setPixelSize( 60 );
    else
        font.setPixelSize( 44 );

    p.setFont( font );
    QRect textRect( 0, gaugeSize.height() / 2 - 14, gaugeSize.width(), 62 );
    p.drawText( textRect, Qt::AlignCenter, value() > 0 ? QString::number( value() ) : "-" );

    pen = QPen( TomahawkStyle::HEADER_GAUGE_TEXT.darker() );
    p.setPen( pen );
    font = p.font();
    font.setWeight( QFont::Black );
    font.setPixelSize( 16 );
    p.setFont( font );

    textRect = QRect( 0, gaugeSize.height() / 2 - 32, gaugeSize.width(), 20 );
    p.drawText( textRect, Qt::AlignCenter, maximum() > 0 ? tr( "out of %1" ).arg( maximum() ) : "-" );

    if ( !m_text.isEmpty() )
    {
        pen = QPen( TomahawkStyle::HEADER_GAUGE_TEXT );
        p.setPen( pen );
        font = p.font();
        font.setWeight( QFont::DemiBold );
        font.setPixelSize( 16 );
        p.setFont( font );

        QColor figColor( TomahawkStyle::HEADER_GAUGE_LABEL_BACKGROUND );
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
    if ( maximum() == 0 || v == 0 )
        return;
    if ( v == m_targetValue )
        return;

    m_targetValue = v;
    {
        QPropertyAnimation* a = new QPropertyAnimation( (QProgressBar*)this, "value" );
        a->setEasingCurve( QEasingCurve( QEasingCurve::OutQuad ) );
        a->setStartValue( value() > 0 ? value() : 1 );
        a->setEndValue( v );
        a->setDuration( 2000 );

        connect( a, SIGNAL( finished() ), a, SLOT( deleteLater() ) );
        a->start();
    }
    {
        QPropertyAnimation* a = new QPropertyAnimation( (QProgressBar*)this, "percentage" );
        a->setEasingCurve( QEasingCurve( QEasingCurve::OutQuad ) );
        a->setStartValue( (float)0 );
        a->setEndValue( (float)v / (float)maximum() );
        a->setDuration( 2000 );

        connect( a, SIGNAL( finished() ), a, SLOT( deleteLater() ) );
        a->start();
    }
}


void
StatsGauge::setText( const QString& text )
{
    m_text = text;
    repaint();
}


void
StatsGauge::setPercentage( float percentage )
{
    m_percentage = percentage;
    repaint();
}
