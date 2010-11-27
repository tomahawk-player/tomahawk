#include "queueview.h"

#include <QDebug>
#include <QTimeLine>
#include <QVBoxLayout>

#include "playlist/queueproxymodel.h"

using namespace Tomahawk;


QueueView::QueueView( QWidget* parent )
    : QWidget( parent )
    , m_prevHeight( 175 )
{
    setMinimumHeight( 25 );
    setMaximumHeight( 25 );
    setLayout( new QVBoxLayout() );

    m_queue = new PlaylistView( this );
    m_queue->setProxyModel( new QueueProxyModel( this ) );

    m_button = new QPushButton();
    m_button->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );
    m_button->setText( tr( "Click to show queue" ) );
    connect( m_button, SIGNAL( clicked() ), SLOT( showQueue() ) );

    layout()->setMargin( 0 );
    layout()->addWidget( m_button );
    layout()->addWidget( m_queue );

    m_queue->hide();
}


QueueView::~QueueView()
{
    qDebug() << Q_FUNC_INFO;
}


void
QueueView::showQueue()
{
    if ( !m_queue->isHidden() )
        return;

    m_button->setText( tr( "Click to hide queue" ) );
    disconnect( m_button, SIGNAL( clicked() ), this, SLOT( showQueue() ) );
    connect( m_button, SIGNAL( clicked() ), SLOT( hideQueue() ) );

    m_queue->setMaximumHeight( 0 );
    m_queue->show();

    QTimeLine *timeLine = new QTimeLine( 300, this );
    timeLine->setFrameRange( 0, m_prevHeight );
    timeLine->setUpdateInterval( 20 );
    connect( timeLine, SIGNAL( frameChanged( int ) ), SLOT( onAnimationStep( int ) ) );
    connect( timeLine, SIGNAL( finished() ), SLOT( onAnimationFinished() ) );
    timeLine->start();
}


void
QueueView::hideQueue()
{
    if ( m_queue->isHidden() )
        return;

    m_button->setText( tr( "Click to show queue" ) );
    disconnect( m_button, SIGNAL( clicked() ), this, SLOT( hideQueue() ) );
    connect( m_button, SIGNAL( clicked() ), SLOT( showQueue() ) );

    m_prevHeight = height() - 25;
    QTimeLine *timeLine = new QTimeLine( 300, this );
    timeLine->setFrameRange( 0, m_prevHeight );
    timeLine->setUpdateInterval( 20 );
    timeLine->setDirection( QTimeLine::Backward );
    connect( timeLine, SIGNAL( frameChanged( int ) ), SLOT( onAnimationStep( int ) ) );
    connect( timeLine, SIGNAL( finished() ), SLOT( onAnimationFinished() ) );
    timeLine->start();
}


void
QueueView::onAnimationStep( int frame )
{
    setUpdatesEnabled( false );
    m_queue->setUpdatesEnabled( false );

    setMinimumHeight( frame + 25 );
    setMaximumHeight( frame + 25 );

    m_queue->setMaximumHeight( frame );
    m_queue->resize( m_queue->width(), frame );

    m_queue->setUpdatesEnabled( true );
    setUpdatesEnabled( true );
}


void
QueueView::onAnimationFinished()
{
    qDebug() << Q_FUNC_INFO << maximumHeight();

    if ( maximumHeight() < 32 )
    {
        setMinimumHeight( 25 );
        setMaximumHeight( 25 );
        m_queue->hide();
    }
    else
    {
        setMinimumHeight( 200 );
        m_queue->setMaximumHeight( QWIDGETSIZE_MAX );
        setMaximumHeight( QWIDGETSIZE_MAX );
    }

    sender()->deleteLater();
}
