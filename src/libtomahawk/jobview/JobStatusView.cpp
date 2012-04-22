/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2011, Leo Franchi <lfranchi@kde.org>
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

#include "JobStatusView.h"

#include "Pipeline.h"
#include "JobStatusModel.h"
#include "JobStatusItem.h"
#include "JobStatusDelegate.h"
#include "PipelineStatusItem.h"
#include "TransferStatusItem.h"
#include "LatchedStatusItem.h"
#include "utils/Logger.h"

#include <QHeaderView>
#include <QVBoxLayout>
#include <QListView>
#include <QAbstractItemModel>

using namespace Tomahawk;


JobStatusView* JobStatusView::s_instance = 0;

JobStatusView::JobStatusView( AnimatedSplitter* parent )
    : AnimatedWidget( parent )
    , m_parent( parent )
    , m_cachedHeight( -1 )
{
    s_instance = this;

    setHiddenSize( QSize( 0, 0 ) );
    setLayout( new QVBoxLayout() );
    m_view = new QListView( this );

    layout()->setMargin( 0 );
    layout()->addWidget( m_view );

    m_view->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    m_view->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    m_view->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Ignored );

    m_view->setFrameShape( QFrame::NoFrame );
    m_view->setAttribute( Qt::WA_MacShowFocusRect, 0 );
    m_view->setUniformItemSizes( false );

#ifndef Q_WS_WIN
    QFont f = font();
    f.setPointSize( f.pointSize() - 1 );
    setFont( f );
#endif

#ifdef Q_WS_MAC
    f.setPointSize( f.pointSize() - 2 );
    setFont( f );
#endif

    new PipelineStatusManager( this );
    new TransferStatusManager( this );
    new LatchedStatusManager( this );
}


void
JobStatusView::setModel( JobStatusModel* m )
{
    m_model = m;
    m_view->setModel( m );
    m_view->setItemDelegate( new JobStatusDelegate( m_view ) );

    connect( m_view->model(), SIGNAL( rowsInserted( QModelIndex, int, int ) ), this, SLOT( checkCount() ) );
    connect( m_view->model(), SIGNAL( rowsRemoved( QModelIndex, int, int ) ), this, SLOT( checkCount() ) );
    connect( m_view->model(), SIGNAL( modelReset() ), this, SLOT( checkCount() ) );
    connect( m_view->model(), SIGNAL( customDelegateJobInserted( int, JobStatusItem* ) ), this, SLOT( customDelegateJobInserted( int, JobStatusItem* ) ) );
    connect( m_view->model(), SIGNAL( customDelegateJobRemoved( int ) ), this, SLOT( customDelegateJobRemoved( int ) ) );
}


void
JobStatusView::customDelegateJobInserted( int row, JobStatusItem* item )
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO;
    if ( !item )
        return;

    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "telling item to create delegate";
    item->createDelegate( m_view );
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "item delegate is " << item->customDelegate();
    m_view->setItemDelegateForRow( row, item->customDelegate() );
}


void
JobStatusView::customDelegateJobRemoved( int row )
{
    m_view->setItemDelegateForRow( row, m_view->itemDelegate() );
}


void
JobStatusView::checkCount()
{
    m_cachedHeight = -1;
    if ( m_view->model()->rowCount() == 0 && !isHidden() )
        emit hideWidget();
    else
        emit sizeHintChanged( sizeHint() );
}


QSize
JobStatusView::sizeHint() const
{
    if ( m_cachedHeight >= 0 )
        return QSize( 0, m_cachedHeight );

    unsigned int y = 0;
    y += m_view->contentsMargins().top() + m_view->contentsMargins().bottom();

    if ( m_view->model()->rowCount() )
    {
        for ( int i = 0; i < m_view->model()->rowCount(); i++ )
        {
            y += m_view->sizeHintForRow( i );
        }
        y += 2; // some padding
    }

    m_cachedHeight = y;
    return QSize( 0, y );
}
