/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2011 - 2012, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "FadingPixmap.h"

#include <QPainter>

#define ANIMATION_TIME 1000

FadingPixmap::FadingPixmap( QWidget* parent )
    : QLabel( parent )
    , m_fadePct( 100 )
{
//    setCursor( Qt::PointingHandCursor );

    m_timeLine = new QTimeLine( ANIMATION_TIME, this );
    m_timeLine->setUpdateInterval( 20 );
    m_timeLine->setEasingCurve( QEasingCurve::Linear );

    connect( m_timeLine, SIGNAL( frameChanged( int ) ), SLOT( onAnimationStep( int ) ) );
    connect( m_timeLine, SIGNAL( finished() ), SLOT( onAnimationFinished() ) );
}


FadingPixmap::~FadingPixmap()
{
}


void
FadingPixmap::onAnimationStep( int frame )
{
    m_fadePct = (float)frame / 10.0;
    repaint();
}


void
FadingPixmap::onAnimationFinished()
{
    m_oldPixmap = QPixmap();
    repaint();
    
    if ( m_pixmapQueue.count() )
    {
        setPixmap( m_pixmapQueue.takeFirst() );
    }
}


void
FadingPixmap::setPixmap( const QPixmap& pixmap, bool clearQueue )
{
    if ( m_timeLine->state() == QTimeLine::Running )
    {
        if ( clearQueue )
            m_pixmapQueue.clear();

        m_pixmapQueue << pixmap;
        return;
    }

    m_oldPixmap = m_pixmap;
    m_pixmap = pixmap;

    m_timeLine->setFrameRange( 0, 1000 );
    m_timeLine->setDirection( QTimeLine::Forward );
    m_timeLine->start();
}


void
FadingPixmap::mouseReleaseEvent( QMouseEvent* event )
{
    QFrame::mouseReleaseEvent( event );

    emit clicked();
}


void
FadingPixmap::paintEvent( QPaintEvent* event )
{
    Q_UNUSED( event );
    
    QPainter p( this );
    QRect r = contentsRect();

    p.save();
    p.setRenderHint( QPainter::Antialiasing );

    p.setOpacity( float( 100.0 - m_fadePct ) / 100.0 );
    p.drawPixmap( r, m_oldPixmap );

    p.setOpacity( float( m_fadePct ) / 100.0 );
    p.drawPixmap( r, m_pixmap );

    p.restore();
}
