/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include <QKeyEvent>
#include <QPainter>
#include <QScrollBar>
#include <qmath.h>

#include "audio/AudioEngine.h"
#include "context/ContextWidget.h"
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
#include "utils/Logger.h"
#include "utils/AnimatedSpinner.h"
#include "utils/TomahawkUtilsGui.h"

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
{
    setFrameShape( QFrame::NoFrame );
    setAttribute( Qt::WA_MacShowFocusRect, 0 );

    setDragEnabled( true );
    setDropIndicatorShown( false );
    setDragDropOverwriteMode( false );
    setUniformItemSizes( true );
    setSpacing( 0 );
    setContentsMargins( 0, 0, 0, 0 );
    setMouseTracking( true );
    setContextMenuPolicy( Qt::CustomContextMenu );
    setResizeMode( Adjust );
    setViewMode( IconMode );
    setVerticalScrollMode( QAbstractItemView::ScrollPerPixel );
    setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOn );

    setStyleSheet( "QListView { background-color: #272b2e; }" );

    setAutoFitItems( true );
    setAutoResize( false );
    setProxyModel( new PlayableProxyModel( this ) );

    connect( this, SIGNAL( doubleClicked( QModelIndex ) ), SLOT( onItemActivated( QModelIndex ) ) );
    connect( this, SIGNAL( customContextMenuRequested( QPoint ) ), SLOT( onCustomContextMenu( QPoint ) ) );
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
        disconnect( proxyModel(), SIGNAL( modelReset() ), this, SLOT( layoutItems() ) );
    }

    m_proxyModel = model;
    connect( m_proxyModel, SIGNAL( filterChanged( QString ) ), SLOT( onFilterChanged( QString ) ) );
    connect( m_proxyModel, SIGNAL( rowsInserted( QModelIndex, int, int ) ), SLOT( verifySize() ) );
    connect( m_proxyModel, SIGNAL( rowsRemoved( QModelIndex, int, int ) ), SLOT( verifySize() ) );
    connect( proxyModel(), SIGNAL( modelReset() ), SLOT( layoutItems() ) );
    
    if ( m_delegate )
        delete m_delegate;

    m_delegate = new GridItemDelegate( this, m_proxyModel );
    connect( m_delegate, SIGNAL( updateIndex( QModelIndex ) ), this, SLOT( update( QModelIndex ) ) );
    connect( m_delegate, SIGNAL( startedPlaying( QPersistentModelIndex ) ), this, SLOT( onDelegatePlaying( QPersistentModelIndex ) ) );
    connect( m_delegate, SIGNAL( stoppedPlaying( QPersistentModelIndex ) ), this, SLOT( onDelegateStopped( QPersistentModelIndex ) ) );
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
    m_inited = false;
    m_model = model;

    if ( m_proxyModel )
    {
        m_proxyModel->setSourcePlayableModel( m_model );
        m_proxyModel->sort( -1 );
    }

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

    PlayableItem* item = m_model->itemFromIndex( m_proxyModel->mapToSource( current ) );
    if ( item )
    {
        if ( !item->album().isNull() )
            ViewManager::instance()->context()->setAlbum( item->album() );
    }
}


void
GridView::onItemActivated( const QModelIndex& index )
{
    PlayableItem* item = m_model->itemFromIndex( m_proxyModel->mapToSource( index ) );
    if ( item )
    {
//        qDebug() << "Result activated:" << item->album()->tracks().first()->toString() << item->album()->tracks().first()->results().first()->url();
//        APP->audioEngine()->playItem( item->album().data(), item->album()->tracks().first()->results().first() );

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
#ifndef Q_WS_MAC
    //HACK: Workaround for QTBUG-7232: Smooth scrolling (scroll per pixel) in ItemViews
    //      does not work as expected.
    verticalScrollBar()->setSingleStep( delegate()->itemSize().height() / 8 );
                                     // ^ scroll step is 1/8 of the estimated row height
#endif

    QListView::wheelEvent( e );
}


void
GridView::verifySize()
{
    if ( !autoResize() || !m_model )
        return;

#ifdef Q_WS_X11
//    int scrollbar = verticalScrollBar()->isVisible() ? verticalScrollBar()->width() + 16 : 0;
    int scrollbar = 0; verticalScrollBar()->rect().width();
#else
    int scrollbar = verticalScrollBar()->rect().width();
#endif

    const int rectWidth = contentsRect().width() - scrollbar - 3;
    const int itemWidth = 160;
    const int itemsPerRow = qMax( 1, qFloor( rectWidth / itemWidth ) );

    const int overlapRows = m_model->rowCount( QModelIndex() ) % itemsPerRow;
    const int rows = floor( (double)m_model->rowCount( QModelIndex() ) / (double)itemsPerRow );
    const int newHeight = rows * m_delegate->itemSize().height();

    m_proxyModel->setMaxVisibleItems( m_model->rowCount( QModelIndex() ) - overlapRows );

    if ( newHeight > 0 )
        setFixedHeight( newHeight );
}


void
GridView::onDelegatePlaying( const QPersistentModelIndex& index )
{
    m_playing = index;
}


void
GridView::onDelegateStopped( const QPersistentModelIndex& index )
{
    if ( m_playing == index )
        m_playing = QPersistentModelIndex();
}


void
GridView::layoutItems()
{
    if ( autoFitItems() && m_model )
    {
#ifdef Q_WS_X11
//        int scrollbar = verticalScrollBar()->isVisible() ? verticalScrollBar()->width() + 16 : 0;
        int scrollbar = 0; verticalScrollBar()->rect().width();
#else
        int scrollbar = verticalScrollBar()->rect().width();
#endif

        const int rectWidth = contentsRect().width() - scrollbar - 3;
        const int itemWidth = 160;
        const int itemsPerRow = qMax( 1, qFloor( rectWidth / itemWidth ) );

        const int remSpace = rectWidth - ( itemsPerRow * itemWidth );
        const int extraSpace = remSpace / itemsPerRow;
        const int newItemWidth = itemWidth + extraSpace;

        m_delegate->setItemSize( QSize( newItemWidth, newItemWidth ) );
        verifySize();

        if ( !m_inited )
        {
            m_inited = true;
            repaint();
        }
    }
}


void
GridView::onFilterChanged( const QString& )
{
    if ( selectedIndexes().count() )
        scrollTo( selectedIndexes().at( 0 ), QAbstractItemView::PositionAtCenter );
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

    if ( indexes.count() == 0 )
        return;

    qDebug() << "Dragging" << indexes.count() << "indexes";
    QMimeData* data = m_proxyModel->mimeData( indexes );
    if ( !data )
        return;

    QDrag* drag = new QDrag( this );
    drag->setMimeData( data );
    const QPixmap p = TomahawkUtils::createDragPixmap( TomahawkUtils::MediaTypeAlbum, indexes.count() );
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
    return proxyModel()->playlistInterface();
}


void
GridView::setPlaylistInterface( const Tomahawk::playlistinterface_ptr& playlistInterface )
{
    proxyModel()->setPlaylistInterface( playlistInterface );
}


#include "GridView.moc"
