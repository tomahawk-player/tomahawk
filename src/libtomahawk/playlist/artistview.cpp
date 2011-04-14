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

#include "artistview.h"

#include <QDebug>
#include <QHeaderView>
#include <QKeyEvent>
#include <QPainter>
#include <QScrollBar>

#include "audio/audioengine.h"

#include "tomahawksettings.h"
#include "treeheader.h"
#include "treeitemdelegate.h"
#include "playlistmanager.h"

using namespace Tomahawk;


ArtistView::ArtistView( QWidget* parent )
    : QTreeView( parent )
    , m_model( 0 )
    , m_proxyModel( 0 )
    , m_header( new TreeHeader( this ) )
//    , m_delegate( 0 )
{
    setAlternatingRowColors( true );
    setDragEnabled( true );
    setDropIndicatorShown( false );
    setDragDropOverwriteMode( false );
    setUniformRowHeights( false );
    setVerticalScrollMode( QAbstractItemView::ScrollPerPixel );
    setRootIsDecorated( true );
    setAnimated( false );
    setAllColumnsShowFocus( true );
    setSelectionMode( QAbstractItemView::ExtendedSelection );
    setSelectionBehavior( QAbstractItemView::SelectRows );

    setHeader( m_header );
    setProxyModel( new TreeProxyModel( this ) );

    #ifndef Q_WS_WIN
    QFont f = font();
    f.setPointSize( f.pointSize() - 1 );
    setFont( f );
    #endif

    #ifdef Q_WS_MAC
    f.setPointSize( f.pointSize() - 2 );
    setFont( f );
    #endif

    connect( this, SIGNAL( doubleClicked( QModelIndex ) ), SLOT( onItemActivated( QModelIndex ) ) );
}


ArtistView::~ArtistView()
{
    qDebug() << Q_FUNC_INFO;
}


void
ArtistView::setProxyModel( TreeProxyModel* model )
{
    m_proxyModel = model;
    setItemDelegate( new TreeItemDelegate( this, m_proxyModel ) );

    QTreeView::setModel( m_proxyModel );
}


void
ArtistView::setModel( TreeModel* model )
{
    m_model = model;

    if ( m_proxyModel )
    {
        m_proxyModel->setSourceModel( model );
        m_proxyModel->sort( 0 );
    }

    connect( m_proxyModel, SIGNAL( filterChanged( QString ) ), SLOT( onFilterChanged( QString ) ) );

    setAcceptDrops( false );
}


void
ArtistView::onItemActivated( const QModelIndex& index )
{
    TreeModelItem* item = m_model->itemFromIndex( m_proxyModel->mapToSource( index ) );
    if ( item )
    {
        if ( !item->artist().isNull() )
            PlaylistManager::instance()->show( item->artist() );
        else if ( !item->album().isNull() )
            PlaylistManager::instance()->show( item->album() );
        else if ( !item->result().isNull() )
            AudioEngine::instance()->playItem( 0, item->result() );
    }
}


void
ArtistView::dragEnterEvent( QDragEnterEvent* event )
{
    qDebug() << Q_FUNC_INFO;
    QTreeView::dragEnterEvent( event );
}


void
ArtistView::dragMoveEvent( QDragMoveEvent* event )
{
    QTreeView::dragMoveEvent( event );
}


void
ArtistView::dropEvent( QDropEvent* event )
{
    QTreeView::dropEvent( event );
}


void
ArtistView::paintEvent( QPaintEvent* event )
{
    QTreeView::paintEvent( event );
}


void
ArtistView::resizeEvent( QResizeEvent* event )
{
    QTreeView::resizeEvent( event );
    m_header->checkState();
}


void
ArtistView::onFilterChanged( const QString& )
{
    if ( selectedIndexes().count() )
        scrollTo( selectedIndexes().at( 0 ), QAbstractItemView::PositionAtCenter );
}


void
ArtistView::startDrag( Qt::DropActions supportedActions )
{
}


// Inspired from dolphin's draganddrophelper.cpp
QPixmap
ArtistView::createDragPixmap( int itemCount ) const
{
    return QPixmap();
}
