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


#include "AnimatedCounterLabel.h"


AnimatedCounterLabel::AnimatedCounterLabel(QWidget* parent, Qt::WindowFlags f)
    : QLabel( parent, f )
    , m_displayed( 0 )
    , m_val( 0 )
    , m_oldval( 0 )
    , m_format( "%L1" )
{
    connect( &m_timer, SIGNAL( frameChanged( int ) ), SLOT( frame( int ) ) );
    connect( &m_timer, SIGNAL( finished() ), SLOT( showDiff() ) );
}


void AnimatedCounterLabel::setFormat(const QString& f)
{
    m_format = f;
    setText( m_format.arg( m_displayed ) );
}


void AnimatedCounterLabel::setVisible(bool b)
{
    QLabel::setVisible( b );
    if ( !m_diff.isNull() )
        m_diff.data()->setVisible( b );
}


void AnimatedCounterLabel::frame(int f)
{
    m_displayed = f;
    QLabel::setText( m_format.arg( m_displayed ) );
    QLabel::update();
}


void AnimatedCounterLabel::setVal(unsigned int v)
{
    if( v == m_val )
        return;

    m_oldval = m_val;
    m_val = v;
    m_timer.stop();
    unsigned int dur = 1000;
    unsigned int r = abs( v - m_oldval );

    if( r > 1000 )        dur = 1500;
    else if( r > 10000 )  dur = 2000;
    else if( r > 25000 )  dur = 2250;
    else if( r > 50000 )  dur = 2750;
    else if( r > 100000 ) dur = 3000;
    else if( r > 500000 ) dur = 5000;

    m_timer.setDuration( dur );
    m_timer.setFrameRange( m_displayed, v );
    m_timer.setEasingCurve( QEasingCurve( QEasingCurve::OutCubic ) );
    m_timer.start();
}


void AnimatedCounterLabel::showDiff()
{
    int differ = m_val - m_oldval;
    m_diff = new QLabel( QString("%1 %L2" ).arg( differ > 0 ? "+" : "" )
    .arg( (int)m_val - (int)m_oldval ),
                                 this->parentWidget() );

    m_diff.data()->setStyleSheet( "font-size:9px; color:grey;" );
    m_diff.data()->move( QPoint( this->pos().x(), this->pos().y() ) );
    QPropertyAnimation* a = new QPropertyAnimation( m_diff.data(), "pos" );
    a->setEasingCurve( QEasingCurve( QEasingCurve::InQuad ) );
    a->setStartValue( m_diff.data()->pos() + QPoint( 0, -10 ) );
    a->setEndValue( QPoint( m_diff.data()->pos().x(), m_diff.data()->pos().y() - 25 ) );
    a->setDuration( 1000 );
    //        qDebug() << "ANIMATING DIFF:" << a->startValue() << a->endValue();

    connect( a, SIGNAL( finished() ), m_diff.data(), SLOT( hide() ) );
    connect( a, SIGNAL( finished() ), m_diff.data(), SLOT( deleteLater() ) );
    connect( a, SIGNAL( finished() ), a, SLOT( deleteLater() ) );

    m_diff.data()->show();
    m_diff.data()->setVisible( this->isVisible() );
    a->start();
}
