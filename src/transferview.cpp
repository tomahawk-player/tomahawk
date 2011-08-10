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

#include "transferview.h"

#include <QHeaderView>
#include <QVBoxLayout>

#include "artist.h"
#include "source.h"
#include "network/streamconnection.h"
#include "network/servent.h"

#include "utils/logger.h"


TransferView::TransferView( AnimatedSplitter* parent )
    : AnimatedWidget( parent )
    , m_parent( parent )
{
    setHiddenSize( QSize( 0, 0 ) );
    setLayout( new QVBoxLayout() );
    m_tree = new QTreeWidget( this );

    layout()->setMargin( 0 );
    layout()->addWidget( m_tree );

    connect( Servent::instance(), SIGNAL( streamStarted( StreamConnection* ) ), SLOT( streamRegistered( StreamConnection* ) ) );
    connect( Servent::instance(), SIGNAL( streamFinished( StreamConnection* ) ), SLOT( streamFinished( StreamConnection* ) ) );

    QStringList headers;
    headers << tr( "Peer" ) << tr( "Rate" ) << tr( "Track" );
    m_tree->setHeaderLabels( headers );

    m_tree->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    m_tree->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Ignored );
    m_tree->setColumnCount( 3 );
    m_tree->setColumnWidth( 0, 80 );
    m_tree->setColumnWidth( 1, 65 );
    m_tree->setColumnWidth( 2, 10 );

    m_tree->header()->setStretchLastSection( true );
    m_tree->setRootIsDecorated( false );

    m_tree->setFrameShape( QFrame::NoFrame );
    m_tree->setAttribute( Qt::WA_MacShowFocusRect, 0 );
}


void
TransferView::streamRegistered( StreamConnection* sc )
{
    qDebug() << Q_FUNC_INFO;
    connect( sc, SIGNAL( updated() ), SLOT( onTransferUpdate() ) );
}


void
TransferView::streamFinished( StreamConnection* sc )
{
    if ( !m_index.contains( sc ) )
        return;

    QPersistentModelIndex i = m_index.take( sc );
    delete m_tree->invisibleRootItem()->takeChild( i.row() );

    if ( m_tree->invisibleRootItem()->childCount() > 0 )
        emit showWidget();
    else
        emit hideWidget();
}


void
TransferView::onTransferUpdate()
{
    StreamConnection* sc = (StreamConnection*)sender();
//    qDebug() << Q_FUNC_INFO << sc->track().isNull() << sc->source().isNull();

    if ( sc->track().isNull() || sc->source().isNull() )
        return;

    QTreeWidgetItem* ti = 0;

    if ( m_index.contains( sc ) )
    {
        QPersistentModelIndex i = m_index.value( sc );
        ti = m_tree->invisibleRootItem()->child( i.row() );
    }
    else
    {
        ti = new QTreeWidgetItem( m_tree );
        m_index.insert( sc, QPersistentModelIndex( m_tree->model()->index( m_tree->invisibleRootItem()->childCount() - 1, 0 ) ) );
        emit showWidget();
    }

    if ( !ti )
        return;

    ti->setText( 0, sc->source()->friendlyName() );
    ti->setText( 1, QString( "%1 kb/s" ).arg( sc->transferRate() / 1024 ) );
    ti->setText( 2, QString( "%1 - %2" ).arg( sc->track()->artist()->name() ).arg( sc->track()->track() ) );

    if ( isHidden() )
        emit showWidget();
}


QSize
TransferView::sizeHint() const
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
