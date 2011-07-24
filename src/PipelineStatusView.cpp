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

#include "PipelineStatusView.h"

#include <QHeaderView>
#include <QVBoxLayout>

#include "libtomahawk/pipeline.h"

#include "utils/logger.h"

using namespace Tomahawk;


PipelineStatusView::PipelineStatusView( AnimatedSplitter* parent )
    : AnimatedWidget( parent )
    , m_parent( parent )
{
    setHiddenSize( QSize( 0, 0 ) );
    setLayout( new QVBoxLayout() );
    m_tree = new QTreeWidget( this );

    layout()->setMargin( 0 );
    layout()->addWidget( m_tree );

    QStringList headers;
    headers << tr( "Searching For" ) << tr( "Pending" );
    m_tree->setHeaderLabels( headers );

    m_tree->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Ignored );
    m_tree->setColumnCount( 2 );
    m_tree->setColumnWidth( 0, 200 );
    m_tree->setColumnWidth( 1, 50 );

    m_tree->header()->setStretchLastSection( true );
    m_tree->setRootIsDecorated( false );

    m_tree->setFrameShape( QFrame::NoFrame );
    m_tree->setAttribute( Qt::WA_MacShowFocusRect, 0 );

    new QTreeWidgetItem( m_tree );

    connect( Pipeline::instance(), SIGNAL( resolving( Tomahawk::query_ptr ) ), SLOT( onPipelineUpdate( Tomahawk::query_ptr ) ) );
    connect( Pipeline::instance(), SIGNAL( idle() ), SLOT( onPipelineUpdate() ) );

    onPipelineUpdate();
}


void
PipelineStatusView::onPipelineUpdate( const query_ptr& query )
{
    qDebug() << Q_FUNC_INFO;

    QTreeWidgetItem* ti = m_tree->invisibleRootItem()->child( 0 );

    if ( Pipeline::instance()->activeQueryCount() && !query.isNull() )
    {
        ti->setText( 0, QString( "%1 - %2" ).arg( query->artist() ).arg( query->track() ) );
        ti->setText( 1, QString( "%1" ).arg( Pipeline::instance()->activeQueryCount() + Pipeline::instance()->pendingQueryCount() ) );

        if ( isHidden() )
            emit showWidget();
    }
    else
    {
        ti->setText( 0, tr( "Idle" ) );
        ti->setText( 1, QString( "None" ) );

        if ( !isHidden() )
            emit hideWidget();
    }
}


QSize
PipelineStatusView::sizeHint() const
{
    unsigned int y = 0;
    y += m_tree->header()->height();
    y += m_tree->contentsMargins().top() + m_tree->contentsMargins().bottom();

    if ( m_tree->invisibleRootItem()->childCount() )
    {
        unsigned int rowheight = m_tree->sizeHintForRow( 0 );
        y += rowheight * m_tree->invisibleRootItem()->childCount() + 2;
    }

    return QSize( 0, y );
}
