#include "queueview.h"

#include <QDebug>
#include <QVBoxLayout>

#include "playlist/queueproxymodel.h"

#ifdef Q_WS_MAC
#define MINIMUM_HEIGHT 38
#else
#define MINIMUM_HEIGHT 27
#endif

using namespace Tomahawk;


QueueView::QueueView( AnimatedSplitter* parent )
    : AnimatedWidget( parent )
{
    setHiddenSize( QSize( 0, MINIMUM_HEIGHT ) );
    setLayout( new QVBoxLayout() );

    m_queue = new PlaylistView( this );
    m_queue->setProxyModel( new QueueProxyModel( this ) );
    m_queue->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Ignored );
    m_queue->setFrameShape( QFrame::NoFrame );
    m_queue->setAttribute( Qt::WA_MacShowFocusRect, 0 );
    
    m_button = new QPushButton();
    m_button->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );
    m_button->setText( tr( "Click to show queue" ) );
    connect( m_button, SIGNAL( clicked() ), SIGNAL( showWidget() ) );

    layout()->setMargin( 0 );
    layout()->addWidget( m_queue );
    layout()->addWidget( m_button );
}


QueueView::~QueueView()
{
    qDebug() << Q_FUNC_INFO;
}


void
QueueView::onShown( QWidget* widget )
{
    qDebug() << Q_FUNC_INFO << widget;
    if ( widget != this )
        return;
    
    AnimatedWidget::onShown( widget );

    m_button->setText( tr( "Click to hide queue" ) );
    disconnect( m_button, SIGNAL( clicked() ), this, SIGNAL( showWidget() ) );
    connect( m_button, SIGNAL( clicked() ), SIGNAL( hideWidget() ) );
}


void
QueueView::onHidden( QWidget* widget )
{
    qDebug() << Q_FUNC_INFO << widget;
    if ( widget != this )
        return;
    
    AnimatedWidget::onHidden( widget );
    
    m_button->setText( tr( "Click to show queue" ) );
    disconnect( m_button, SIGNAL( clicked() ), this, SIGNAL( hideWidget() ) );
    connect( m_button, SIGNAL( clicked() ), SIGNAL( showWidget() ) );
}
