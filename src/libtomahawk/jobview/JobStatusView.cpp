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
#include "AclJobItem.h"
#include "JobStatusModel.h"
#include "JobStatusItem.h"
#include "JobStatusDelegate.h"
#include "utils/Logger.h"
#include "Source.h"

#ifndef ENABLE_HEADLESS
#include "PipelineStatusItem.h"
#include "TransferStatusItem.h"
#include "LatchedStatusItem.h"
#endif

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

#ifndef ENABLE_HEADLESS
    new PipelineStatusManager( this );
    new TransferStatusManager( this );
    new LatchedStatusManager( this );
#endif

    setMouseTracking( true );
    m_view->setMouseTracking( true );
}


void
JobStatusView::setModel( JobStatusSortModel* m )
{
    m_model = m;
    m_view->setModel( m );
    m_view->setItemDelegate( new JobStatusDelegate( m_view ) );

    connect( m_view->model(), SIGNAL( rowsInserted( QModelIndex, int, int ) ), this, SLOT( checkCount() ) );
    connect( m_view->model(), SIGNAL( rowsRemoved( QModelIndex, int, int ) ), this, SLOT( checkCount() ) );
    connect( m_view->model(), SIGNAL( modelReset() ), this, SLOT( checkCount() ) );
    connect( m_view->model(), SIGNAL( customDelegateJobInserted( int, JobStatusItem* ) ), this, SLOT( customDelegateJobInserted( int, JobStatusItem* ) ) );
    connect( m_view->model(), SIGNAL( customDelegateJobRemoved( int ) ), this, SLOT( customDelegateJobRemoved( int ) ) );
    connect( m_view->model(), SIGNAL( refreshDelegates() ), this, SLOT( refreshDelegates() ) );
}


void
JobStatusView::customDelegateJobInserted( int row, JobStatusItem* item )
{
    if ( !item )
        return;

    item->createDelegate( m_view );
    m_view->setItemDelegateForRow( row, item->customDelegate() );
    ACLJobDelegate* delegate = qobject_cast< ACLJobDelegate* >( item->customDelegate() );
    if ( delegate )
    {
        connect( delegate, SIGNAL( update( const QModelIndex& ) ), m_view, SLOT( update( const QModelIndex & ) ) );
        connect( delegate, SIGNAL( aclResult( ACLRegistry::ACL ) ), item, SLOT( aclResult( ACLRegistry::ACL ) ) );
        delegate->emitSizeHintChanged( m_model->index( row, 0 ) );
    }
    else
        tLog() << Q_FUNC_INFO << "delegate was not properly found!";

    checkCount();
}


void
JobStatusView::customDelegateJobRemoved( int row )
{
    Q_UNUSED( row );
    checkCount();
}


void
JobStatusView::refreshDelegates()
{
    int count = m_model->rowCount();
    for ( int i = 0; i < count; i++ )
    {
        QModelIndex index = m_model->index( i, 0 );
        QVariant itemVar = index.data( JobStatusModel::JobDataRole );
        if ( !itemVar.canConvert< JobStatusItem* >() || !itemVar.value< JobStatusItem* >() )
        {
            tLog() << Q_FUNC_INFO << "unable to fetch JobStatusItem* at row" << i;
            continue;
        }
        JobStatusItem* item = itemVar.value< JobStatusItem* >();
        if ( item->hasCustomDelegate() )
            m_view->setItemDelegateForRow( i, item->customDelegate() );
        else
            m_view->setItemDelegateForRow( i, m_view->itemDelegate() );
    }

    checkCount();
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
