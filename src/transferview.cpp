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

#include "tomahawk/tomahawkapp.h"
#include "artist.h"
#include "source.h"
#include "network/filetransferconnection.h"
#include "network/servent.h"


TransferView::TransferView( AnimatedSplitter* parent )
    : AnimatedWidget( parent )
    , m_parent( parent )
{
    setHiddenSize( QSize( 0, 0 ) );
    setLayout( new QVBoxLayout() );
    m_tree = new QTreeWidget( this );

    layout()->setMargin( 0 );
    layout()->addWidget( m_tree );

    connect( Servent::instance(), SIGNAL( fileTransferStarted( FileTransferConnection* ) ), SLOT( fileTransferRegistered( FileTransferConnection* ) ) );
    connect( Servent::instance(), SIGNAL( fileTransferFinished( FileTransferConnection* ) ), SLOT( fileTransferFinished( FileTransferConnection* ) ) );

    QStringList headers;
    headers << tr( "Peer" ) << tr( "Rate" ) << tr( "Track" );
    m_tree->setHeaderLabels( headers );

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
TransferView::fileTransferRegistered( FileTransferConnection* ftc )
{
    qDebug() << Q_FUNC_INFO;
    connect( ftc, SIGNAL( updated() ), SLOT( onTransferUpdate() ) );
}


void
TransferView::fileTransferFinished( FileTransferConnection* ftc )
{
    if ( !m_index.contains( ftc ) )
        return;

    QPersistentModelIndex i = m_index.take( ftc );
    delete m_tree->invisibleRootItem()->takeChild( i.row() );

    if ( m_tree->invisibleRootItem()->childCount() > 0 )
        emit showWidget();
    else
        emit hideWidget();

/*    if ( m_index.contains( ftc ) )
    {
        int i = m_index.value( ftc );
        m_tree->invisibleRootItem()->child( i )->setText( 1, tr( "Finished" ) );
    }*/
}


void
TransferView::onTransferUpdate()
{
    FileTransferConnection* ftc = (FileTransferConnection*)sender();
//    qDebug() << Q_FUNC_INFO << ftc->track().isNull() << ftc->source().isNull();

    if ( ftc->track().isNull() || ftc->source().isNull() )
        return;

    QTreeWidgetItem* ti = 0;

    if ( m_index.contains( ftc ) )
    {
        QPersistentModelIndex i = m_index.value( ftc );
        ti = m_tree->invisibleRootItem()->child( i.row() );
    }
    else
    {
        ti = new QTreeWidgetItem( m_tree );
        m_index.insert( ftc, QPersistentModelIndex( m_tree->model()->index( m_tree->invisibleRootItem()->childCount() - 1, 0 ) ) );
        emit showWidget();
    }

    if ( !ti )
        return;

    ti->setText( 0, ftc->source()->friendlyName() );
    ti->setText( 1, QString( "%1 kb/s" ).arg( ftc->transferRate() / 1024 ) );
    ti->setText( 2, QString( "%1 - %2" ).arg( ftc->track()->artist()->name() ).arg( ftc->track()->track() ) );

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
