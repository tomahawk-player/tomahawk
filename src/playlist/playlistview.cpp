#include "playlistview.h"

#include <QDebug>
#include <QHeaderView>
#include <QKeyEvent>
#include <QPainter>
#include <QScrollBar>

#include "tomahawk/tomahawkapp.h"
#include "audioengine.h"
#include "playlistproxymodel.h"

#define PLAYLISTVIEW_FILTER_TIMEOUT 250
#include <tomahawksettings.h>

using namespace Tomahawk;


PlaylistView::PlaylistView( QWidget* parent )
    : ProgressTreeView( parent )
    , m_proxyModel( new PlaylistProxyModel( this ) )
    , m_model( 0 )
    , m_delegate( new PlaylistItemDelegate( this ) )
    , m_resizing( false )
{
    setRootIsDecorated( false );
    setSortingEnabled( false );
    setAlternatingRowColors( true );
    setUniformRowHeights( true );
    setDragEnabled( true );
    setAcceptDrops( true );
    setMouseTracking( true );
    setSelectionMode( QAbstractItemView::ExtendedSelection );
    setSelectionBehavior( QAbstractItemView::SelectRows );
    setDropIndicatorShown( false );
    setDragDropMode( QAbstractItemView::InternalMove );
    setDragDropOverwriteMode ( false );
    setAllColumnsShowFocus( true );

    setItemDelegate( m_delegate );

    header()->setMinimumSectionSize( 60 );
    restoreColumnsState();

    connect( this, SIGNAL( activated( QModelIndex ) ), SLOT( onItemActivated( QModelIndex ) ) );

    connect( header(), SIGNAL( sectionResized( int, int, int ) ), SLOT( onSectionResized( int, int, int ) ) );

    connect( APP->audioEngine(), SIGNAL( stopped() ), SLOT( onAudioStopped() ), Qt::QueuedConnection );

    connect( this, SIGNAL( itemStarted( PlaylistModelInterface*, PlaylistItem* ) ),
             APP->audioEngine(), SLOT( playItem( PlaylistModelInterface*, PlaylistItem* ) ), Qt::QueuedConnection );

    connect( this, SIGNAL( playlistModelChanged( PlaylistModelInterface* ) ),
             APP->audioEngine(), SLOT( onPlaylistActivated( PlaylistModelInterface* ) ), Qt::QueuedConnection );

    connect( m_proxyModel, SIGNAL( numSourcesChanged( unsigned int ) ),
                           SIGNAL( numSourcesChanged( unsigned int ) ) );

    connect( m_proxyModel, SIGNAL( numTracksChanged( unsigned int ) ),
                           SIGNAL( numTracksChanged( unsigned int ) ) );

    connect( m_proxyModel, SIGNAL( numArtistsChanged( unsigned int ) ),
                           SIGNAL( numArtistsChanged( unsigned int ) ) );

    connect( m_proxyModel, SIGNAL( numShownChanged( unsigned int ) ),
                           SIGNAL( numShownChanged( unsigned int ) ) );

    connect( &m_filterTimer, SIGNAL( timeout() ), SLOT( applyFilter() ) );

    QTreeView::setModel( m_proxyModel );
}


PlaylistView::~PlaylistView()
{
    qDebug() << Q_FUNC_INFO;

    saveColumnsState();
}


void
PlaylistView::setModel( PlaylistModel* model )
{
    m_model = model;
    m_model->setParent( m_proxyModel );
    m_proxyModel->setSourceModel( m_model );

    emit playlistModelChanged( m_proxyModel );
}


void
PlaylistView::restoreColumnsState()
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
PlaylistView::saveColumnsState()
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
PlaylistView::onSectionResized( int logicalIndex, int oldSize, int newSize )
{
    return;
}


void
PlaylistView::setFilter( const QString& filter )
{
    m_filter = filter;

    m_filterTimer.stop();
    m_filterTimer.setInterval( PLAYLISTVIEW_FILTER_TIMEOUT );
    m_filterTimer.setSingleShot( true );
    m_filterTimer.start();
}


void
PlaylistView::applyFilter()
{
    qDebug() << Q_FUNC_INFO;
    m_proxyModel->setFilterRegExp( m_filter );

    if ( selectedIndexes().count() )
        scrollTo( selectedIndexes().at( 0 ) );
}


void
PlaylistView::onItemActivated( const QModelIndex& index )
{
    PlaylistItem* item = PlaylistModel::indexToPlaylistItem( index );
    if ( item && item->query()->numResults() )
    {
        qDebug() << "PlaylistItem activated:" << item->query()->toString() << item->query()->results().at( 0 )->url();
        m_proxyModel->setCurrentItem( item->index() );
        emit itemStarted( m_proxyModel, item );
    }
}


void
PlaylistView::onAudioStopped()
{
    m_proxyModel->setCurrentItem( QModelIndex() );
}


void
PlaylistView::addSource( const source_ptr& source )
{
    qDebug() << "Adding source to PlaylistView:" << source->userName();

    m_proxyModel->addSource( source );
    emit playlistModelChanged( m_proxyModel );
    //emit sourceAdded( source );
}


void
PlaylistView::removeSource( const source_ptr& source )
{
    qDebug() << "Removing source from PlaylistView:" << source->userName();

    m_proxyModel->removeSource( source );
    emit playlistModelChanged( m_proxyModel );
    //emit sourceRemoved( source );
}


void
PlaylistView::resizeEvent( QResizeEvent* event )
{
//    qDebug() << Q_FUNC_INFO;
    resizeColumns();
}


void
PlaylistView::resizeColumns()
{
    double cw = contentsRect().width();
    int i = 0;
    int total = 0;

    if ( verticalScrollBar() && verticalScrollBar()->isVisible() )
    {
        cw -= verticalScrollBar()->width() + 1;
    }

    m_resizing = true;
    foreach( double w, m_columnWeights )
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
PlaylistView::keyPressEvent( QKeyEvent* event )
{
//    qDebug() << Q_FUNC_INFO;
    QTreeView::keyPressEvent( event );

    if ( !m_model )
        return;

    if ( event->key() == Qt::Key_Delete )
    {
        if ( m_model->isPlaylistBacked() && selectedIndexes().count() )
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
        }
    }
}


void
PlaylistView::dragEnterEvent( QDragEnterEvent* event )
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
PlaylistView::dragMoveEvent( QDragMoveEvent* event )
{
    QTreeView::dragMoveEvent( event );

    if ( m_model->isReadOnly() )
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
PlaylistView::dropEvent( QDropEvent* event )
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
PlaylistView::paintEvent( QPaintEvent* event )
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
