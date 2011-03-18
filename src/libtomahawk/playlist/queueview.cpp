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

#include "queueview.h"

#include <QDebug>
#include <QVBoxLayout>

#include "playlist/queueproxymodel.h"
#include "widgets/overlaywidget.h"

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
    m_queue->overlay()->setEnabled( false );
    
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
