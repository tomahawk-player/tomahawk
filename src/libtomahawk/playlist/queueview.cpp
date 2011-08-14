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

#include <QVBoxLayout>

#include "playlist/queueproxymodel.h"
#include "widgets/overlaywidget.h"
#include "utils/logger.h"

using namespace Tomahawk;


QueueView::QueueView( AnimatedSplitter* parent )
    : AnimatedWidget( parent )
{
    setHiddenSize( QSize( 0, 0 ) );
    setLayout( new QVBoxLayout() );

    m_queue = new PlaylistView( this );
    m_queue->setProxyModel( new QueueProxyModel( this ) );
    m_queue->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Ignored );
    m_queue->setFrameShape( QFrame::NoFrame );
    m_queue->setAttribute( Qt::WA_MacShowFocusRect, 0 );
    m_queue->overlay()->setEnabled( false );

    layout()->setMargin( 0 );
    layout()->addWidget( m_queue );
}


QueueView::~QueueView()
{
    qDebug() << Q_FUNC_INFO;
}


void
QueueView::onShown( QWidget* widget, bool animated )
{
    qDebug() << Q_FUNC_INFO << widget;
    if ( widget != this )
        return;

    AnimatedWidget::onShown( widget, animated );
}


void
QueueView::onHidden( QWidget* widget, bool animated )
{
    qDebug() << Q_FUNC_INFO << widget;
    if ( widget != this )
        return;

    AnimatedWidget::onHidden( widget, animated );
}
