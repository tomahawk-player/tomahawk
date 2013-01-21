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

#include "utils/Logger.h"

#include <QPoint>
#include <QTimeLine>
#include <QDebug>
#include <QDateTime>
#include <QApplication>
#include <QHideEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QShowEvent>

AnimatedSpinner::AnimatedSpinner( QWidget* parent )
    : QWidget( parent )
    , m_showHide( new QTimeLine )
    , m_animation( new QTimeLine )
    , m_currentIndex( -1 )
    , m_size( QSize( 0, 0 ) )
{
    init();
}

AnimatedSpinner::AnimatedSpinner( const QSize& size, QWidget *parent )
    : QWidget( parent )
    , m_showHide( new QTimeLine )
    , m_animation( new QTimeLine )
    , m_currentIndex( -1 )
    , m_size( size )
{
    init();
}


AnimatedSpinner::AnimatedSpinner( const QSize& size, bool autoStart )
    : QWidget()
    , m_showHide( new QTimeLine )
    , m_animation( new QTimeLine )
    , m_currentIndex( -1 )
{
    m_pixmap = QPixmap( size );
    m_pixmap.fill( Qt::transparent );

    init();

    if ( autoStart )
        fadeIn();
}


void
AnimatedSpinner::init()
{
    m_autoCenter = true;

    m_showHide->setDuration( 300 );
    m_showHide->setStartFrame( 0 );
    m_showHide->setEndFrame( 100 );
    m_showHide->setUpdateInterval( 20 );

    if ( parentWidget() )
        connect( m_showHide, SIGNAL( frameChanged( int ) ), this, SLOT( update() ) );
    else
        connect( m_showHide, SIGNAL( frameChanged( int ) ), this, SLOT( updatePixmap() ) );

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

    QSize size;
    if ( parentWidget() )
        size = m_size != QSize( 0, 0 ) ? m_size : sizeHint();
    else
        size = m_pixmap.size();

    /// Radius is best-fit line with points (13x13, 2), (28x28, 5), (48x48, 10)
    m_radius = qRound( ( 23. * ( size.width() - 5. ) ) / 100. );
    m_armLength = size.width() / 2 - m_radius;

    /// Arm width is best-fit line with points (13x13, 1), (28x28, 2), (48x48, 5)
    m_armWidth = qRound( ( 116. * size.width() - 781. ) / 1000. );
    m_border = 2;
    m_armRect = QRect( m_radius, 0, m_armLength, m_armWidth );

    hide();
}


void
AnimatedSpinner::paintEvent( QPaintEvent* event )
{
    Q_UNUSED( event );
    if ( m_autoCenter && parentWidget() )
    {
        QPoint center = parentWidget()->contentsRect().center() - QPoint( sizeHint().width() / 2, sizeHint().height() / 2 );

        if ( center != pos() )
        {
            move( center );
            return;
        }
    }

    QPainter p( this );
    drawFrame( &p, rect() );
}


void
AnimatedSpinner::updatePixmap()
{
    Q_ASSERT( !m_pixmap.isNull() );

    QPainter p( &m_pixmap );
    m_pixmap.fill( Qt::transparent );
    drawFrame( &p, m_pixmap.rect() );
    p.end();

    emit requestUpdate();
}


void
AnimatedSpinner::drawFrame( QPainter* p, const QRect& rect )
{
    if ( m_showHide->state() == QTimeLine::Running )
    {
        // showing or hiding
        p->setOpacity( (qreal)m_showHide->currentValue() );
    }

    p->setRenderHint( QPainter::Antialiasing, true );
    p->translate( rect.center() + QPoint( 0, 1 ) ); // center

    const qreal stepRadius = (360 + 2*m_armWidth) / segmentCount();
    p->rotate( stepRadius );

    for (int segment = 0; segment < segmentCount(); ++segment) {
        p->rotate(stepRadius);
        QPainterPath arm;
        arm.addRoundedRect( m_armRect.adjusted( 0, -m_armWidth/2., 0, -m_armWidth/2 ), m_border, m_border );

        p->fillPath( arm, colorForSegment( segment ) );
    }
}


void
AnimatedSpinner::fadeIn()
{
    if ( ( parentWidget() && isVisible() ) || m_animation->state() == QTimeLine::Running )
        return;

    m_animation->start();

    m_showHide->setDirection( QTimeLine::Forward );

    if ( m_showHide->state() != QTimeLine::Running )
        m_showHide->start();

    if ( parentWidget() )
        show();
    else
        updatePixmap();
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
        m_animation->stop();
        if ( parentWidget() )
            hide();
        else
            updatePixmap();
    }
}


QSize
AnimatedSpinner::sizeHint() const
{
    return QSize( 48, 48 );
}


void
AnimatedSpinner::frameChanged( int frame )
{
    if ( m_currentIndex == frame || frame > segmentCount() - 1 )
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

    if ( parentWidget() )
        update();
    else
        updatePixmap();
}


QColor
AnimatedSpinner::colorForSegment( int seg ) const
{
    // Highlight color is 227, 227, 227
    // Base color is      101, 101, 101
    Q_ASSERT( seg < m_colors.size() );
    const int comp = 101 + m_colors[seg] * ( 126 );
    return QColor( comp, comp, comp, 255 );
}


int
AnimatedSpinner::segmentCount() const
{
    return 11;
}


LoadingSpinner::LoadingSpinner( QAbstractItemView* parent )
    : AnimatedSpinner( parent )
    , m_parent( parent )
{
    if ( m_parent->model() )
    {
        connect( m_parent->model(), SIGNAL( loadingStarted() ), SLOT( fadeIn() ), Qt::UniqueConnection );
        connect( m_parent->model(), SIGNAL( loadingFinished() ), SLOT( fadeOut() ), Qt::UniqueConnection );
    }
    connect( m_parent, SIGNAL( modelChanged() ), SLOT( onViewModelChanged() ) );
}


void
LoadingSpinner::onViewModelChanged()
{
    if ( m_parent->model() )
    {
        connect( m_parent->model(), SIGNAL( loadingStarted() ), SLOT( fadeIn() ), Qt::UniqueConnection );
        connect( m_parent->model(), SIGNAL( loadingFinished() ), SLOT( fadeOut() ), Qt::UniqueConnection );
    }
}
