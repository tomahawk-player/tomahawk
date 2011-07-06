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
#include "dynamic/widgets/LoadingSpinner.h"

#include "tomahawksettings.h"
#include "treeheader.h"
#include "treeitemdelegate.h"
#include "viewmanager.h"

static QString s_tmInfoIdentifier = QString( "TREEMODEL" );

#define SCROLL_TIMEOUT 280

using namespace Tomahawk;


ArtistView::ArtistView( QWidget* parent )
    : QTreeView( parent )
    , m_header( new TreeHeader( this ) )
    , m_model( 0 )
    , m_proxyModel( 0 )
//    , m_delegate( 0 )
    , m_loadingSpinner( new LoadingSpinner( this ) )
    , m_contextMenu( new ContextMenu( this ) )
    , m_showModes( true )
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
    setContextMenuPolicy( Qt::CustomContextMenu );

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

    m_timer.setInterval( SCROLL_TIMEOUT );

    connect( verticalScrollBar(), SIGNAL( rangeChanged( int, int ) ), SLOT( onViewChanged() ) );
    connect( verticalScrollBar(), SIGNAL( valueChanged( int ) ), SLOT( onViewChanged() ) );
    connect( &m_timer, SIGNAL( timeout() ), SLOT( onScrollTimeout() ) );

    connect( this, SIGNAL( doubleClicked( QModelIndex ) ), SLOT( onItemActivated( QModelIndex ) ) );
    connect( this, SIGNAL( customContextMenuRequested( const QPoint& ) ), SLOT( onCustomContextMenu( const QPoint& ) ) );
    connect( m_contextMenu, SIGNAL( triggered( int ) ), SLOT( onMenuTriggered( int ) ) );
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
ArtistView::setModel( QAbstractItemModel* model )
{
    Q_UNUSED( model );
    qDebug() << "Explicitly use setPlaylistModel instead";
    Q_ASSERT( false );
}


void
ArtistView::setTreeModel( TreeModel* model )
{
    m_model = model;

    if ( m_proxyModel )
    {
        m_proxyModel->setSourceTreeModel( model );
        m_proxyModel->sort( 0 );
    }

    connect( m_model, SIGNAL( loadingStarted() ), m_loadingSpinner, SLOT( fadeIn() ) );
    connect( m_model, SIGNAL( loadingFinished() ), m_loadingSpinner, SLOT( fadeOut() ) );

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
            ViewManager::instance()->show( item->artist() );
        else if ( !item->album().isNull() )
            ViewManager::instance()->show( item->album() );
        else if ( !item->result().isNull() )
        {
            m_model->setCurrentItem( item->index );
            AudioEngine::instance()->playItem( m_proxyModel, item->result() );
        }
    }
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
    QList<QPersistentModelIndex> pindexes;
    QModelIndexList indexes;
    foreach( const QModelIndex& idx, selectedIndexes() )
    {
        if ( ( m_proxyModel->flags( idx ) & Qt::ItemIsDragEnabled ) )
        {
            indexes << idx;
            pindexes << idx;
        }
    }

    if ( indexes.count() == 0 )
        return;

    qDebug() << "Dragging" << indexes.count() << "indexes";
    QMimeData* data = m_proxyModel->mimeData( indexes );
    if ( !data )
        return;

    QDrag* drag = new QDrag( this );
    drag->setMimeData( data );
    const QPixmap p = TomahawkUtils::createDragPixmap( indexes.count() );
    drag->setPixmap( p );
    drag->setHotSpot( QPoint( -20, -20 ) );

    drag->exec( supportedActions, Qt::CopyAction );
}


void
ArtistView::onViewChanged()
{
    if ( m_timer.isActive() )
        m_timer.stop();

    m_timer.start();
}


void
ArtistView::onScrollTimeout()
{
    qDebug() << Q_FUNC_INFO;
    if ( m_timer.isActive() )
        m_timer.stop();

    QModelIndex left = indexAt( viewport()->rect().topLeft() );
    while ( left.isValid() && left.parent().isValid() )
        left = left.parent();

    QModelIndex right = indexAt( viewport()->rect().bottomLeft() );
    while ( right.isValid() && right.parent().isValid() )
        right = right.parent();

    int max = m_proxyModel->trackCount();
    if ( right.isValid() )
        max = right.row() + 1;

    if ( !max )
        return;

    for ( int i = left.row(); i < max; i++ )
    {
        TreeModelItem* item = m_model->itemFromIndex( m_proxyModel->mapToSource( m_proxyModel->index( i, 0 ) ) );
        if ( item->artist().isNull() )
            continue;

        Tomahawk::InfoSystem::InfoCriteriaHash trackInfo;
        trackInfo["artist"] = item->artist()->name();
        trackInfo["pptr"] = QString::number( (qlonglong)item );

        Tomahawk::InfoSystem::InfoSystem::instance()->getInfo(
            s_tmInfoIdentifier, Tomahawk::InfoSystem::InfoArtistImages,
            QVariant::fromValue< Tomahawk::InfoSystem::InfoCriteriaHash >( trackInfo ), QVariantMap() );
    }
}


void
ArtistView::onCustomContextMenu( const QPoint& pos )
{
    m_contextMenu->clear();

    QModelIndex idx = indexAt( pos );
    idx = idx.sibling( idx.row(), 0 );
    m_contextMenuIndex = idx;

    if ( !idx.isValid() )
        return;

    QList<query_ptr> queries;
    QList<artist_ptr> artists;
    QList<album_ptr> albums;

    foreach ( const QModelIndex& index, selectedIndexes() )
    {
        if ( index.column() || selectedIndexes().contains( index.parent() ) )
            continue;

        TreeModelItem* item = m_proxyModel->itemFromIndex( m_proxyModel->mapToSource( index ) );

        if ( item && !item->result().isNull() )
            queries << item->result()->toQuery();
        if ( item && !item->artist().isNull() )
            artists << item->artist();
        if ( item && !item->album().isNull() )
            albums << item->album();
    }

    m_contextMenu->setQueries( queries );
    m_contextMenu->setArtists( artists );
    m_contextMenu->setAlbums( albums );

    m_contextMenu->exec( mapToGlobal( pos ) );
}


void
ArtistView::onMenuTriggered( int action )
{
    switch ( action )
    {
        case ContextMenu::ActionPlay:
            onItemActivated( m_contextMenuIndex );
            break;

        default:
            break;
    }
}


bool
ArtistView::jumpToCurrentTrack()
{
    scrollTo( m_proxyModel->currentIndex(), QAbstractItemView::PositionAtCenter );
    return true;
}
