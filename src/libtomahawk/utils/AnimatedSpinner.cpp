/*
    Copyright (C) 2012  Leo Franchi <leo.franchi@kdab.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "AnimatedSpinner.h"

#include <QtCore/QPoint>
#include <QTimeLine>
#include <QDebug>
#include <QDateTime>

#include <QtGui/QApplication>
#include <QtGui/QHideEvent>
#include <QtGui/QPainter>
#include <QtGui/QPaintEvent>
#include <QtGui/QShowEvent>

AnimatedSpinner::AnimatedSpinner(QWidget *parent)
    : QWidget(parent)
    , m_showHide( new QTimeLine )
    , m_animation( new QTimeLine )
    , m_currentIndex( -1 )
{
    m_showHide->setDuration( 300 );
    m_showHide->setStartFrame( 0 );
    m_showHide->setEndFrame( 100 );
    m_showHide->setUpdateInterval( 20 );
    connect( m_showHide, SIGNAL( frameChanged( int ) ), this, SLOT( update() ) );
    connect( m_showHide, SIGNAL( finished() ), this, SLOT( hideFinished() ) );

    m_animation->setDuration( 1000 );
    m_animation->setStartFrame( 0 );
    m_animation->setEndFrame( segmentCount() );
    m_animation->setUpdateInterval( 20 );
    m_animation->setLoopCount( 0 );
    m_animation->setDirection( QTimeLine::Forward );
    m_animation->setCurveShape( QTimeLine::LinearCurve );
    connect( m_animation, SIGNAL( frameChanged( int ) ), this, SLOT( frameChanged( int ) ) );

    m_colors.resize( segmentCount() );

    hide();

}


void
AnimatedSpinner::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    if ( parentWidget() )
    {
        QPoint center( ( parentWidget()->width() / 2 ) - ( width() / 2 ), ( parentWidget()->height() / 2 ) - ( height() / 2 ) );
        if ( center != pos() )
        {
            move( center );
            return;
        }
    }

    QPainter p(this);

    if ( m_showHide->state() == QTimeLine::Running )
    {
        // showing or hiding
        p.setOpacity( (qreal)m_showHide->currentValue() );
    }

    const int radius = 10;
    const int armLength = width()/2 - radius;
    const int armWidth = 5;
    const int border = 3;
    const QRectF armRect( radius, 0, armLength, armWidth );

    p.setRenderHint(QPainter::Antialiasing, true);

    p.translate(width() / 2, height() / 2); // center

    const qreal stepRadius = (360 + 2*armWidth) / segmentCount();
    p.rotate( stepRadius );

    for (int segment = 0; segment < segmentCount(); ++segment) {
        p.rotate(stepRadius);
        QPainterPath arm;
        arm.addRoundedRect( armRect.adjusted( 0, -armWidth/2., 0, -armWidth/2 ), border, border );

        p.fillPath( arm, colorForSegment( segment ) );
    }
}


void
AnimatedSpinner::fadeIn()
{
    if ( isVisible() )
        return;

    show();

    m_animation->start();

    m_showHide->setDirection( QTimeLine::Forward );

    if ( m_showHide->state() != QTimeLine::Running )
        m_showHide->start();
}


void
AnimatedSpinner::fadeOut()
{
    m_showHide->setDirection( QTimeLine::Backward );

    if ( m_showHide->state() != QTimeLine::Running )
        m_showHide->start();
}


void
AnimatedSpinner::hideFinished()
{
    if ( m_showHide->direction() == QTimeLine::Backward )
    {
        hide();
        m_animation->stop();
    }
}


QSize
AnimatedSpinner::sizeHint() const
{
    return QSize(48, 48);
}


void
AnimatedSpinner::frameChanged( int frame )
{

    if ( m_currentIndex == frame || frame > segmentCount()-1 )
        return;

    m_currentIndex = frame;

    Q_ASSERT( frame >= 0 && frame < m_colors.size() );

    // calculate colors, save a factor from 1 to 0 behind the current item
    m_colors.fill( -1 );
    int cur = m_currentIndex, running = 0, tailLength = 5;

    while ( m_colors[cur] == -1 )
    {
        if ( running > tailLength )
            m_colors[cur] = 0.; // beyond the tail, draw at base color
        else
            m_colors[cur] = 1. - ((qreal)running/tailLength); // scale from 1 to 0 along tail

        ++running;
        cur = --cur < 0 ? m_colors.size() - 1 : cur;
    }
    update();
}


QColor
AnimatedSpinner::colorForSegment( int seg ) const
{
    // Highlight color is 227, 227, 227
    // Base color is      101, 101, 101
    Q_ASSERT( seg < m_colors.size() );
    const int comp = 101 + m_colors[seg] * ( 126 );
    return QColor(comp, comp, comp, 255);
}


int
AnimatedSpinner::segmentCount() const
{
    return 11;
}
