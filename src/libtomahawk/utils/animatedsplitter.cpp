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

#define ANIMATION_TIME 400


AnimatedSplitter::AnimatedSplitter( QWidget* parent )
    : QSplitter( parent )
    , m_animateIndex( -1 )
    , m_greedyIndex( 0 )
{
    setHandleWidth( 1 );
    
    m_timeLine = new QTimeLine( ANIMATION_TIME, this );
    m_timeLine->setUpdateInterval( 5 );
    m_timeLine->setEasingCurve( QEasingCurve::OutBack );

    connect( m_timeLine, SIGNAL( frameChanged( int ) ), SLOT( onAnimationStep( int ) ) );
    connect( m_timeLine, SIGNAL( finished() ), SLOT( onAnimationFinished() ) );
}


void
AnimatedSplitter::show( int index, bool animate )
{
    m_animateIndex = index;

    QWidget* w = widget( index );
    QSize size = w->sizeHint();

    if ( w->height() == size.height() )
        return;

    emit shown( w );
    w->setMaximumHeight( QWIDGETSIZE_MAX );
    qDebug() << "animating to:" << size.height() << "from" << w->height();

    m_animateForward = true;
    if ( animate )
    {
        if ( m_timeLine->state() == QTimeLine::Running )
            m_timeLine->stop();

        m_timeLine->setFrameRange( w->height(), size.height() );
        m_timeLine->setDirection( QTimeLine::Forward );
        m_timeLine->start();
    }
    else
    {
        onAnimationStep( size.height() );
        onAnimationFinished();
    }
}


void
AnimatedSplitter::hide( int index, bool animate )
{
    m_animateIndex = index;

    QWidget* w = widget( index );
    int minHeight = m_sizes.at( index ).height();

    if ( w->height() == minHeight )
        return;

    emit hidden( w );
    w->setMinimumHeight( minHeight );
    qDebug() << "animating to:" << w->height() << "from" << minHeight;

    m_animateForward = false;
    if ( animate )
    {
        if ( m_timeLine->state() == QTimeLine::Running )
            m_timeLine->stop();

        m_timeLine->setFrameRange( minHeight, w->height() );
        m_timeLine->setDirection( QTimeLine::Backward );
        m_timeLine->start();
    }
    else
    {
        onAnimationStep( minHeight );
        onAnimationFinished();
    }
}


void
AnimatedSplitter::addWidget( QWidget* widget )
{
    QSplitter::addWidget( widget );
    m_sizes << widget->minimumSize();
}


void
AnimatedSplitter::addWidget( AnimatedWidget* widget )
{
    qDebug() << Q_FUNC_INFO << widget;
    QSplitter::addWidget( widget );
    m_sizes << widget->hiddenSize();

    connect( widget, SIGNAL( showWidget() ), SLOT( onShowRequest() ) );
    connect( widget, SIGNAL( hideWidget() ), SLOT( onHideRequest() ) );
    connect( widget, SIGNAL( hiddenSizeChanged() ), SLOT( onHiddenSizeChanged() ) );
    connect( this, SIGNAL( shown( QWidget* ) ), widget, SLOT( onShown( QWidget* ) ) );
    connect( this, SIGNAL( hidden( QWidget* ) ), widget, SLOT( onHidden( QWidget* ) ) );
}


void
AnimatedSplitter::onShowRequest()
{
    qDebug() << Q_FUNC_INFO << sender();

    int j = -1;
    for ( int i = 0; i < count(); i ++ )
    {
        if ( widget( i ) == sender() )
        {
            j = i;
            break;
        }
    }

    if ( j > 0 )
        show( j );
    else
        qDebug() << "Could not find widget:" << sender();
}


void
AnimatedSplitter::onHideRequest()
{
    int j = -1;
    for ( int i = 0; i < count(); i ++ )
    {
        if ( widget( i ) == sender() )
        {
            j = i;
            break;
        }
    }

    if ( j > 0 )
        hide( j );
    else
        qDebug() << "Could not find widget:" << sender();
}


void
AnimatedSplitter::onAnimationStep( int frame )
{
    QList< int > sizes;

    for ( int i = 0; i < count(); i ++ )
    {
        int j = 0;

        if ( i == m_greedyIndex )
        {
            j = height() - frame; // FIXME
        }
        else if ( i == m_animateIndex )
        {
            j = frame;
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
AnimatedSplitter::onAnimationFinished()
{
    qDebug() << Q_FUNC_INFO;

    QWidget* w = widget( m_animateIndex );
    if ( m_animateForward )
    {
        w->setMinimumHeight( w->minimumHeight() );
    }
    else
    {
        w->setMaximumHeight( m_sizes.at( m_animateIndex ).height() );
    }

    m_animateIndex = -1;
}

void 
AnimatedSplitter::setGreedyWidget(int index)
{
    m_greedyIndex = index;
    if( !widget( index ) )
        return;
    QSizePolicy policy = widget( m_greedyIndex )->sizePolicy();
    if( orientation() == Qt::Horizontal )
        policy.setHorizontalStretch( 1 );
    else
        policy.setVerticalStretch( 1 );
    widget( m_greedyIndex )->setSizePolicy( policy );
    
}


void
AnimatedSplitter::onHiddenSizeChanged()
{
    AnimatedWidget* w = (AnimatedWidget*)(sender());
    int i = indexOf( w );

    m_sizes.replace( i, w->hiddenSize() );
}


AnimatedWidget::AnimatedWidget( AnimatedSplitter* parent )
    : m_parent( parent )
    , m_isHidden( false )
{
    qDebug() << Q_FUNC_INFO;
}

AnimatedWidget::~AnimatedWidget()
{

}

void
AnimatedWidget::onShown( QWidget* )
{
    qDebug() << Q_FUNC_INFO << this;
    m_isHidden = false;
}


void
AnimatedWidget::onHidden( QWidget* )
{
    qDebug() << Q_FUNC_INFO << this;
    m_isHidden = true;
}
