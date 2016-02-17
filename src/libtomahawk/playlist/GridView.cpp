/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2015, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "GridView.h"

#include "audio/AudioEngine.h"
#include "TomahawkSettings.h"
#include "Artist.h"
#include "Source.h"
#include "PlayableItem.h"
#include "GridItemDelegate.h"
#include "AlbumModel.h"
#include "PlayableModel.h"
#include "PlayableProxyModelPlaylistInterface.h"
#include "ContextMenu.h"
#include "ViewManager.h"
#include "MetaPlaylistInterface.h"
#include "utils/DpiScaler.h"
#include "utils/Logger.h"
#include "utils/AnimatedSpinner.h"
#include "utils/PixmapDelegateFader.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/TomahawkStyle.h"

#include <QDrag>
#include <QHeaderView>
#include <QKeyEvent>
#include <QMimeData>
#include <QPainter>
#include <QScrollBar>
#include <qmath.h>

#define SCROLL_TIMEOUT 280

using namespace Tomahawk;


GridView::GridView( QWidget* parent )
    : QListView( parent )
    , m_model( 0 )
    , m_proxyModel( 0 )
    , m_delegate( 0 )
    , m_loadingSpinner( new LoadingSpinner( this ) )
    , m_overlay( new OverlayWidget( this ) )
    , m_contextMenu( new ContextMenu( this ) )
    , m_inited( false )
    , m_itemWidth( 0 )
{
    setFrameShape( QFrame::NoFrame );
    setAttribute( Qt::WA_MacShowFocusRect, 0 );

    setDragEnabled( true );
    setDropIndicatorShown( false );
    setDragDropOverwriteMode( false );
    setUniformItemSizes( true );
    setSpacing( TomahawkUtils::DpiScaler::scaledX( this, 16 ) );
    setContentsMargins( 0, 0, 0, 0 );
    setMouseTracking( true );
    setContextMenuPolicy( Qt::CustomContextMenu );
    setResizeMode( Adjust );
    setViewMode( IconMode );
    setVerticalScrollMode( QAbstractItemView::ScrollPerPixel );
    setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOn );
    setEditTriggers( NoEditTriggers );

    setStyleSheet( QString( "QListView { background-color: %1; }" ).arg( TomahawkStyle::PAGE_BACKGROUND.name() ) );

    setAutoFitItems( true );
    setAutoResize( false );

    setItemWidth( TomahawkUtils::DpiScaler::scaledX( this, 170 ) );
    setProxyModel( new PlayableProxyModel( this ) );

    m_timer.setInterval( SCROLL_TIMEOUT );
    connect( verticalScrollBar(), SIGNAL( rangeChanged( int, int ) ), SLOT( onViewChanged() ) );
    connect( verticalScrollBar(), SIGNAL( valueChanged( int ) ), SLOT( onViewChanged() ) );
    connect( &m_timer, SIGNAL( timeout() ), SLOT( onScrollTimeout() ) );

    connect( this, SIGNAL( doubleClicked( QModelIndex ) ), SLOT( onItemActivated( QModelIndex ) ) );
    connect( this, SIGNAL( customContextMenuRequested( QPoint ) ), SLOT( onCustomContextMenu( QPoint ) ) );

    m_mpl = new Tomahawk::MetaPlaylistInterface();
    m_mpl->addChildInterface( proxyModel()->playlistInterface() );
    m_playlistInterface = playlistinterface_ptr( m_mpl );
}


GridView::~GridView()
{
    tDebug() << Q_FUNC_INFO;
}


void
GridView::setProxyModel( PlayableProxyModel* model )
{
    if ( m_proxyModel )
    {
        disconnect( m_proxyModel, SIGNAL( filterChanged( QString ) ), this, SLOT( onFilterChanged( QString ) ) );
        disconnect( m_proxyModel, SIGNAL( rowsInserted( QModelIndex, int, int ) ), this, SLOT( verifySize() ) );
        disconnect( m_proxyModel, SIGNAL( rowsRemoved( QModelIndex, int, int ) ), this, SLOT( verifySize() ) );
        disconnect( m_proxyModel, SIGNAL( rowsInserted( QModelIndex, int, int ) ), this, SLOT( onViewChanged() ) );
        disconnect( m_proxyModel, SIGNAL( modelReset() ), this, SLOT( layoutItems() ) );
    }

    m_proxyModel = model;
    connect( m_proxyModel, SIGNAL( filterChanged( QString ) ), SLOT( onFilterChanged( QString ) ) );
    connect( m_proxyModel, SIGNAL( rowsInserted( QModelIndex, int, int ) ), SLOT( verifySize() ), Qt::QueuedConnection );
    connect( m_proxyModel, SIGNAL( rowsInserted( QModelIndex, int, int ) ), SLOT( onViewChanged() ), Qt::QueuedConnection );
    connect( m_proxyModel, SIGNAL( rowsRemoved( QModelIndex, int, int ) ), SLOT( verifySize() ), Qt::QueuedConnection );
    connect( m_proxyModel, SIGNAL( modelReset() ), SLOT( layoutItems() ), Qt::QueuedConnection );

    if ( m_delegate )
        delete m_delegate;

    m_delegate = new GridItemDelegate( this, m_proxyModel );
    connect( m_delegate, SIGNAL( startedPlaying( QPersistentModelIndex ) ), this, SLOT( onDelegatePlaying( QPersistentModelIndex ) ) );
    connect( m_delegate, SIGNAL( stoppedPlaying( QPersistentModelIndex ) ), this, SLOT( onDelegateStopped( QPersistentModelIndex ) ) );
    m_delegate->setItemWidth( m_itemWidth );
    setItemDelegate( m_delegate );

    QListView::setModel( m_proxyModel );
}


void
GridView::setModel( QAbstractItemModel* model )
{
    Q_UNUSED( model );
    qDebug() << "Explicitly use setAlbumModel instead";
    Q_ASSERT( false );
}


void
GridView::setPlayableModel( PlayableModel* model )
{
    if ( m_model )
    {
        disconnect( m_model, SIGNAL( loadingStarted() ), m_loadingSpinner, SLOT( fadeIn() ) );
        disconnect( m_model, SIGNAL( loadingFinished() ), m_loadingSpinner, SLOT( fadeOut() ) );
    }

    m_inited = false;
    m_model = model;

    if ( m_proxyModel )
    {
        m_proxyModel->setSourcePlayableModel( m_model );
        m_proxyModel->sort( -1 );
    }

    connect( m_model, SIGNAL( loadingStarted() ), m_loadingSpinner, SLOT( fadeIn() ) );
    connect( m_model, SIGNAL( loadingFinished() ), m_loadingSpinner, SLOT( fadeOut() ) );

    if ( m_model->isLoading() )
        m_loadingSpinner->fadeIn();

    emit modelChanged();
}


void
GridView::setEmptyTip( const QString& tip )
{
    m_emptyTip = tip;
    m_overlay->setText( tip );
}


void
GridView::currentChanged( const QModelIndex& current, const QModelIndex& previous )
{
    QListView::currentChanged( current, previous );

/*    PlayableItem* item = m_model->itemFromIndex( m_proxyModel->mapToSource( current ) );
    if ( item )
    {
    }*/
}


void
GridView::onItemActivated( const QModelIndex& index )
{
    PlayableItem* item = m_model->itemFromIndex( m_proxyModel->mapToSource( index ) );
    if ( item )
    {
        if ( !item->album().isNull() )
            ViewManager::instance()->show( item->album() );
        else if ( !item->artist().isNull() )
            ViewManager::instance()->show( item->artist() );
        else if ( !item->query().isNull() )
            ViewManager::instance()->show( item->query() );
    }
}


void
GridView::scrollContentsBy( int dx, int dy )
{
    QListView::scrollContentsBy( dx, dy );
    emit scrolledContents( dx, dy );
}


void
GridView::leaveEvent( QEvent* event )
{
    QListView::leaveEvent( event );

    m_delegate->resetHoverIndex();
}


void
GridView::paintEvent( QPaintEvent* event )
{
    if ( !autoFitItems() || m_inited || !m_proxyModel->rowCount() )
        QListView::paintEvent( event );
}


void
GridView::resizeEvent( QResizeEvent* event )
{
    QListView::resizeEvent( event );
    layoutItems();

    emit resized();
}


void GridView::wheelEvent( QWheelEvent* e )
{
    //HACK: Workaround for QTBUG-7232: Smooth scrolling (scroll per pixel) in ItemViews
    //      does not work as expected.
    verticalScrollBar()->setSingleStep( delegate()->itemSize().height() / 8 );
                                     // ^ scroll step is 1/8 of the estimated row height

    QListView::wheelEvent( e );

    m_delegate->resetHoverIndex();
}


QSize
GridView::minimumSizeHint() const
{
    if ( !m_sizeHint.isEmpty() )
        return m_sizeHint;

    return QListView::minimumSizeHint();
}


QSize
GridView::sizeHint() const
{
    if ( !m_sizeHint.isEmpty() )
        return m_sizeHint;

    return QListView::sizeHint();
}


void
GridView::verifySize()
{
    if ( !autoResize() || !m_model )
        return;

    int scrollbar = verticalScrollBar()->rect().width();

    if ( rect().width() - contentsRect().width() > scrollbar ) //HACK: if the contentsRect includes the scrollbar
        scrollbar = 0; //don't count it any more

    const int rectWidth = contentsRect().width() - scrollbar - 3 - spacing();
    const int itemWidth = m_itemWidth + spacing();
    const int itemsPerRow = qMax( 1, qFloor( rectWidth / itemWidth ) );

    const int overlapRows = m_model->rowCount( QModelIndex() ) % itemsPerRow;
    const int rows = qMax( 1.0, floor( (double)m_model->rowCount( QModelIndex() ) / (double)itemsPerRow ) );
    const int newHeight = rows * ( m_delegate->itemSize().height() + spacing() );

    if ( !isWrapping() )
    {
        m_proxyModel->setMaxVisibleItems( itemsPerRow );
    }
    else if ( newHeight > 0 )
    {
        m_proxyModel->setMaxVisibleItems( m_model->rowCount( QModelIndex() ) - overlapRows );

        m_sizeHint = QSize( QListView::sizeHint().width(), newHeight + spacing() );
        updateGeometry();
        setMinimumHeight( newHeight + spacing() );
    }
}


void
GridView::layoutItems()
{
    if ( autoFitItems() && m_model )
    {
        int scrollbar = verticalScrollBar()->rect().width();

        if ( rect().width() - contentsRect().width() >= scrollbar ) //HACK: if the contentsRect includes the scrollbar
            scrollbar = 0; //don't count it any more

        const int rectWidth = contentsRect().width() - scrollbar - 3 - spacing();
        const int itemWidth = m_itemWidth + spacing();
        const int itemsPerRow = qMax( 1, qFloor( rectWidth / itemWidth ) );

        const int remSpace = rectWidth - ( itemsPerRow * itemWidth );
        const int extraSpace = remSpace / itemsPerRow;
        const int newItemWidth = itemWidth + extraSpace - spacing();

        m_delegate->setItemWidth( newItemWidth );
    }

    verifySize();

    if ( !m_inited )
    {
        m_inited = true;
        repaint();
    }
}


void
GridView::onDelegatePlaying( const QPersistentModelIndex& index )
{
    m_playing = index;
    m_mpl->addChildInterface( AudioEngine::instance()->currentTrackPlaylist() );
}


void
GridView::onDelegateStopped( const QPersistentModelIndex& index )
{
    if ( m_playing == index )
        m_playing = QPersistentModelIndex();
    m_mpl->removeChildInterface( AudioEngine::instance()->currentTrackPlaylist() );
}


void
GridView::onFilterChanged( const QString& )
{
    if ( !selectedIndexes().isEmpty() )
        scrollTo( selectedIndexes().at( 0 ), QAbstractItemView::PositionAtCenter );

    onViewChanged();
}


void
GridView::startDrag( Qt::DropActions supportedActions )
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

    if ( indexes.isEmpty() )
        return;

    qDebug() << "Dragging" << indexes.count() << "indexes";
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

    /* Qt::DropAction action = */ drag->exec( supportedActions, Qt::CopyAction );
}


void
GridView::onCustomContextMenu( const QPoint& pos )
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

        PlayableItem* item = m_model->itemFromIndex( m_proxyModel->mapToSource( index ) );

        if ( item && !item->query().isNull() )
            queries << item->query();
        else if ( item && !item->artist().isNull() )
            artists << item->artist();
        else if ( item && !item->album().isNull() )
            albums << item->album();
    }

    m_contextMenu->setQueries( queries );
    m_contextMenu->setArtists( artists );
    m_contextMenu->setAlbums( albums );

    m_contextMenu->exec( viewport()->mapToGlobal( pos ) );
}


bool
GridView::setFilter( const QString& filter )
{
    ViewPage::setFilter( filter );
    m_proxyModel->setFilter( filter );
    return true;
}


bool
GridView::jumpToCurrentTrack()
{
    if ( !m_playing.isValid() )
        return false;

    scrollTo( m_playing, QListView::PositionAtCenter );

    return true;
}


QRect
GridView::currentTrackRect() const
{
    if ( !m_playing.isValid() )
        return QRect();

    return visualRect( m_playing );
}


playlistinterface_ptr
GridView::playlistInterface() const
{
    return m_playlistInterface;
}


void
GridView::setPlaylistInterface( const Tomahawk::playlistinterface_ptr& playlistInterface )
{
    m_mpl->removeChildInterface( proxyModel()->playlistInterface() );
    proxyModel()->setPlaylistInterface( playlistInterface );
    m_mpl->addChildInterface( proxyModel()->playlistInterface() );
}


QSize
GridView::itemSize() const
{
    return m_delegate->itemSize();
}


void
GridView::setItemWidth( int width )
{
    m_itemWidth = width;
    if ( m_delegate )
        m_delegate->setItemWidth( m_itemWidth );

    layoutItems();
}


void
GridView::onViewChanged()
{
    if ( m_timer.isActive() )
        m_timer.stop();

    m_timer.start();
}


void
GridView::onScrollTimeout()
{
    if ( m_timer.isActive() )
        m_timer.stop();

    QModelIndex left, right;
    for ( int y = viewport()->rect().topLeft().y(); y <= viewport()->rect().bottomLeft().y(); y += ( spacing() + 1 ) )
    {
        for ( int x = viewport()->rect().topLeft().x(); x <= viewport()->rect().topRight().x(); x += ( spacing() + 1 ) )
        {
            left = indexAt( QPoint( x, y ) );
            if ( left.isValid() )
                break;
        }
        if ( left.isValid() )
            break;
    }
    if ( !left.isValid() )
    {
        tDebug() << "Could not find first visible index!";
        return;
    }
    while ( left.isValid() && left.parent().isValid() )
        left = left.parent();

    for ( int y = viewport()->rect().bottomLeft().y(); y >= viewport()->rect().topLeft().y(); y -= ( spacing() + 1 ) )
    {
        for ( int x = viewport()->rect().topRight().x(); x >= viewport()->rect().topLeft().x(); x -= ( spacing() + 1 ) )
        {
            right = indexAt( QPoint( x, y ) );
            if ( right.isValid() )
                break;
        }
        if ( right.isValid() )
            break;
    }
    while ( right.isValid() && right.parent().isValid() )
        right = right.parent();
    if ( !right.isValid() )
    {
        tDebug() << "Could not find last visible index!";
        return;
    }

    for ( int i = left.row(); i <= right.row(); i++ )
    {
        m_proxyModel->updateDetailedInfo( m_proxyModel->index( i, 0 ) );
    }
}


PlayableModel*
GridView::model() const
{
    return m_model.data();
}
