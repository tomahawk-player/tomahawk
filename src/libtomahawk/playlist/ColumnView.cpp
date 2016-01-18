/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2015, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2014,      Teo Mrnjavac <teo@kde.org>
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

#include "ColumnView.h"

#include "audio/AudioEngine.h"
#include "utils/AnimatedSpinner.h"
#include "widgets/OverlayWidget.h"

#include "ContextMenu.h"
#include "TomahawkSettings.h"
#include "ViewHeader.h"
#include "ColumnItemDelegate.h"
#include "ColumnViewPreviewWidget.h"
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


ColumnView::ColumnView( QWidget* parent )
    : QColumnView( parent )
    , m_overlay( new OverlayWidget( this ) )
    , m_model( 0 )
    , m_proxyModel( 0 )
    , m_delegate( 0 )
    , m_loadingSpinner( new LoadingSpinner( this ) )
    , m_previewWidget( new ColumnViewPreviewWidget( this ) )
    , m_contextMenu( new ContextMenu( this ) )
    , m_scrollDelta( 0 )
{
    setFrameShape( QFrame::NoFrame );
    setAttribute( Qt::WA_MacShowFocusRect, 0 );

    setContentsMargins( 0, 0, 0, 0 );
    setMouseTracking( true );
    setAlternatingRowColors( true );
    setDragEnabled( true );
    setDropIndicatorShown( false );
    setDragDropOverwriteMode( false );
    setVerticalScrollMode( QAbstractItemView::ScrollPerPixel );
    setSelectionMode( QAbstractItemView::SingleSelection );
    setSelectionBehavior( QAbstractItemView::SelectRows );
    setContextMenuPolicy( Qt::CustomContextMenu );
    setProxyModel( new TreeProxyModel( this ) );
    setEditTriggers( NoEditTriggers );

    setPreviewWidget( m_previewWidget );

    m_timer.setInterval( SCROLL_TIMEOUT );
    connect( verticalScrollBar(), SIGNAL( sliderMoved( int ) ), SLOT( onViewChanged() ) );
    connect( verticalScrollBar(), SIGNAL( rangeChanged( int, int ) ), SLOT( onViewChanged() ) );
    connect( verticalScrollBar(), SIGNAL( valueChanged( int ) ), SLOT( onViewChanged() ) );
    connect( &m_timer, SIGNAL( timeout() ), SLOT( onScrollTimeout() ) );

    connect( this, SIGNAL( updatePreviewWidget( QModelIndex ) ), SLOT( onUpdatePreviewWidget( QModelIndex ) ) );
    connect( this, SIGNAL( doubleClicked( QModelIndex ) ), SLOT( onItemActivated( QModelIndex ) ) );
    connect( this, SIGNAL( customContextMenuRequested( QPoint ) ), SLOT( onCustomContextMenu( QPoint ) ) );
    connect( m_contextMenu, SIGNAL( triggered( int ) ), SLOT( onMenuTriggered( int ) ) );
}


ColumnView::~ColumnView()
{
    tDebug() << Q_FUNC_INFO;
}


void
ColumnView::setProxyModel( TreeProxyModel* model )
{
    m_proxyModel = model;
    m_delegate = new ColumnItemDelegate( this, m_proxyModel );
    setItemDelegate( m_delegate );

    QColumnView::setModel( m_proxyModel );
}


void
ColumnView::setModel( QAbstractItemModel* model )
{
    Q_UNUSED( model );
    tDebug() << "Explicitly use setTreeModel instead";
    Q_ASSERT( false );
}


void
ColumnView::setTreeModel( TreeModel* model )
{
    // HACK: without setting a new preview widget, the old preview widget is shown in the first column while loading the artists.
    m_previewWidget = new ColumnViewPreviewWidget( this );
    setPreviewWidget( m_previewWidget );

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
    connect( m_proxyModel, SIGNAL( rowsInserted( QModelIndex, int, int ) ), SLOT( fixScrollBars() ) );

    guid(); // this will set the guid on the header

    setHorizontalScrollBarPolicy( Qt::ScrollBarAsNeeded );

    connect( model, SIGNAL( changed() ), this, SIGNAL( modelChanged() ) );
    emit modelChanged();

/*    setColumnHidden( PlayableModel::Score, true ); // Hide score column per default
    setColumnHidden( PlayableModel::Origin, true ); // Hide origin column per default
    setColumnHidden( PlayableModel::Composer, true ); //Hide composer column per default

    setGuid( QString( "columnview/%1" ).arg( model->columnCount() ) );
    sortByColumn( PlayableModel::Artist, Qt::AscendingOrder );*/

    QList< int > widths;
    int baseUnit = m_previewWidget->minimumSize().width() + 32;
    widths << baseUnit << baseUnit << baseUnit << baseUnit;
    setColumnWidths( widths );
}


void
ColumnView::setEmptyTip( const QString& tip )
{
    m_emptyTip = tip;
    m_overlay->setText( tip );
}


bool
ColumnView::setFilter( const QString& filter )
{
    proxyModel()->setFilter( filter );
    return true;
}


void
ColumnView::onViewChanged()
{
    if ( m_timer.isActive() )
        m_timer.stop();

    m_timer.start();
}


void
ColumnView::onScrollTimeout()
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
ColumnView::currentChanged( const QModelIndex& current, const QModelIndex& previous )
{
    QColumnView::currentChanged( current, previous );
}


void
ColumnView::onItemActivated( const QModelIndex& index )
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
ColumnView::keyPressEvent( QKeyEvent* event )
{
    QColumnView::keyPressEvent( event );

    if ( !model() )
        return;

    if ( event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return )
    {
        onItemActivated( currentIndex() );
    }
}


void
ColumnView::resizeEvent( QResizeEvent* event )
{
    QColumnView::resizeEvent( event );
}


void
ColumnView::wheelEvent( QWheelEvent* event )
{
    QColumnView::wheelEvent( event );

    m_delegate->resetHoverIndex();
    repaint();
}


void
ColumnView::onFilterChangeFinished()
{
    if ( !selectedIndexes().isEmpty() )
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
ColumnView::onFilteringStarted()
{
    m_overlay->hide();
    m_loadingSpinner->fadeIn();
}


void
ColumnView::startDrag( Qt::DropActions supportedActions )
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
ColumnView::onCustomContextMenu( const QPoint& pos )
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

    QModelIndexList indexes = selectedIndexes();
    if ( !indexes.contains( idx ) )
    {
        indexes.clear();
        indexes << idx;
    }

    foreach ( const QModelIndex& index, indexes )
    {
        if ( index.column() || indexes.contains( index.parent() ) )
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
ColumnView::onMenuTriggered( int action )
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
ColumnView::jumpToCurrentTrack()
{
    if ( !m_proxyModel || !m_proxyModel->sourceModel() )
        return false;

    scrollTo( m_proxyModel->currentIndex(), QAbstractItemView::PositionAtCenter );
    return true;
}


QString
ColumnView::guid() const
{
    if ( m_guid.isEmpty() )
    {
        m_guid = QString( "columnview/%1" ).arg( m_model->columnCount( QModelIndex() ) );
    }

    return m_guid;
}


void
ColumnView::fixScrollBars()
{
    foreach ( QObject* widget, children() )
    {
        foreach ( QObject* view, widget->children() )
        {
            QScrollBar* sb = qobject_cast< QScrollBar* >( view );
            if ( sb && sb->orientation() == Qt::Horizontal )
            {
                sb->setSingleStep( 6 );
            }

            foreach ( QObject* subviews, view->children() )
            {
                foreach ( QObject* scrollbar, subviews->children() )
                {
                    QScrollBar* sb = qobject_cast< QScrollBar* >( scrollbar );
                    if ( sb && sb->orientation() == Qt::Vertical )
                    {
                        sb->setSingleStep( 6 );
                        connect( sb, SIGNAL( sliderMoved( int ) ), SLOT( onViewChanged() ) );
                        connect( sb, SIGNAL( rangeChanged( int, int ) ), SLOT( onViewChanged() ) );
                        connect( sb, SIGNAL( valueChanged( int ) ), SLOT( onViewChanged() ) );


                        break;
                    }
                }
            }
        }
    }
}


void
ColumnView::onUpdatePreviewWidget( const QModelIndex& index )
{
    fixScrollBars();

    PlayableItem* item = m_proxyModel->itemFromIndex( m_proxyModel->mapToSource( index ) );
    if ( !item || !item->result() )
    {
        QList< int > widths = columnWidths();
        QList< int > finalWidths;
        foreach ( int w, widths )
        {
            finalWidths << qMax( m_previewWidget->minimumSize().width() + 32, w );
        }
        setColumnWidths( finalWidths );

        return;
    }

    m_previewWidget->setQuery( item->result()->toQuery() );

    QList< int > widths = columnWidths();
    int previewWidth = viewport()->width();
    // Sometimes we do not have 3 columns because of wrong usage of the ColumnView.
    // At least do not crash.
    for (int i = 0; i < 3 && i < widths.length(); i++ ) {
        previewWidth -= widths.at( i );
    }
    widths.removeLast();
    widths << qMax( previewWidth, m_previewWidget->minimumSize().width() + 32 );
    setColumnWidths( widths );
}
