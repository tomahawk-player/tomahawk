/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *                        Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include "PipelineStatusItem.h"

#include "utils/TomahawkUtilsGui.h"
#include "Pipeline.h"
#include "Source.h"

#ifndef ENABLE_HEADLESS
#include "JobStatusModel.h"
#include "JobStatusView.h"
#endif


PipelineStatusItem::PipelineStatusItem( const Tomahawk::query_ptr& q )
    : JobStatusItem()
{
    connect( Tomahawk::Pipeline::instance(), SIGNAL( resolving( Tomahawk::query_ptr ) ), this, SLOT( resolving( Tomahawk::query_ptr ) ) );
    connect( Tomahawk::Pipeline::instance(), SIGNAL( idle() ), this, SLOT( idle() ) );

    if ( !q.isNull() )
        resolving( q );
}


PipelineStatusItem::~PipelineStatusItem()
{
}


QString
PipelineStatusItem::rightColumnText() const
{
    return QString( "%1" ).arg( Tomahawk::Pipeline::instance()->activeQueryCount() + Tomahawk::Pipeline::instance()->pendingQueryCount() );
}


QString
PipelineStatusItem::mainText() const
{
    return m_latestQuery;
}


void
PipelineStatusItem::idle()
{
    if ( !Tomahawk::Pipeline::instance()->activeQueryCount() )
        emit finished();
}


QPixmap
PipelineStatusItem::icon() const
{
    return TomahawkUtils::defaultPixmap( TomahawkUtils::Search );
}


void
PipelineStatusItem::resolving( const Tomahawk::query_ptr& query )
{
    if ( query->isFullTextQuery() )
        m_latestQuery = query->fullTextQuery();
    else
        m_latestQuery = QString( "%1 - %2" ).arg( query->artist() ).arg( query->track() );

    if ( m_latestQuery.isEmpty() )
        qDebug() << "EMPTY STRING IN STATUS ITEM:" << query->fullTextQuery() << query->track() << query->artist() << query->album();
    Q_ASSERT( !m_latestQuery.isEmpty() );

    emit statusChanged();
}


PipelineStatusManager::PipelineStatusManager( QObject* parent )
    : QObject( parent )
{
    connect( Tomahawk::Pipeline::instance(), SIGNAL( resolving( Tomahawk::query_ptr ) ), this, SLOT( resolving( Tomahawk::query_ptr ) ) );
}


void
PipelineStatusManager::resolving( const Tomahawk::query_ptr& p )
{
    Q_UNUSED( p );

#ifndef ENABLE_HEADLESS
    if ( m_curItem.isNull() )
    {
        // No current query item and we're resolving something, so show it
        m_curItem = QPointer< PipelineStatusItem >( new PipelineStatusItem( p ) );
        JobStatusView::instance()->model()->addJob( m_curItem.data() );
    }
#endif
}
