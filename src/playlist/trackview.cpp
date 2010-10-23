#include "trackview.h"

#include <QDebug>
#include <QHeaderView>
#include <QKeyEvent>
#include <QPainter>
#include <QScrollBar>

#include "tomahawk/tomahawkapp.h"
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
    , m_resizing( false )
{
    setSortingEnabled( false );
    setAlternatingRowColors( true );
    setSelectionMode( QAbstractItemView::ExtendedSelection );
    setSelectionBehavior( QAbstractItemView::SelectRows );
    setDragEnabled( true );
    setDropIndicatorShown( false );
    setDragDropMode( QAbstractItemView::InternalMove );
    setDragDropOverwriteMode ( false );
    setAllColumnsShowFocus( true );

    header()->setMinimumSectionSize( 60 );
    restoreColumnsState();

    connect( this, SIGNAL( activated( QModelIndex ) ), SLOT( onItemActivated( QModelIndex ) ) );
    connect( header(), SIGNAL( sectionResized( int, int, int ) ), SLOT( onSectionResized( int, int, int ) ) );
}


TrackView::~TrackView()
{
    qDebug() << Q_FUNC_INFO;

    saveColumnsState();
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
TrackView::restoreColumnsState()
{
    TomahawkSettings* s = APP->settings();
    QList<QVariant> list = s->playlistColumnSizes();

    if ( list.count() != 6 ) // FIXME: const
    {
        m_columnWeights << 0.22 << 0.29 << 0.19 << 0.08 << 0.08 << 0.14;
    }
    else
    {
        foreach( const QVariant& v, list )
            m_columnWeights << v.toDouble();
    }

    for ( int i = 0; i < m_columnWeights.count(); i++ )
        m_columnWidths << 0;
}


void
TrackView::saveColumnsState()
{
    TomahawkSettings *s = APP->settings();
    QList<QVariant> wlist;
//    int i = 0;

    foreach( double w, m_columnWeights )
    {
        wlist << QVariant( w );
//        qDebug() << "Storing weight for column" << i++ << w;
    }

    s->setPlaylistColumnSizes( wlist );
}


void
TrackView::onSectionResized( int logicalIndex, int oldSize, int newSize )
{
    return;
}


void
TrackView::onItemActivated( const QModelIndex& index )
{
    PlItem* item = ((PlaylistInterface*)m_model)->itemFromIndex( m_proxyModel->mapToSource( index ) );
    if ( item && item->query()->numResults() )
    {
        qDebug() << "Result activated:" << item->query()->toString() << item->query()->results().first()->url();
        m_proxyModel->setCurrentItem( index );
        APP->audioEngine()->playItem( m_proxyModel, item->query()->results().first() );
    }
}


void
TrackView::onItemResized( const QModelIndex& index )
{
    qDebug() << Q_FUNC_INFO;
    m_delegate->updateRowSize( index );
}


void
TrackView::resizeEvent( QResizeEvent* event )
{
//    qDebug() << Q_FUNC_INFO;
    resizeColumns();
}


void
TrackView::resizeColumns()
{
    double cw = contentsRect().width();
    int i = 0;
    int total = 0;

    QList<double> mcw = m_columnWeights;

    if ( verticalScrollBar() && verticalScrollBar()->isVisible() )
    {
        cw -= verticalScrollBar()->width() + 1;
    }

    m_resizing = true;
    foreach( double w, mcw )
    {
        int fw = (int)( cw * w );
        if ( fw < header()->minimumSectionSize() )
            fw = header()->minimumSectionSize();

        if ( i + 1 == header()->count() )
            fw = cw - total;

        total += fw;
//        qDebug() << "Resizing column:" << i << fw;

        m_columnWidths[ i ] = fw;

        header()->resizeSection( i++, fw );
    }
    m_resizing = false;
}


void
TrackView::keyPressEvent( QKeyEvent* event )
{
//    qDebug() << Q_FUNC_INFO;
    QTreeView::keyPressEvent( event );

    if ( !m_model )
        return;

    if ( event->key() == Qt::Key_Delete )
    {
/*        if ( m_model->isPlaylistBacked() && selectedIndexes().count() )
        {
            qDebug() << "Removing selected items";
            QList<PlaylistItem*> items;

            QModelIndexList sidxs = selectedIndexes();
            foreach( const QModelIndex& idx, sidxs )
            {
                if ( idx.column() > 0 )
                    continue;

                PlaylistItem* item = PlaylistModel::indexToPlaylistItem( idx );
                if ( item )
                    items << item;
            }

            m_model->removeItems( items );
        }*/
    }
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
/*    const QPoint pos = event->pos();
    const QModelIndex index = indexAt( pos );

    if ( event->mimeData()->hasFormat( "application/tomahawk.query.list" ) )
    {
        const QPoint pos = event->pos();
        const QModelIndex index = indexAt( pos );

        if ( index.isValid() )
        {
            event->acceptProposedAction();
        }
    }*/

    QTreeView::dropEvent( event );
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
    QModelIndexList indexes = selectedIndexes();
    qDebug() << "Dragging" << indexes.count() << "indexes";
    for( int i = indexes.count() - 1 ; i >= 0; --i )
    {
        if( !( m_proxyModel->flags( indexes.at( i ) ) & Qt::ItemIsDragEnabled ) )
            indexes.removeAt( i );
    }

    if( indexes.count() == 0 )
        return;

    qDebug() << "Dragging" << indexes.count() << "indexes";

    QMimeData *data = m_proxyModel->mimeData( indexes );
    if ( !data )
        return;

    QDrag *drag = new QDrag( this );
    drag->setMimeData( data );
    const QPixmap p = createDragPixmap( indexes.count() );
    drag->setPixmap( p );
    drag->setHotSpot( QPoint( -20, -20 ) );

    // NOTE: if we support moving items in the model
    //       in the future, if exec() returns Qt::MoveAction
    //       we need to clean up ourselves.
    drag->exec( supportedActions, Qt::CopyAction );
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

    QPainter painter(&dragPixmap);
    painter.setRenderHint( QPainter::Antialiasing );
    int x = 0;
    int y = 0;
    for( int i = 0; i < itemCount; ++i )
    {
        const QPixmap pixmap = QPixmap( QString( ":/data/icons/audio-x-generic-%2.png" ).arg( size ) );
        painter.drawPixmap(x, y, pixmap);

        x += size + 1;
        if (x >= dragPixmap.width())
        {
            x = 0;
            y += size + 1;
        }
        if (y >= dragPixmap.height())
        {
            break;
        }
    }

    return dragPixmap;
}
