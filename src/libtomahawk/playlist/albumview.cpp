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

#include "albumview.h"

#include <QHeaderView>
#include <QKeyEvent>
#include <QPainter>
#include <QScrollBar>
#include <qmath.h>

#include "audio/audioengine.h"
#include "tomahawksettings.h"
#include "artist.h"
#include "albumitem.h"
#include "albumitemdelegate.h"
#include "albummodel.h"
#include "viewmanager.h"
#include "utils/logger.h"
#include "dynamic/widgets/LoadingSpinner.h"

#define SCROLL_TIMEOUT 280

using namespace Tomahawk;


AlbumView::AlbumView( QWidget* parent )
    : QListView( parent )
    , m_model( 0 )
    , m_proxyModel( 0 )
    , m_delegate( 0 )
    , m_loadingSpinner( new LoadingSpinner( this ) )
    , m_overlay( new OverlayWidget( this ) )
    , m_inited( false )
{
    setDragEnabled( true );
    setDropIndicatorShown( false );
    setDragDropOverwriteMode( false );
    setUniformItemSizes( true );
    setSpacing( 16 );
    setContentsMargins( 0, 0, 0, 0 );
    setMouseTracking( true );

    setResizeMode( Adjust );
    setViewMode( IconMode );
    setVerticalScrollMode( QAbstractItemView::ScrollPerPixel );

    setAutoFitItems( true );
    setProxyModel( new AlbumProxyModel( this ) );

    m_timer.setInterval( SCROLL_TIMEOUT );

    connect( verticalScrollBar(), SIGNAL( rangeChanged( int, int ) ), SLOT( onViewChanged() ) );
    connect( verticalScrollBar(), SIGNAL( valueChanged( int ) ), SLOT( onViewChanged() ) );
    connect( &m_timer, SIGNAL( timeout() ), SLOT( onScrollTimeout() ) );

    connect( this, SIGNAL( doubleClicked( QModelIndex ) ), SLOT( onItemActivated( QModelIndex ) ) );
}


AlbumView::~AlbumView()
{
    qDebug() << Q_FUNC_INFO;
}


void
AlbumView::setProxyModel( AlbumProxyModel* model )
{
    m_proxyModel = model;
    m_delegate = new AlbumItemDelegate( this, m_proxyModel );
    connect( m_delegate, SIGNAL( updateIndex( QModelIndex ) ), this, SLOT( update( QModelIndex ) ) );
    setItemDelegate( m_delegate );

    QListView::setModel( m_proxyModel );
}


void
AlbumView::setModel( QAbstractItemModel* model )
{
    Q_UNUSED( model );
    qDebug() << "Explicitly use setAlbumModel instead";
    Q_ASSERT( false );
}


void
AlbumView::setAlbumModel( AlbumModel* model )
{
    m_model = model;

    if ( m_proxyModel )
    {
        m_proxyModel->setSourceAlbumModel( m_model );
        m_proxyModel->sort( 0 );
    }

    connect( m_proxyModel, SIGNAL( filterChanged( QString ) ), SLOT( onFilterChanged( QString ) ) );
    connect( m_proxyModel, SIGNAL( rowsInserted( QModelIndex, int, int ) ), SLOT( onViewChanged() ) );

    connect( m_model, SIGNAL( itemCountChanged( unsigned int ) ), SLOT( onItemCountChanged( unsigned int ) ) );
    connect( m_model, SIGNAL( loadingStarted() ), m_loadingSpinner, SLOT( fadeIn() ) );
    connect( m_model, SIGNAL( loadingFinished() ), m_loadingSpinner, SLOT( fadeOut() ) );

    onViewChanged(); // Fetch covers if albums were added to model before model was attached to view
}


void
AlbumView::onItemActivated( const QModelIndex& index )
{
    AlbumItem* item = m_model->itemFromIndex( m_proxyModel->mapToSource( index ) );
    if ( item )
    {
//        qDebug() << "Result activated:" << item->album()->tracks().first()->toString() << item->album()->tracks().first()->results().first()->url();
//        APP->audioEngine()->playItem( item->album().data(), item->album()->tracks().first()->results().first() );

        if ( !item->album().isNull() )
            ViewManager::instance()->show( item->album() );
        else if ( !item->artist().isNull() )
            ViewManager::instance()->show( item->artist() );
    }
}


void
AlbumView::onViewChanged()
{
    if ( m_timer.isActive() )
        m_timer.stop();

    m_timer.start();
}


void
AlbumView::onItemCountChanged( unsigned int items )
{
    if ( items == 0 )
    {
        if ( m_model->collection().isNull() || ( !m_model->collection().isNull() && m_model->collection()->source()->isLocal() ) )
            m_overlay->setText( tr( "After you have scanned your music collection you will find your latest album additions right here." ) );
        else
            m_overlay->setText( tr( "This collection doesn't have any recent albums." ) );

        m_overlay->show();
    }
    else
        m_overlay->hide();
}


void
AlbumView::onScrollTimeout()
{
    if ( m_timer.isActive() )
        m_timer.stop();

    if ( !m_proxyModel->rowCount() )
        return;

    QRect viewRect = viewport()->rect();
    int rowHeight = m_proxyModel->data( QModelIndex(), Qt::SizeHintRole ).toSize().height();
    viewRect.adjust( 0, -rowHeight, 0, rowHeight );

    bool started = false;
    bool done = false;
    for ( int i = 0; i < m_proxyModel->rowCount(); i++ )
    {
        if ( started && done )
            break;

        for ( int j = 0; j < m_proxyModel->columnCount(); j++ )
        {
            QModelIndex idx = m_proxyModel->index( i, j );
            if ( !viewRect.contains( visualRect( idx ) ) )
            {
                done = true;
                break;
            }

            started = true;
            done = false;

            if ( !m_model->getCover( m_proxyModel->mapToSource( idx ) ) )
                break;
        }
    }
}


void
AlbumView::paintEvent( QPaintEvent* event )
{
    if ( m_inited )
        QListView::paintEvent( event );
}


void
AlbumView::resizeEvent( QResizeEvent* event )
{
    QListView::resizeEvent( event );

    m_inited = true;
    if ( !autoFitItems() )
        return;

#ifdef Q_WS_X11
    int scrollbar = !verticalScrollBar()->isVisible() ? verticalScrollBar()->rect().width() : 0;
#else
    int scrollbar = verticalScrollBar()->rect().width();
#endif
    int rectWidth = contentsRect().width() - scrollbar - 16 - 3;
    QSize itemSize = m_proxyModel->data( QModelIndex(), Qt::SizeHintRole ).toSize();

    int itemsPerRow = qFloor( rectWidth / ( itemSize.width() + 16 ) );
    int rightSpacing = rectWidth - ( itemsPerRow * ( itemSize.width() + 16 ) );
    int newSpacing = 16 + floor( rightSpacing / ( itemsPerRow + 1 ) );

    if ( itemsPerRow < 1 )
        setSpacing( 16 );
    else
        setSpacing( newSpacing );
}


void
AlbumView::onFilterChanged( const QString& )
{
    if ( selectedIndexes().count() )
        scrollTo( selectedIndexes().at( 0 ), QAbstractItemView::PositionAtCenter );
}


void
AlbumView::startDrag( Qt::DropActions supportedActions )
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
