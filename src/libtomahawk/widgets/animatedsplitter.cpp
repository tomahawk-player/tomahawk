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

#include "animatedsplitter.h"

#include "utils/logger.h"

#define ANIMATION_TIME 400


AnimatedSplitter::AnimatedSplitter( QWidget* parent )
    : QSplitter( parent )
    , m_greedyIndex( 0 )
{
    setHandleWidth( 1 );
}


void
AnimatedSplitter::show( int index, bool animate )
{
    QWidget* w = widget( index );
    emit shown( w, animate );
}


void
AnimatedSplitter::hide( int index, bool animate )
{
    QWidget* w = widget( index );
    emit hidden( w, animate );
}


void
AnimatedSplitter::addWidget( QWidget* widget )
{
    QSplitter::addWidget( widget );
}


void
AnimatedSplitter::addWidget( AnimatedWidget* widget )
{
    QSplitter::addWidget( widget );

    connect( widget, SIGNAL( showWidget() ), SLOT( onShowRequest() ) );
    connect( widget, SIGNAL( hideWidget() ), SLOT( onHideRequest() ) );
    connect( widget, SIGNAL( sizeChanged( QSize) ), SLOT( onSizeChanged( QSize ) ) );

    connect( this, SIGNAL( shown( QWidget*, bool ) ), widget, SLOT( onShown( QWidget*, bool ) ) );
    connect( this, SIGNAL( hidden( QWidget*, bool ) ), widget, SLOT( onHidden( QWidget*, bool ) ) );
}


void
AnimatedSplitter::onShowRequest()
{
    AnimatedWidget* w = (AnimatedWidget*)(sender());
    if ( indexOf( w ) > 0 )
        show( indexOf( w ) );
    else
        qDebug() << "Could not find widget:" << sender();
}


void
AnimatedSplitter::onHideRequest()
{
    AnimatedWidget* w = (AnimatedWidget*)(sender());
    if ( indexOf( w ) > 0 )
        hide( indexOf( w ) );
    else
        qDebug() << "Could not find widget:" << sender();
}


void
AnimatedSplitter::onSizeChanged( const QSize& size )
{
    AnimatedWidget* w = (AnimatedWidget*)(sender());
    int wi = indexOf( w );

    QList< int > sizes;
    for ( int i = 0; i < count(); i ++ )
    {
        int j = 0;

        if ( i == m_greedyIndex )
        {
            j = height() - size.height();
            for ( int x = 0; x < count(); x++ )
            {
                if ( x != i && x != wi )
                    j -= widget( x )->height();
            }
        }
        else if ( i == wi )
        {
            j = size.height();
        }
        else
        {
            j = widget( i )->height();
        }

        sizes << j;
    }

    setSizes( sizes );
}


void
AnimatedSplitter::setGreedyWidget( int index )
{
    if( !widget( index ) )
        return;

    m_greedyIndex = index;

    QSizePolicy policy = widget( m_greedyIndex )->sizePolicy();
    if( orientation() == Qt::Horizontal )
        policy.setHorizontalStretch( 1 );
    else
        policy.setVerticalStretch( 1 );

    widget( m_greedyIndex )->setSizePolicy( policy );

}


QSplitterHandle*
AnimatedSplitter::createHandle()
{
    return new AnimatedSplitterHandle( orientation(), this );
}


AnimatedWidget::AnimatedWidget( AnimatedSplitter* parent )
    : m_parent( parent )
    , m_isHidden( false )
{
    m_timeLine = new QTimeLine( ANIMATION_TIME, this );
    m_timeLine->setUpdateInterval( 20 );
    m_timeLine->setEasingCurve( QEasingCurve::OutCubic );

    connect( m_timeLine, SIGNAL( frameChanged( int ) ), SLOT( onAnimationStep( int ) ) );
    connect( m_timeLine, SIGNAL( finished() ), SLOT( onAnimationFinished() ) );
}


AnimatedWidget::~AnimatedWidget()
{
}


void
AnimatedWidget::onShown( QWidget* widget, bool animated )
{
    if ( widget != this )
        return;

    m_animateForward = true;
    if ( animated )
    {
        if ( m_timeLine->state() == QTimeLine::Running )
            m_timeLine->stop();

        m_timeLine->setFrameRange( height(), sizeHint().height() );
        m_timeLine->setDirection( QTimeLine::Forward );
        m_timeLine->start();
    }
    else
    {
        onAnimationStep( sizeHint().height() );
        onAnimationFinished();
    }

    m_isHidden = false;
}


void
AnimatedWidget::onHidden( QWidget* widget, bool animated )
{
    if ( widget != this )
        return;

    m_animateForward = false;
    int minHeight = hiddenSize().height();

    if ( animated )
    {
        if ( m_timeLine->state() == QTimeLine::Running )
            m_timeLine->stop();

        m_timeLine->setFrameRange( minHeight, height() );
        m_timeLine->setDirection( QTimeLine::Backward );
        m_timeLine->start();
    }
    else
    {
        onAnimationStep( minHeight );
        onAnimationFinished();
    }

    m_isHidden = true;
}


void
AnimatedWidget::onAnimationStep( int frame )
{
    setFixedHeight( frame );

    QSize s( 0, frame ); //FIXME
    emit sizeChanged( s );
}


void
AnimatedWidget::onAnimationFinished()
{
    if ( m_animateForward )
    {
        setMinimumHeight( hiddenSize().height() );
        setMaximumHeight( QWIDGETSIZE_MAX );
    }
    else
    {
        setFixedHeight( hiddenSize().height() );
    }
}
