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

#include "collectionview.h"

#include <QDebug>
#include <QDragEnterEvent>
#include <QPainter>

#include "playlist/collectionproxymodel.h"
#include "widgets/overlaywidget.h"

using namespace Tomahawk;


CollectionView::CollectionView( QWidget* parent )
    : TrackView( parent )
{
    setProxyModel( new CollectionProxyModel( this ) );

    setSortingEnabled( true );
    sortByColumn( 0, Qt::AscendingOrder );

    setDragDropMode( QAbstractItemView::DragOnly );
    setAcceptDrops( false );

    setContextMenuPolicy( Qt::CustomContextMenu );
    connect( this, SIGNAL( customContextMenuRequested( const QPoint& ) ), SLOT( onCustomContextMenu( const QPoint& ) ) );
}


CollectionView::~CollectionView()
{
    qDebug() << Q_FUNC_INFO;
}


void
CollectionView::setModel( QAbstractItemModel* model )
{
    Q_UNUSED( model );
    qDebug() << "Explicitly use setTrackModel instead";
    Q_ASSERT( false );
}


void
CollectionView::setTrackModel( TrackModel* model )
{
    TrackView::setTrackModel( model );
    setColumnHidden( TrackModel::Score, true ); // Hide age column per default
    setGuid( "collectionview" );

    setSortingEnabled( true );
    sortByColumn( 0, Qt::AscendingOrder );

    connect( model, SIGNAL( trackCountChanged( unsigned int ) ), SLOT( onTrackCountChanged( unsigned int ) ) );
}


void
CollectionView::dragEnterEvent( QDragEnterEvent* event )
{
    event->ignore();
}


void
CollectionView::setupMenus()
{
    m_itemMenu.clear();

    m_playItemAction = m_itemMenu.addAction( tr( "&Play" ) );
    m_addItemsToQueueAction = m_itemMenu.addAction( tr( "Add to &Queue" ) );
    m_itemMenu.addSeparator();

    foreach( QAction* a, actions() )
     m_itemMenu.addAction( a );
//    m_addItemsToPlaylistAction = m_itemMenu.addAction( tr( "&Add to Playlist" ) );

     connect( m_playItemAction,           SIGNAL( triggered() ), SLOT( playItem() ) );
     connect( m_addItemsToQueueAction,    SIGNAL( triggered() ), SLOT( addItemsToQueue() ) );
//    connect( m_addItemsToPlaylistAction, SIGNAL( triggered() ), SLOT( addItemsToPlaylist() ) );
}


void
CollectionView::onCustomContextMenu( const QPoint& pos )
{
    qDebug() << Q_FUNC_INFO;
    setupMenus();

    QModelIndex idx = indexAt( pos );
    idx = idx.sibling( idx.row(), 0 );
    setContextMenuIndex( idx );

    if ( !idx.isValid() )
        return;

    m_itemMenu.exec( mapToGlobal( pos ) );
}


void
CollectionView::onTrackCountChanged( unsigned int tracks )
{
    if ( tracks == 0 )
    {
        overlay()->setText( tr( "This collection is empty." ) );
        overlay()->show();
    }
    else
        overlay()->hide();
}


bool
CollectionView::jumpToCurrentTrack()
{
    scrollTo( proxyModel()->currentIndex(), QAbstractItemView::PositionAtCenter );
    return true;
}
