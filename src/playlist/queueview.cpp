#include "queueview.h"

#include <QDebug>
#include <QVBoxLayout>

#include "playlist/queueproxymodel.h"

using namespace Tomahawk;


QueueView::QueueView( QWidget* parent )
    : QWidget( parent )
{
    setMinimumHeight( 27 );
    setLayout( new QVBoxLayout() );

    m_queue = new PlaylistView( this );
    m_queue->setProxyModel( new QueueProxyModel( this ) );
    m_queue->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Ignored );

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

    m_button->setText( tr( "Click to show queue" ) );
    disconnect( m_button, SIGNAL( clicked() ), this, SIGNAL( hideWidget() ) );
    connect( m_button, SIGNAL( clicked() ), SIGNAL( showWidget() ) );
}
