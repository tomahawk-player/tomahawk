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

#include "playlistview.h"

#include <QDebug>
#include <QKeyEvent>
#include <QPainter>

#include "playlist/playlistproxymodel.h"
#include "widgets/overlaywidget.h"

using namespace Tomahawk;


PlaylistView::PlaylistView( QWidget* parent )
    : TrackView( parent )
    , m_model( 0 )
    , m_itemMenu( 0 )
    , m_playItemAction( 0 )
    , m_addItemsToQueueAction( 0 )
    , m_addItemsToPlaylistAction( 0 )
    , m_deleteItemsAction( 0 )
{
    setProxyModel( new PlaylistProxyModel( this ) );

    setContextMenuPolicy( Qt::CustomContextMenu );
    connect( this, SIGNAL( customContextMenuRequested( const QPoint& ) ), SLOT( onCustomContextMenu( const QPoint& ) ) );
}


PlaylistView::~PlaylistView()
{
    qDebug() << Q_FUNC_INFO;
}


void
PlaylistView::setModel( QAbstractItemModel* model )
{
    Q_UNUSED( model );
    qDebug() << "Explicitly use setPlaylistModel instead";
    Q_ASSERT( false );
}


void
PlaylistView::setPlaylistModel( PlaylistModel* model )
{
    m_model = model;

    TrackView::setTrackModel( m_model );
    setColumnHidden( TrackModel::Age, true ); // Hide age column per default

    if ( !m_model->playlist().isNull() )
        setGuid( QString( "playlistview/%1" ).arg( m_model->playlist()->guid() ) );
    else
        setGuid( "playlistview" );

    connect( m_model, SIGNAL( trackCountChanged( unsigned int ) ), SLOT( onTrackCountChanged( unsigned int ) ) );
    connect( m_model, SIGNAL( playlistDeleted() ), SLOT( onDeleted() ) );
    connect( m_model->playlist().data(), SIGNAL( changed() ), SLOT( onChanged() ) );
}


void
PlaylistView::setupMenus()
{
    m_itemMenu.clear();

    unsigned int i = 0;
    foreach( const QModelIndex& idx, selectedIndexes() )
        if ( idx.column() == 0 )
            i++;

    m_playItemAction = m_itemMenu.addAction( tr( "&Play" ) );
    m_addItemsToQueueAction = m_itemMenu.addAction( tr( "Add to &Queue" ) );
    m_itemMenu.addSeparator();

    foreach( QAction* a, actions() )
        m_itemMenu.addAction( a );
//    m_addItemsToPlaylistAction = m_itemMenu.addAction( tr( "&Add to Playlist" ) );
//    m_itemMenu.addSeparator();
    m_deleteItemsAction = m_itemMenu.addAction( i > 1 ? tr( "&Delete Items" ) : tr( "&Delete Item" ) );

    if ( model() )
        m_deleteItemsAction->setEnabled( !model()->isReadOnly() );

    connect( m_playItemAction,           SIGNAL( triggered() ), SLOT( playItem() ) );
    connect( m_addItemsToQueueAction,    SIGNAL( triggered() ), SLOT( addItemsToQueue() ) );
//    connect( m_addItemsToPlaylistAction, SIGNAL( triggered() ), SLOT( addItemsToPlaylist() ) );
    connect( m_deleteItemsAction,        SIGNAL( triggered() ), SLOT( deleteItems() ) );
}


void
PlaylistView::onCustomContextMenu( const QPoint& pos )
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
PlaylistView::keyPressEvent( QKeyEvent* event )
{
    qDebug() << Q_FUNC_INFO;
    TrackView::keyPressEvent( event );

    if ( !model() )
        return;

    if ( event->key() == Qt::Key_Delete && !model()->isReadOnly() )
    {
        qDebug() << "Removing selected items";
        proxyModel()->removeIndexes( selectedIndexes() );
    }
}


void
PlaylistView::addItemsToPlaylist()
{
}


void
PlaylistView::deleteItems()
{
    proxyModel()->removeIndexes( selectedIndexes() );
}

void
PlaylistView::onTrackCountChanged( unsigned int tracks )
{
    if ( tracks == 0 )
    {
        overlay()->setText( tr( "This playlist is currently empty. Add some tracks to it and enjoy the music!" ) );
        overlay()->show();
    }
    else
        overlay()->hide();
}


bool
PlaylistView::jumpToCurrentTrack()
{
    scrollTo( proxyModel()->currentItem(), QAbstractItemView::PositionAtCenter );
    return true;
}


void
PlaylistView::onDeleted()
{
    qDebug() << Q_FUNC_INFO;
    emit destroyed( widget() );
    deleteLater();
}

void
PlaylistView::onChanged()
{
    if ( m_model && !m_model->playlist().isNull() )
        emit nameChanged( m_model->playlist()->title() );
}
