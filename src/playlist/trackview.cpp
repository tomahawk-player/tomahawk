#include "trackview.h"

#include <QDebug>
#include <QKeyEvent>
#include <QPainter>
#include <QScrollBar>

#include "tomahawk/tomahawkapp.h"
#include "playlist/playlistmanager.h"
#include "playlist/queueview.h"
#include "audioengine.h"
#include "tomahawksettings.h"
#include "trackmodel.h"
#include "trackproxymodel.h"

using namespace Tomahawk;


TrackView::TrackView( QWidget* parent )
    : QTreeView( parent )
    , m_model( 0 )
    , m_proxyModel( 0 )
    , m_delegate( 0 )
    , m_header( new TrackHeader( this ) )
    , m_resizing( false )
{
    setSortingEnabled( false );
    setAlternatingRowColors( true );
    setSelectionMode( QAbstractItemView::ExtendedSelection );
    setSelectionBehavior( QAbstractItemView::SelectRows );
    setDragEnabled( true );
    setDropIndicatorShown( false );
    setDragDropMode( QAbstractItemView::InternalMove );
    setDragDropOverwriteMode( false );
    setAllColumnsShowFocus( true );
    setVerticalScrollMode( QAbstractItemView::ScrollPerPixel );
    setMinimumWidth( 690 );

    setHeader( m_header );

#ifndef Q_WS_WIN
    QFont f = font();
    f.setPointSize( f.pointSize() - 1 );
    setFont( f );
#endif

    connect( this, SIGNAL( doubleClicked( QModelIndex ) ), SLOT( onItemActivated( QModelIndex ) ) );
}


TrackView::~TrackView()
{
    qDebug() << Q_FUNC_INFO;
}


void
TrackView::setProxyModel( TrackProxyModel* model )
{
    m_proxyModel = model;
    m_delegate = new PlaylistItemDelegate( this, m_proxyModel );
    setItemDelegate( m_delegate );

    QTreeView::setModel( m_proxyModel );
}


void
TrackView::setModel( TrackModel* model )
{
    m_model = model;
    m_modelInterface = (PlaylistInterface*)model;

    if ( m_proxyModel )
        m_proxyModel->setSourceModel( model );

    connect( m_model, SIGNAL( itemSizeChanged( QModelIndex ) ), SLOT( onItemResized( QModelIndex ) ) );
    connect( m_proxyModel, SIGNAL( filterChanged( QString ) ), SLOT( onFilterChanged( QString ) ) );

    setAcceptDrops( true );
    setRootIsDecorated( false );
    setUniformRowHeights( true );
}


void
TrackView::onItemActivated( const QModelIndex& index )
{
    PlItem* item = m_model->itemFromIndex( m_proxyModel->mapToSource( index ) );
    if ( item && item->query()->numResults() )
    {
        qDebug() << "Result activated:" << item->query()->toString() << item->query()->results().first()->url();
        m_proxyModel->setCurrentItem( index );
        APP->audioEngine()->playItem( m_proxyModel, item->query()->results().first() );
    }
}


void
TrackView::keyPressEvent( QKeyEvent* event )
{
    qDebug() << Q_FUNC_INFO;
    QTreeView::keyPressEvent( event );

    if ( !model() )
        return;

    if ( event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return )
    {
        onItemActivated( currentIndex() );
    }
}


void
TrackView::onItemResized( const QModelIndex& index )
{
    qDebug() << Q_FUNC_INFO;
    m_delegate->updateRowSize( index );
}


void
TrackView::playItem()
{
    onItemActivated( m_contextMenuIndex );
}


void
TrackView::addItemsToQueue()
{
    foreach( const QModelIndex& idx, selectedIndexes() )
    {
        if ( idx.column() )
            continue;

        PlItem* item = model()->itemFromIndex( proxyModel()->mapToSource( idx ) );
        if ( item && item->query()->numResults() )
        {
            APP->playlistManager()->queue()->model()->appendTrack( item->query() );
            APP->playlistManager()->showQueue();
        }
    }
}


void
TrackView::resizeEvent( QResizeEvent* event )
{
    qDebug() << Q_FUNC_INFO;
    m_header->onResized();
}


void
TrackView::dragEnterEvent( QDragEnterEvent* event )
{
    qDebug() << Q_FUNC_INFO;
    QTreeView::dragEnterEvent( event );

    if ( event->mimeData()->hasFormat( "application/tomahawk.query.list" ) || event->mimeData()->hasFormat( "application/tomahawk.plentry.list" ) )
    {
        m_dragging = true;
        m_dropRect = QRect();

        qDebug() << "Accepting Drag Event";
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

    if ( event->mimeData()->hasFormat( "application/tomahawk.query.list" ) || event->mimeData()->hasFormat( "application/tomahawk.plentry.list" ) )
    {
        setDirtyRegion( m_dropRect );
        const QPoint pos = event->pos();
        const QModelIndex index = indexAt( pos );

        if ( index.isValid() )
        {
            const QRect rect = visualRect( index );
            m_dropRect = rect;

            // indicate that the item will be inserted above the current place
            const int gap = 5; // FIXME constant
            m_dropRect = QRect( rect.left(), rect.top() - gap / 2, rect.width(), gap );

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
        qDebug() << "Ignoring accepted event!";
    }
    else
        if ( event->mimeData()->hasFormat( "application/tomahawk.query.list" ) )
        {
            const QPoint pos = event->pos();
            const QModelIndex index = indexAt( pos );

            qDebug() << "Drop Event accepted at row:" << index.row();
            event->acceptProposedAction();

            if ( !model()->isReadOnly() )
            {
                model()->dropMimeData( event->mimeData(), event->proposedAction(), index.row(), 0, index.parent() );
            }
        }

    m_dragging = false;
}


void
TrackView::paintEvent( QPaintEvent* event )
{
    QTreeView::paintEvent( event );

    if ( m_dragging )
    {
        // draw drop indicator
        QPainter painter( viewport() );
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

    qDebug() << "Dragging" << indexes.count() << "indexes";
    QMimeData* data = m_proxyModel->mimeData( indexes );
    if ( !data )
        return;

    QDrag* drag = new QDrag( this );
    drag->setMimeData( data );
    const QPixmap p = createDragPixmap( indexes.count() );
    drag->setPixmap( p );
    drag->setHotSpot( QPoint( -20, -20 ) );

    Qt::DropAction action = drag->exec( supportedActions, Qt::CopyAction );
    if ( action == Qt::MoveAction )
    {
        m_proxyModel->removeIndexes( pindexes );
    }
}


// Inspired from dolphin's draganddrophelper.cpp
QPixmap
TrackView::createDragPixmap( int itemCount ) const
{
    // If more than one item is dragged, align the items inside a
    // rectangular grid. The maximum grid size is limited to 5 x 5 items.
    int xCount = 3;
    int size = 32;

    if ( itemCount > 16 )
    {
        xCount = 5;
        size = 16;
    } else if( itemCount > 9 )
    {
        xCount = 4;
        size = 22;
    }

    if( itemCount < xCount )
    {
        xCount = itemCount;
    }

    int yCount = itemCount / xCount;
    if( itemCount % xCount != 0 )
    {
        ++yCount;
    }
    if( yCount > xCount )
    {
        yCount = xCount;
    }
    // Draw the selected items into the grid cells
    QPixmap dragPixmap( xCount * size + xCount - 1, yCount * size + yCount - 1 );
    dragPixmap.fill( Qt::transparent );

    QPainter painter( &dragPixmap );
    painter.setRenderHint( QPainter::Antialiasing );
    int x = 0;
    int y = 0;
    for( int i = 0; i < itemCount; ++i )
    {
        const QPixmap pixmap = QPixmap( QString( ":/data/icons/audio-x-generic-%2x%2.png" ).arg( size ) );
        painter.drawPixmap( x, y, pixmap );

        x += size + 1;
        if ( x >= dragPixmap.width() )
        {
            x = 0;
            y += size + 1;
        }
        if ( y >= dragPixmap.height() )
        {
            break;
        }
    }

    return dragPixmap;
}


TrackHeader::TrackHeader( TrackView* parent )
    : QHeaderView( Qt::Horizontal, parent )
    , m_parent( parent )
    , m_init( false )
{
    setStretchLastSection( false );
    setResizeMode( QHeaderView::Interactive );
    setMinimumSectionSize( 60 );

    connect( this, SIGNAL( sectionResized( int, int, int ) ), SLOT( onSectionResized( int, int, int ) ) );
}


TrackHeader::~TrackHeader()
{
    saveColumnsState();
}


static uint negativeWidth = 0;

void
TrackHeader::onSectionResized( int logicalIndex, int oldSize, int newSize )
{
    qDebug() << Q_FUNC_INFO;

    if ( !m_init )
        return;

    blockSignals( true );

    if ( newSize < 0 )
        resizeSection( logicalIndex, 0 );

    for ( int i = logicalIndex + 1; i < count(); i++ )
    {
        int ns = sectionSize( i ) + oldSize - newSize;

        if ( ns < minimumSectionSize() )
        {
            resizeSection( logicalIndex, newSize - ( minimumSectionSize() - ns ) );
            ns = minimumSectionSize();
        }

        resizeSection( i, ns );
        break;
    }

    blockSignals( false );

    negativeWidth = 0;
    uint w = 0;

    for ( int x = 0; x < m_columnWeights.count(); x++ )
    {
        w += sectionSize( x );
        negativeWidth += sectionSize( x );
    }

    for ( int x = 0; x < m_columnWeights.count(); x++ )
    {
        m_columnWeights[x] = (double)sectionSize( x ) / double( w );
    }

    negativeWidth -= w;
}


void
TrackHeader::onResized()
{
    qDebug() << Q_FUNC_INFO;

    if ( !m_init && count() )
        restoreColumnsState();

    m_init = false;
    blockSignals( true );

    double width = m_parent->contentsRect().width();
#ifdef Q_WS_MAC
    if ( m_parent->verticalScrollBar() && m_parent->verticalScrollBar()->isVisible() )
    {
        width -= m_parent->verticalScrollBar()->width() + 1;
    }
#endif

    blockSignals( false );

    for ( int i = 0; i < m_columnWeights.count(); i++ )
    {
        if ( m_columnWeights[i] > 0 )
            resizeSection( i, int( width * m_columnWeights[i] ) );
    }

    m_init = true;
}


void
TrackHeader::restoreColumnsState()
{
    TomahawkSettings* s = APP->settings();
    QList<QVariant> list = s->playlistColumnSizes();

    qDebug() << "COOOOOOUNT:" << count() << list.count();
    if ( list.count() != count() ) // FIXME: const
    {
        m_columnWeights << 0.19 << 0.24 << 0.18 << 0.07 << 0.07 << 0.11 << 0.14;
    }
    else
    {
        foreach( const QVariant& v, list )
            m_columnWeights << v.toDouble();
    }
}


void
TrackHeader::saveColumnsState()
{
    TomahawkSettings *s = APP->settings();
    QList<QVariant> wlist;

    foreach( double w, m_columnWeights )
    {
        wlist << QVariant( w );
//        qDebug() << "Storing weight for column" << i++ << w;
    }

    s->setPlaylistColumnSizes( wlist );
}
