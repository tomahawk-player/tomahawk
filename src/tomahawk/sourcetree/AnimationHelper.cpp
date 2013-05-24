/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2011, Michael Zanetti <mzanetti@kde.org>
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

#include "AnimationHelper.h"

#include <QDebug>

AnimationHelper::AnimationHelper( const QModelIndex& index, QObject *parent )
    :QObject( parent )
    , m_index( index )
    , m_fullyExpanded( false )
    , m_expandAnimation( 0 )
{
    m_expandTimer.setSingleShot( true );
    m_expandTimer.setInterval( 600 );
    connect( &m_expandTimer, SIGNAL( timeout() ), SLOT(expandTimeout() ) );

    m_collapseTimer.setSingleShot( true );
    m_collapseTimer.setInterval( 600 );
    connect( &m_collapseTimer, SIGNAL( timeout() ), SLOT(collapseTimeout() ) );
}


bool
AnimationHelper::initialized() const
{
    return m_expandAnimation != 0;
}


void
AnimationHelper::initialize( const QSize& startValue, const QSize& endValue, int duration )
{
    m_size = startValue;
    m_targetSize = endValue;
    m_startSize = startValue;

    m_expandAnimation = new QPropertyAnimation( this, "size", this );
    m_expandAnimation->setStartValue( startValue );
    m_expandAnimation->setEndValue( endValue );
    m_expandAnimation->setDuration( duration );
    m_expandAnimation->setEasingCurve( QEasingCurve::OutExpo );
    qDebug() << "starting animation" << startValue << endValue << duration;
    connect( m_expandAnimation, SIGNAL( finished() ), SLOT( expandAnimationFinished() ) );

    m_collapseAnimation= new QPropertyAnimation( this, "size", this );
    m_collapseAnimation->setStartValue( endValue );
    m_collapseAnimation->setEndValue( startValue );
    m_collapseAnimation->setDuration( duration );
    m_collapseAnimation->setEasingCurve( QEasingCurve::InExpo );
    connect( m_collapseAnimation, SIGNAL( finished() ), SLOT( collapseAnimationFinished() ) );

}


void
AnimationHelper::setSize( const QSize& size )
{
    m_size = size;
    emit sizeChanged();
    //qDebug() << "animaton setting size to" << size;
}


void
AnimationHelper::expand()
{
    m_collapseTimer.stop();
    m_expandTimer.start();
}


void
AnimationHelper::collapse( bool immediately )
{
    if ( m_expandTimer.isActive() )
    {
        m_expandTimer.stop();
        emit finished( m_index );
        return;
    }

    if ( immediately )
    {
        m_fullyExpanded = false;
        m_collapseAnimation->start();
    }
    else
        m_collapseTimer.start();
}


bool
AnimationHelper::partlyExpanded()
{
    return m_size != m_startSize;
//    return m_fullyExpanded
//            || ( m_expandAnimation->state() == QPropertyAnimation::Running && m_expandAnimation->currentTime() > 250 )
//            || ( m_collapseAnimation->state() == QPropertyAnimation::Running && m_collapseAnimation->currentTime() < 100 );
}


bool
AnimationHelper::fullyExpanded()
{
    return m_fullyExpanded;
}


void
AnimationHelper::expandTimeout()
{
    m_expandAnimation->start();
}


void
AnimationHelper::collapseTimeout()
{
    m_fullyExpanded = false;
    m_collapseAnimation->start();
}


void
AnimationHelper::expandAnimationFinished()
{
    m_fullyExpanded = true;
}


void
AnimationHelper::collapseAnimationFinished()
{
    emit finished( m_index );
}


QSize
AnimationHelper::originalSize() const
{
    return m_startSize;
}


QSize
AnimationHelper::size() const
{
    return m_size;
}
