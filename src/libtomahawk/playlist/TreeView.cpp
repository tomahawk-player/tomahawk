/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include "TreeView.h"

#include "audio/AudioEngine.h"
#include "context/ContextWidget.h"
#include "utils/AnimatedSpinner.h"
#include "widgets/OverlayWidget.h"

#include "ContextMenu.h"
#include "TomahawkSettings.h"
#include "ViewHeader.h"
#include "TreeItemDelegate.h"
#include "TreeModel.h"
#include "PlayableItem.h"
#include "Source.h"
#include "ViewManager.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"

#include <QHeaderView>
#include <QKeyEvent>
#include <QPainter>
#include <QScrollBar>
#include <QDrag>
#include <QMimeData>

#define SCROLL_TIMEOUT 280

using namespace Tomahawk;


TreeView::TreeView( QWidget* parent )
    : QTreeView( parent )
    , m_header( new ViewHeader( this ) )
    , m_overlay( new OverlayWidget( this ) )
    , m_model( 0 )
    , m_proxyModel( 0 )
    , m_loadingSpinner( new LoadingSpinner( this ) )
    , m_updateContextView( true )
    , m_contextMenu( new ContextMenu( this ) )
{
    setFrameShape( QFrame::NoFrame );
    setAttribute( Qt::WA_MacShowFocusRect, 0 );

    setContentsMargins( 0, 0, 0, 0 );
    setMouseTracking( true );
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

    m_timer.setInterval( SCROLL_TIMEOUT );
    connect( verticalScrollBar(), SIGNAL( rangeChanged( int, int ) ), SLOT( onViewChanged() ) );
    connect( verticalScrollBar(), SIGNAL( valueChanged( int ) ), SLOT( onViewChanged() ) );
    connect( &m_timer, SIGNAL( timeout() ), SLOT( onScrollTimeout() ) );

    connect( this, SIGNAL( doubleClicked( QModelIndex ) ), SLOT( onItemActivated( QModelIndex ) ) );
    connect( this, SIGNAL( customContextMenuRequested( QPoint ) ), SLOT( onCustomContextMenu( QPoint ) ) );
    connect( m_contextMenu, SIGNAL( triggered( int ) ), SLOT( onMenuTriggered( int ) ) );
}


TreeView::~TreeView()
{
    tDebug() << Q_FUNC_INFO;
}


void
TreeView::setProxyModel( TreeProxyModel* model )
{
    m_proxyModel = model;
    TreeItemDelegate* del = new TreeItemDelegate( this, m_proxyModel );
    connect( del, SIGNAL( updateIndex( QModelIndex ) ), this, SLOT( update( QModelIndex ) ) );
    setItemDelegate( del );

    QTreeView::setModel( m_proxyModel );
}


void
TreeView::setModel( QAbstractItemModel* model )
{
    Q_UNUSED( model );
    tDebug() << "Explicitly use setPlaylistModel instead";
    Q_ASSERT( false );
}


void
TreeView::setTreeModel( TreeModel* model )
{
    m_model = model;

    if ( m_proxyModel )
    {
        m_proxyModel->setSourcePlayableModel( model );
        m_proxyModel->sort( 0 );
    }

    connect( m_proxyModel, SIGNAL( filteringStarted() ), SLOT( onFilteringStarted() ) );
    connect( m_proxyModel, SIGNAL( filteringFinished() ), m_loadingSpinner, SLOT( fadeOut() ) );

    connect( m_proxyModel, SIGNAL( filteringFinished() ), SLOT( onFilterChangeFinished() ) );
    connect( m_proxyModel, SIGNAL( rowsInserted( QModelIndex, int, int ) ), SLOT( onViewChanged() ) );

    guid(); // this will set the guid on the header

    m_header->setDefaultColumnWeights( m_proxyModel->columnWeights() );
    if ( m_proxyModel->style() == PlayableProxyModel::Large )
    {
        setHeaderHidden( true );
        setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    }
    else
    {
        setHeaderHidden( false );
        setHorizontalScrollBarPolicy( Qt::ScrollBarAsNeeded );
    }

    connect( model, SIGNAL( changed() ), this, SIGNAL( modelChanged() ) );
    emit modelChanged();

/*    setColumnHidden( PlayableModel::Score, true ); // Hide score column per default
    setColumnHidden( PlayableModel::Origin, true ); // Hide origin column per default
    setColumnHidden( PlayableModel::Composer, true ); //Hide composer column per default

    setGuid( QString( "collectionview/%1" ).arg( model->columnCount() ) );
    sortByColumn( PlayableModel::Artist, Qt::AscendingOrder );*/
}


void
TreeView::setEmptyTip( const QString& tip )
{
    m_emptyTip = tip;
    m_overlay->setText( tip );
}


void
TreeView::onViewChanged()
{
    if ( m_timer.isActive() )
        m_timer.stop();

    m_timer.start();
}


void
TreeView::onScrollTimeout()
{
    if ( m_timer.isActive() )
        m_timer.stop();

    QModelIndex left = indexAt( viewport()->rect().topLeft() );
    while ( left.isValid() && left.parent().isValid() )
        left = left.parent();

    QModelIndex right = indexAt( viewport()->rect().bottomLeft() );
    while ( right.isValid() && right.parent().isValid() )
        right = right.parent();

    int max = m_proxyModel->playlistInterface()->trackCount();
    if ( right.isValid() )
        max = right.row() + 1;

    if ( !max )
        return;

    for ( int i = left.row(); i < max; i++ )
    {
        m_model->getCover( m_proxyModel->mapToSource( m_proxyModel->index( i, 0 ) ) );
    }
}


void
TreeView::currentChanged( const QModelIndex& current, const QModelIndex& previous )
{
    QTreeView::currentChanged( current, previous );

    if ( !m_updateContextView )
        return;

    PlayableItem* item = m_model->itemFromIndex( m_proxyModel->mapToSource( current ) );
    if ( item )
    {
        if ( !item->result().isNull() )
            ViewManager::instance()->context()->setQuery( item->result()->toQuery() );
        else if ( !item->artist().isNull() )
            ViewManager::instance()->context()->setArtist( item->artist() );
        else if ( !item->album().isNull() )
            ViewManager::instance()->context()->setAlbum( item->album() );
        else if ( !item->query().isNull() )
            ViewManager::instance()->context()->setQuery( item->query() );
    }
}


void
TreeView::onItemActivated( const QModelIndex& index )
{
    PlayableItem* item = m_model->itemFromIndex( m_proxyModel->mapToSource( index ) );
    if ( item )
    {
        if ( !item->artist().isNull() )
        {
            ViewManager::instance()->show( item->artist() );
        }
        else if ( !item->album().isNull() )
        {
            ViewManager::instance()->show( item->album() );
        }
        else if ( !item->result().isNull() && item->result()->isOnline() )
        {
            AudioEngine::instance()->playItem( m_proxyModel->playlistInterface(), item->result() );
        }
        else if ( !item->query().isNull() )
        {
            AudioEngine::instance()->playItem( m_proxyModel->playlistInterface(), item->query() );
        }
    }
}


void
TreeView::keyPressEvent( QKeyEvent* event )
{
    QTreeView::keyPressEvent( event );

    if ( !model() )
        return;

    if ( event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return )
    {
        onItemActivated( currentIndex() );
    }
}


void
TreeView::resizeEvent( QResizeEvent* event )
{
    QTreeView::resizeEvent( event );
    m_header->checkState();

    if ( !model() )
        return;

    if ( model()->columnCount( QModelIndex() ) == 1 )
    {
        m_header->resizeSection( 0, event->size().width() );
    }
}


void
TreeView::onFilterChangeFinished()
{
    if ( selectedIndexes().count() )
        scrollTo( selectedIndexes().at( 0 ), QAbstractItemView::PositionAtCenter );

    if ( !proxyModel()->filter().isEmpty() && !proxyModel()->playlistInterface()->trackCount() && model()->trackCount() )
    {
        m_overlay->setText( tr( "Sorry, your filter '%1' did not match any results." ).arg( proxyModel()->filter() ) );
        m_overlay->show();
    }
    else
    {
        if ( model()->trackCount() )
        {
            m_overlay->hide();
        }
        else
        {
            m_overlay->setText( m_emptyTip );
            m_overlay->show();
        }
    }
}


void
TreeView::onFilteringStarted()
{
    m_overlay->hide();
    m_loadingSpinner->fadeIn();
}


void
TreeView::startDrag( Qt::DropActions supportedActions )
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

    tDebug( LOGVERBOSE ) << "Dragging" << indexes.count() << "indexes";
    QMimeData* data = m_proxyModel->mimeData( indexes );
    if ( !data )
        return;

    QDrag* drag = new QDrag( this );
    drag->setMimeData( data );

    QPixmap p;
    if ( data->hasFormat( "application/tomahawk.metadata.artist" ) )
        p = TomahawkUtils::createDragPixmap( TomahawkUtils::MediaTypeArtist, indexes.count() );
    else if ( data->hasFormat( "application/tomahawk.metadata.album" ) )
        p = TomahawkUtils::createDragPixmap( TomahawkUtils::MediaTypeAlbum, indexes.count() );
    else
        p = TomahawkUtils::createDragPixmap( TomahawkUtils::MediaTypeTrack, indexes.count() );

    drag->setPixmap( p );
    drag->setHotSpot( QPoint( -20, -20 ) );

    drag->exec( supportedActions, Qt::CopyAction );
}


void
TreeView::onCustomContextMenu( const QPoint& pos )
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

        PlayableItem* item = m_proxyModel->itemFromIndex( m_proxyModel->mapToSource( index ) );

        if ( item && !item->result().isNull() )
            queries << item->result()->toQuery();
        else if ( item && !item->query().isNull() )
            queries << item->query();
        if ( item && !item->artist().isNull() )
            artists << item->artist();
        if ( item && !item->album().isNull() )
            albums << item->album();
    }

    m_contextMenu->setQueries( queries );
    m_contextMenu->setArtists( artists );
    m_contextMenu->setAlbums( albums );
    m_contextMenu->setPlaylistInterface( proxyModel()->playlistInterface() );

    m_contextMenu->exec( viewport()->mapToGlobal( pos ) );
}


void
TreeView::onMenuTriggered( int action )
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
TreeView::jumpToCurrentTrack()
{
    if ( !m_proxyModel || !m_proxyModel->sourceModel() )
        return false;

    scrollTo( m_proxyModel->currentIndex(), QAbstractItemView::PositionAtCenter );
    return true;
}


void
TreeView::updateHoverIndex( const QPoint& pos )
{
    QModelIndex idx = indexAt( pos );

    if ( idx != m_hoveredIndex )
    {
        m_hoveredIndex = idx;
        repaint();
    }

    if ( !m_model || m_proxyModel->style() != PlayableProxyModel::Collection )
        return;

    PlayableItem* item = proxyModel()->itemFromIndex( proxyModel()->mapToSource( idx ) );
    if ( idx.column() == 0 && !item->query().isNull() )
    {
        if ( pos.x() > header()->sectionViewportPosition( idx.column() ) + header()->sectionSize( idx.column() ) - 16 &&
             pos.x() < header()->sectionViewportPosition( idx.column() ) + header()->sectionSize( idx.column() ) )
        {
            setCursor( Qt::PointingHandCursor );
            return;
        }
    }

    if ( cursor().shape() != Qt::ArrowCursor )
        setCursor( Qt::ArrowCursor );
}


void
TreeView::wheelEvent( QWheelEvent* event )
{
    QTreeView::wheelEvent( event );

    if ( m_hoveredIndex.isValid() )
    {
        m_hoveredIndex = QModelIndex();
        repaint();
    }
}


void
TreeView::leaveEvent( QEvent* event )
{
    QTreeView::leaveEvent( event );
    updateHoverIndex( QPoint( -1, -1 ) );
}


void
TreeView::mouseMoveEvent( QMouseEvent* event )
{
    QTreeView::mouseMoveEvent( event );
    updateHoverIndex( event->pos() );
}


void
TreeView::mousePressEvent( QMouseEvent* event )
{
    QTreeView::mousePressEvent( event );

    if ( !m_model || m_proxyModel->style() != PlayableProxyModel::Collection )
        return;

    QModelIndex idx = indexAt( event->pos() );
    if ( event->pos().x() > header()->sectionViewportPosition( idx.column() ) + header()->sectionSize( idx.column() ) - 16 &&
         event->pos().x() < header()->sectionViewportPosition( idx.column() ) + header()->sectionSize( idx.column() ) )
    {
        PlayableItem* item = proxyModel()->itemFromIndex( proxyModel()->mapToSource( idx ) );
        if ( item->query().isNull() )
            return;

        switch ( idx.column() )
        {
            case 0:
            {
                ViewManager::instance()->show( item->query()->displayQuery() );
                break;
            }

            default:
                break;
        }
    }
}


QString
TreeView::guid() const
{
    if ( m_guid.isEmpty() )
    {
        m_guid = QString( "artistview/%1" ).arg( m_model->columnCount( QModelIndex() ) );
        m_header->setGuid( m_guid );
    }

    return m_guid;
}
