#include "animatedsplitter.h"

#include <QDebug>
#include <QTimeLine>

#define ANIMATION_TIME 300


AnimatedSplitter::AnimatedSplitter( QWidget* parent )
    : QSplitter( parent )
    , m_animateIndex( -1 )
    , m_greedyIndex( -1 )
    , m_greedyHeight( -1 )
{
}


void
AnimatedSplitter::show( int index, bool animate )
{
    if ( m_greedyIndex < 0 )
        return;

    m_animateIndex = index;

    QWidget* w = widget( index );
    QSize size = w->sizeHint();
    w->setMaximumHeight( QWIDGETSIZE_MAX );
    qDebug() << "SizeHint:" << index << w->height() << size;

    m_animateForward = true;
    if ( animate )
    {
        m_greedyHeight = widget( m_greedyIndex )->height();

        QTimeLine *timeLine = new QTimeLine( ANIMATION_TIME, this );
        timeLine->setFrameRange( w->height(), size.height() );
        timeLine->setUpdateInterval( 10 );
        timeLine->setCurveShape( QTimeLine::EaseOutCurve );

        connect( timeLine, SIGNAL( frameChanged( int ) ), SLOT( onAnimationStep( int ) ) );
        connect( timeLine, SIGNAL( finished() ), SLOT( onAnimationFinished() ) );
        timeLine->start();
    }
    else
    {
        onAnimationStep( size.height() );
        onAnimationFinished();
    }

    emit shown( w );
}


void
AnimatedSplitter::hide( int index, bool animate )
{
    if ( m_greedyIndex < 0 )
        return;

    m_animateIndex = index;

    QWidget* w = widget( index );
    qDebug() << "SizeHint:" << index << w->height();
    m_greedyHeight = widget( m_greedyIndex )->height();

    m_animateForward = false;
    if ( animate )
    {

        QTimeLine *timeLine = new QTimeLine( ANIMATION_TIME, this );
        timeLine->setFrameRange( 25, w->height() );
        timeLine->setUpdateInterval( 10 );
        timeLine->setDirection( QTimeLine::Backward );
        timeLine->setCurveShape( QTimeLine::EaseOutCurve );

        connect( timeLine, SIGNAL( frameChanged( int ) ), SLOT( onAnimationStep( int ) ) );
        connect( timeLine, SIGNAL( finished() ), SLOT( onAnimationFinished() ) );
        timeLine->start();
    }
    else
    {
        onAnimationStep( 25 );
        onAnimationFinished();
    }

    emit hidden( w );
}


void
AnimatedSplitter::addWidget( QWidget* widget )
{
    QSplitter::addWidget( widget );

    connect( widget, SIGNAL( showWidget() ), SLOT( onShowRequest() ) );
    connect( widget, SIGNAL( hideWidget() ), SLOT( onHideRequest() ) );
    connect( this, SIGNAL( shown( QWidget* ) ), widget, SLOT( onShown( QWidget* ) ) );
    connect( this, SIGNAL( hidden( QWidget* ) ), widget, SLOT( onHidden( QWidget* ) ) );
}


void
AnimatedSplitter::onShowRequest()
{
    qDebug() << Q_FUNC_INFO;

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
}


void
AnimatedSplitter::onHideRequest()
{
    qDebug() << Q_FUNC_INFO;

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
}


void
AnimatedSplitter::onAnimationStep( int frame )
{
    qDebug() << Q_FUNC_INFO << frame;

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

    qDebug() << sizes;
    setSizes( sizes );
}


void
AnimatedSplitter::onAnimationFinished()
{
    qDebug() << Q_FUNC_INFO;

    QWidget* w = widget( m_animateIndex );
    if ( m_animateForward )
        w->setMinimumHeight( 100 );
    else
    {
        w->setMinimumHeight( 25 );
        w->setMaximumHeight( 25 );
    }

    m_animateIndex = -1;

    if ( sender() )
        sender()->deleteLater();
}
