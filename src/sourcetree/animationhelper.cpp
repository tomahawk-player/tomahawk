#include "animationhelper.h"

#include "QDebug"

AnimationHelper::AnimationHelper( const QModelIndex& index, QObject *parent )
    :QObject( parent )
    , m_index( index )
    , m_fullyExpanded( false )
    , m_expandAnimation( 0 )
    , m_forceClosing( false )
{
    m_expandTimer.setSingleShot( true );
    m_expandTimer.setInterval( 1000 );
    connect( &m_expandTimer, SIGNAL(timeout()), SLOT(expandTimeout()));

    m_collapseTimer.setSingleShot( true );
    m_collapseTimer.setInterval( 1000 );
    connect( &m_collapseTimer, SIGNAL(timeout()), SLOT(collapseTimeout()));
}

bool AnimationHelper::initialized() const
{
    return m_expandAnimation != 0;
}

void AnimationHelper::initialize( const QSize& startValue, const QSize& endValue, int duration )
{
    m_size = startValue;
    m_targetSize = endValue;
    m_startSize = startValue;

    m_expandAnimation = new QPropertyAnimation( this, "size", this );
    m_expandAnimation->setStartValue( startValue );
    m_expandAnimation->setEndValue( endValue );
    m_expandAnimation->setDuration( duration );
    m_expandAnimation->setEasingCurve( QEasingCurve::OutBounce );
    qDebug() << "starting animation" << startValue << endValue << duration;
    connect( m_expandAnimation, SIGNAL( finished() ), SLOT(expandAnimationFinished()));

    m_collapseAnimation= new QPropertyAnimation( this, "size", this );
    m_collapseAnimation->setStartValue( endValue );
    m_collapseAnimation->setEndValue( startValue );
    m_collapseAnimation->setDuration( duration );
    m_collapseAnimation->setEasingCurve( QEasingCurve::OutBounce );
    connect( m_collapseAnimation, SIGNAL( finished() ), SLOT(collapseAnimationFinished()));

}

void AnimationHelper::setSize( const QSize& size )
{
    m_size = size;
    emit sizeChanged();
    qDebug() << "animaton setting size to" << size;
}

void AnimationHelper::expand()
{
    m_collapseTimer.stop();
    m_expandTimer.start();
}

void AnimationHelper::collapse( bool immediately )
{
    m_fullyExpanded = false;
    m_expandTimer.stop();

    if ( immediately )
    {
        m_forceClosing = true;
        if ( m_size != m_startSize )
            m_collapseAnimation->start();
    }
    else
    {
        if ( m_size != m_startSize )
            m_collapseTimer.start();
    }
}

bool AnimationHelper::partlyExpanded()
{
    if ( m_forceClosing )
        return false;

    return m_fullyExpanded
            || ( m_expandAnimation->state() == QPropertyAnimation::Running && m_expandAnimation->currentTime() > 250 )
            || ( m_collapseAnimation->state() == QPropertyAnimation::Running && m_collapseAnimation->currentTime() < 100 );
}

bool AnimationHelper::fullyExpanded()
{
    return m_fullyExpanded;
}

void AnimationHelper::expandTimeout()
{
    m_expandAnimation->start();
//        m_fullyExpanded = true;
}

void AnimationHelper::collapseTimeout()
{
//        m_size = m_startSize;
        m_fullyExpanded = false;
//        emit finished( m_index );
    m_collapseAnimation->start();
}

void AnimationHelper::expandAnimationFinished()
{
    m_fullyExpanded = true;
}

void AnimationHelper::collapseAnimationFinished()
{
    m_fullyExpanded = false;
    emit finished( m_index );
}
