#include "animatedsplitter.h"

#include <QDebug>
#include <QTimeLine>


AnimatedSplitter::AnimatedSplitter( QWidget* parent )
    : QSplitter( parent )
    , m_animateIndex( -1 )
    , m_greedyIndex( -1 )
    , m_greedyHeight( -1 )
{
}


void
AnimatedSplitter::show( int index )
{
    if ( m_greedyIndex < 0 )
        return;

    m_animateIndex = index;

    QWidget* w = widget( index );
    QSize size = w->sizeHint();
    qDebug() << "SizeHint:" << index << w->height() << size;

    m_greedyHeight = widget( m_greedyIndex )->height();

    QTimeLine *timeLine = new QTimeLine( 300, this );
    timeLine->setFrameRange( w->height(), size.height() );
    timeLine->setUpdateInterval( 20 );
    timeLine->setCurveShape( QTimeLine::EaseOutCurve );

    connect( timeLine, SIGNAL( frameChanged( int ) ), SLOT( onAnimationStep( int ) ) );
    connect( timeLine, SIGNAL( finished() ), SLOT( onAnimationFinished() ) );
    timeLine->start();

    emit shown( w );
}


void
AnimatedSplitter::hide( int index )
{
    if ( m_greedyIndex < 0 )
        return;

    m_animateIndex = index;

    QWidget* w = widget( index );
    qDebug() << "SizeHint:" << index << w->height();

    m_greedyHeight = widget( m_greedyIndex )->height();

    QTimeLine *timeLine = new QTimeLine( 300, this );
    timeLine->setFrameRange( 25, w->height() );
    timeLine->setUpdateInterval( 20 );
    timeLine->setDirection( QTimeLine::Backward );
    timeLine->setCurveShape( QTimeLine::EaseOutCurve );

    connect( timeLine, SIGNAL( frameChanged( int ) ), SLOT( onAnimationStep( int ) ) );
    connect( timeLine, SIGNAL( finished() ), SLOT( onAnimationFinished() ) );
    timeLine->start();

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

    sender()->deleteLater();
}
