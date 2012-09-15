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

#include "TrackView.h"

#include <QKeyEvent>
#include <QPainter>
#include <QScrollBar>

#include "ViewHeader.h"
#include "ViewManager.h"
#include "PlayableModel.h"
#include "PlayableProxyModel.h"
#include "PlayableItem.h"
#include "audio/AudioEngine.h"
#include "context/ContextWidget.h"
#include "widgets/OverlayWidget.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"
#include "utils/Closure.h"
#include "DropJob.h"
#include "Artist.h"
#include "Album.h"
#include "Source.h"
#include "utils/AnimatedSpinner.h"

#define SCROLL_TIMEOUT 280

using namespace Tomahawk;


TrackView::TrackView( QWidget* parent )
    : QTreeView( parent )
    , m_model( 0 )
    , m_proxyModel( 0 )
    , m_delegate( 0 )
    , m_header( new ViewHeader( this ) )
    , m_overlay( new OverlayWidget( this ) )
    , m_loadingSpinner( new LoadingSpinner( this ) )
    , m_resizing( false )
    , m_dragging( false )
    , m_updateContextView( true )
    , m_contextMenu( new ContextMenu( this ) )
{
    setFrameShape( QFrame::NoFrame );
    setAttribute( Qt::WA_MacShowFocusRect, 0 );

    setContentsMargins( 0, 0, 0, 0 );
    setMouseTracking( true );
    setAlternatingRowColors( true );
    setSelectionMode( QAbstractItemView::ExtendedSelection );
    setSelectionBehavior( QAbstractItemView::SelectRows );
    setDragEnabled( true );
    setDropIndicatorShown( false );
    setDragDropMode( QAbstractItemView::InternalMove );
    setDragDropOverwriteMode( false );
    setAllColumnsShowFocus( true );
    setVerticalScrollMode( QAbstractItemView::ScrollPerPixel );
    setRootIsDecorated( false );
    setUniformRowHeights( true );

    setHeader( m_header );
    setSortingEnabled( true );
    sortByColumn( -1 );
    setContextMenuPolicy( Qt::CustomContextMenu );

    m_timer.setInterval( SCROLL_TIMEOUT );
    connect( verticalScrollBar(), SIGNAL( rangeChanged( int, int ) ), SLOT( onViewChanged() ) );
    connect( verticalScrollBar(), SIGNAL( valueChanged( int ) ), SLOT( onViewChanged() ) );
    connect( &m_timer, SIGNAL( timeout() ), SLOT( onScrollTimeout() ) );

    connect( this, SIGNAL( doubleClicked( QModelIndex ) ), SLOT( onItemActivated( QModelIndex ) ) );
    connect( this, SIGNAL( customContextMenuRequested( const QPoint& ) ), SLOT( onCustomContextMenu( const QPoint& ) ) );
    connect( m_contextMenu, SIGNAL( triggered( int ) ), SLOT( onMenuTriggered( int ) ) );

    setProxyModel( new PlayableProxyModel( this ) );
}


TrackView::~TrackView()
{
    tDebug() << Q_FUNC_INFO;
}


void
TrackView::setGuid( const QString& guid )
{
    m_guid = guid;
    m_header->setGuid( guid );
}


void
TrackView::setProxyModel( PlayableProxyModel* model )
{
    m_proxyModel = model;

    m_delegate = new PlaylistItemDelegate( this, m_proxyModel );
    setItemDelegate( m_delegate );

    QTreeView::setModel( m_proxyModel );
}


void
TrackView::setModel( QAbstractItemModel* model )
{
    Q_UNUSED( model );
    tDebug() << "Explicitly use setPlayableModel instead";
    Q_ASSERT( false );
}


void
TrackView::setPlayableModel( PlayableModel* model )
{
    m_model = model;

    if ( m_proxyModel )
    {
        m_proxyModel->setSourcePlayableModel( m_model );
    }

    connect( m_proxyModel, SIGNAL( filterChanged( QString ) ), SLOT( onFilterChanged( QString ) ) );
    connect( m_proxyModel, SIGNAL( rowsInserted( QModelIndex, int, int ) ), SLOT( onViewChanged() ) );

    setAcceptDrops( true );
    m_header->setDefaultColumnWeights( m_proxyModel->columnWeights() );

    switch( m_proxyModel->style() )
    {
        case PlayableProxyModel::Short:
        case PlayableProxyModel::ShortWithAvatars:
        case PlayableProxyModel::Large:
            setHeaderHidden( true );
            setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
        break;

        default:
            setHeaderHidden( false );
            setHorizontalScrollBarPolicy( Qt::ScrollBarAsNeeded );
    }

    emit modelChanged();
}


void
TrackView::setEmptyTip( const QString& tip )
{
    m_emptyTip = tip;
    m_overlay->setText( tip );
}


void
TrackView::onViewChanged()
{
    if ( m_proxyModel->style() != PlayableProxyModel::Short && m_proxyModel->style() != PlayableProxyModel::Large ) // eventual FIXME?
        return;

    if ( m_timer.isActive() )
        m_timer.stop();

    m_timer.start();
}


void
TrackView::onScrollTimeout()
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
        max = right.row();

    if ( !max )
        return;

    for ( int i = left.row(); i <= max; i++ )
    {
        m_proxyModel->updateDetailedInfo( m_proxyModel->index( i, 0 ) );
    }
}


void
TrackView::startPlayingFromStart()
{
    if ( m_proxyModel->rowCount() == 0 )
        return;

    const QModelIndex index = m_proxyModel->index( 0, 0 );
    startAutoPlay( index );
}


void
TrackView::autoPlayResolveFinished( const query_ptr& query, int row )
{
    Q_ASSERT( !query.isNull() );
    Q_ASSERT( row >= 0 );

    if ( query.isNull() || row < 0  || query != m_autoPlaying )
        return;

    const QModelIndex index = m_proxyModel->index( row, 0 );
    if ( query->playable() )
    {
        onItemActivated( index );
        return;
    }

    // Try the next one..
    const QModelIndex sib = index.sibling( index.row() + 1, index.column() );
    if ( sib.isValid() )
        startAutoPlay( sib );

}


void
TrackView::currentChanged( const QModelIndex& current, const QModelIndex& previous )
{
    QTreeView::currentChanged( current, previous );

    if ( !m_updateContextView )
        return;

    PlayableItem* item = m_model->itemFromIndex( m_proxyModel->mapToSource( current ) );
    if ( item )
    {
        ViewManager::instance()->context()->setQuery( item->query() );
    }
}


void
TrackView::onItemActivated( const QModelIndex& index )
{
    if ( !index.isValid() )
        return;

    tryToPlayItem( index );
    emit itemActivated( index );
}


void
TrackView::startAutoPlay( const QModelIndex& index )
{
    if ( tryToPlayItem( index ) )
        return;

    // item isn't playable but still resolving
    PlayableItem* item = m_model->itemFromIndex( m_proxyModel->mapToSource( index ) );
    if ( item && !item->query().isNull() && !item->query()->resolvingFinished() )
    {
        m_autoPlaying = item->query(); // So we can kill it if user starts autoplaying this playlist again
        NewClosure( item->query().data(), SIGNAL( resolvingFinished( bool ) ), this, SLOT( autoPlayResolveFinished( Tomahawk::query_ptr, int ) ),
                    item->query(), index.row() );
        return;
    }

    // not playable at all, try next
    const QModelIndex sib = index.sibling( index.row() + 1, index.column() );
    if ( sib.isValid() )
        startAutoPlay( sib );
}


bool
TrackView::tryToPlayItem( const QModelIndex& index )
{
    PlayableItem* item = m_model->itemFromIndex( m_proxyModel->mapToSource( index ) );
    if ( item && !item->query().isNull() )
    {
        m_proxyModel->setCurrentIndex( index );
        AudioEngine::instance()->playItem( m_proxyModel->playlistInterface(), item->query() );

        return true;
    }

    return false;
}


void
TrackView::keyPressEvent( QKeyEvent* event )
{
    QTreeView::keyPressEvent( event );

    if ( !model() )
        return;

    if ( event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return )
    {
        onItemActivated( currentIndex() );
    }
    if ( event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace )
    {
        tDebug() << "Removing selected items from playlist";
        deleteSelectedItems();
    }
}


void
TrackView::onItemResized( const QModelIndex& index )
{
    tDebug() << Q_FUNC_INFO;
    m_delegate->updateRowSize( index );
}


void
TrackView::playItem()
{
    onItemActivated( m_contextMenuIndex );
}


void
TrackView::resizeEvent( QResizeEvent* event )
{
    QTreeView::resizeEvent( event );

    int sortSection = m_header->sortIndicatorSection();
    Qt::SortOrder sortOrder = m_header->sortIndicatorOrder();

    if ( m_header->checkState() && sortSection >= 0 )
    {
        // restoreState keeps overwriting our previous sort-order
        sortByColumn( sortSection, sortOrder );
    }

    if ( !model() )
        return;

    if ( model()->columnCount() == 1 )
    {
        m_header->resizeSection( 0, event->size().width() );
    }
}


void
TrackView::dragEnterEvent( QDragEnterEvent* event )
{
    tDebug() << Q_FUNC_INFO;
    QTreeView::dragEnterEvent( event );

    if ( DropJob::acceptsMimeData( event->mimeData() ) )
    {
        m_dragging = true;
        m_dropRect = QRect();

        event->acceptProposedAction();
    }
}


void
TrackView::dragMoveEvent( QDragMoveEvent* event )
{
    QTreeView::dragMoveEvent( event );

    if ( model()->isReadOnly() )
    {
        event->ignore();
        return;
    }

    if ( DropJob::acceptsMimeData( event->mimeData() ) )
    {
        setDirtyRegion( m_dropRect );
        const QPoint pos = event->pos();
        QModelIndex index = indexAt( pos );
        bool pastLast = false;

        if ( !index.isValid() && proxyModel()->rowCount( QModelIndex() ) > 0 )
        {
            index = proxyModel()->index( proxyModel()->rowCount( QModelIndex() ) - 1, 0, QModelIndex() );
            pastLast = true;
        }

        if ( index.isValid() )
        {
            const QRect rect = visualRect( index );
            m_dropRect = rect;

            // indicate that the item will be inserted above the current place
            const int gap = 5; // FIXME constant
            int yHeight = ( pastLast ? rect.bottom() : rect.top() ) - gap / 2;
            m_dropRect = QRect( 0, yHeight, width(), gap );

            event->acceptProposedAction();
        }

        setDirtyRegion( m_dropRect );
    }
}


void
TrackView::dropEvent( QDropEvent* event )
{
    QTreeView::dropEvent( event );

    if ( event->isAccepted() )
    {
        tDebug() << "Ignoring accepted event!";
    }
    else
    {
        if ( DropJob::acceptsMimeData( event->mimeData()) )
        {
            const QPoint pos = event->pos();
            const QModelIndex index = indexAt( pos );

            tDebug() << Q_FUNC_INFO << "Drop Event accepted at row:" << index.row();
            event->acceptProposedAction();

            if ( !model()->isReadOnly() )
            {
                model()->dropMimeData( event->mimeData(), event->proposedAction(), index.row(), 0, index.parent() );
            }
        }
    }

    m_dragging = false;
}


void
TrackView::paintEvent( QPaintEvent* event )
{
    QTreeView::paintEvent( event );
    QPainter painter( viewport() );

    if ( m_dragging )
    {
        // draw drop indicator
        {
            // draw indicator for inserting items
            QBrush blendedBrush = viewOptions().palette.brush( QPalette::Normal, QPalette::Highlight );
            QColor color = blendedBrush.color();

            const int y = ( m_dropRect.top() + m_dropRect.bottom() ) / 2;
            const int thickness = m_dropRect.height() / 2;

            int alpha = 255;
            const int alphaDec = alpha / ( thickness + 1 );
            for ( int i = 0; i < thickness; i++ )
            {
                color.setAlpha( alpha );
                alpha -= alphaDec;
                painter.setPen( color );
                painter.drawLine( 0, y - i, width(), y - i );
                painter.drawLine( 0, y + i, width(), y + i );
            }
        }
    }
}


void
TrackView::onFilterChanged( const QString& )
{
    if ( selectedIndexes().count() )
        scrollTo( selectedIndexes().at( 0 ), QAbstractItemView::PositionAtCenter );

    if ( !filter().isEmpty() && !proxyModel()->playlistInterface()->trackCount() && model()->trackCount() )
    {
        m_overlay->setText( tr( "Sorry, your filter '%1' did not match any results." ).arg( filter() ) );
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
TrackView::startDrag( Qt::DropActions supportedActions )
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

    tDebug() << "Dragging" << indexes.count() << "indexes";
    QMimeData* data = m_proxyModel->mimeData( indexes );
    if ( !data )
        return;

    QDrag* drag = new QDrag( this );
    drag->setMimeData( data );
    const QPixmap p = TomahawkUtils::createDragPixmap( TomahawkUtils::MediaTypeTrack, indexes.count() );
    drag->setPixmap( p );
    drag->setHotSpot( QPoint( -20, -20 ) );

    Qt::DropAction action = drag->exec( supportedActions, Qt::CopyAction );
    if ( action == Qt::MoveAction )
    {
        m_proxyModel->removeIndexes( pindexes );
    }
}


void
TrackView::onCustomContextMenu( const QPoint& pos )
{
    m_contextMenu->clear();

    QModelIndex idx = indexAt( pos );
    idx = idx.sibling( idx.row(), 0 );
    setContextMenuIndex( idx );

    if ( !idx.isValid() )
        return;

    if ( model() && !model()->isReadOnly() )
        m_contextMenu->setSupportedActions( m_contextMenu->supportedActions() | ContextMenu::ActionDelete );

    QList<query_ptr> queries;
    foreach ( const QModelIndex& index, selectedIndexes() )
    {
        if ( index.column() )
            continue;

        PlayableItem* item = proxyModel()->itemFromIndex( proxyModel()->mapToSource( index ) );
        if ( item && !item->query().isNull() )
        {
            if ( item->query()->numResults() > 0 )
                queries << item->query()->results().first()->toQuery();
            else
                queries << item->query();
        }
    }

    m_contextMenu->setQueries( queries );
    m_contextMenu->exec( viewport()->mapToGlobal( pos ) );
}


void
TrackView::onMenuTriggered( int action )
{
    switch ( action )
    {
        case ContextMenu::ActionPlay:
            onItemActivated( m_contextMenuIndex );
            break;

        case ContextMenu::ActionDelete:
            deleteSelectedItems();
            break;

        default:
            break;
    }
}


void
TrackView::updateHoverIndex( const QPoint& pos )
{
    QModelIndex idx = indexAt( pos );

    if ( idx != m_hoveredIndex )
    {
        m_hoveredIndex = idx;
        repaint();
    }

    if ( !m_model || m_proxyModel->style() != PlayableProxyModel::Detailed )
        return;

    if ( idx.column() == PlayableModel::Artist || idx.column() == PlayableModel::Album || idx.column() == PlayableModel::Track )
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
TrackView::wheelEvent( QWheelEvent* event )
{
    QTreeView::wheelEvent( event );

    if ( m_hoveredIndex.isValid() )
    {
        m_hoveredIndex = QModelIndex();
        repaint();
    }
}


void
TrackView::leaveEvent( QEvent* event )
{
    QTreeView::leaveEvent( event );
    updateHoverIndex( QPoint( -1, -1 ) );
}


void
TrackView::mouseMoveEvent( QMouseEvent* event )
{
    QTreeView::mouseMoveEvent( event );
    updateHoverIndex( event->pos() );
}


void
TrackView::mousePressEvent( QMouseEvent* event )
{
    QTreeView::mousePressEvent( event );

    if ( !m_model || m_proxyModel->style() != PlayableProxyModel::Detailed )
        return;

    QModelIndex idx = indexAt( event->pos() );
    if ( event->pos().x() > header()->sectionViewportPosition( idx.column() ) + header()->sectionSize( idx.column() ) - 16 &&
         event->pos().x() < header()->sectionViewportPosition( idx.column() ) + header()->sectionSize( idx.column() ) )
    {
        PlayableItem* item = proxyModel()->itemFromIndex( proxyModel()->mapToSource( idx ) );
        switch ( idx.column() )
        {
            case PlayableModel::Artist:
            {
                ViewManager::instance()->show( Artist::get( item->query()->displayQuery()->artist() ) );
                break;
            }

            case PlayableModel::Album:
            {
                artist_ptr artist = Artist::get( item->query()->displayQuery()->artist() );
                ViewManager::instance()->show( Album::get( artist, item->query()->displayQuery()->album() ) );
                break;
            }

            case PlayableModel::Track:
            {
                ViewManager::instance()->show( item->query()->displayQuery() );
                break;
            }

            default:
                break;
        }
    }
}


Tomahawk::playlistinterface_ptr
TrackView::playlistInterface() const
{
    return proxyModel()->playlistInterface();
}


QString
TrackView::title() const
{
    return model()->title();
}


QString
TrackView::description() const
{
    return model()->description();
}


QPixmap
TrackView::pixmap() const
{
    return QPixmap( RESPATH "images/music-icon.png" );
}


bool
TrackView::jumpToCurrentTrack()
{
    scrollTo( proxyModel()->currentIndex(), QAbstractItemView::PositionAtCenter );
    return true;
}


bool
TrackView::setFilter( const QString& filter )
{
    ViewPage::setFilter( filter );
    m_proxyModel->setFilter( filter );
    return true;
}


void
TrackView::deleteSelectedItems()
{
    if ( !model()->isReadOnly() )
    {
        proxyModel()->removeIndexes( selectedIndexes() );
    }
    else
    {
        tDebug() << Q_FUNC_INFO << "Error: Model is read-only!";
    }
}
