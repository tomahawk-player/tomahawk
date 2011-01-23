#include "collectionview.h"

#include <QDebug>
#include <QDragEnterEvent>
#include <QPainter>

#include "playlist/collectionproxymodel.h"
#include "widgets/overlaywidget.h"

using namespace Tomahawk;


CollectionView::CollectionView( QWidget* parent )
    : TrackView( parent )
{
    setProxyModel( new CollectionProxyModel( this ) );

    setSortingEnabled( true );
    sortByColumn( 0, Qt::AscendingOrder );

    setDragDropMode( QAbstractItemView::DragOnly );
    setAcceptDrops( false );

    setContextMenuPolicy( Qt::CustomContextMenu );
    connect( this, SIGNAL( customContextMenuRequested( const QPoint& ) ), SLOT( onCustomContextMenu( const QPoint& ) ) );
}


CollectionView::~CollectionView()
{
    qDebug() << Q_FUNC_INFO;
}


void
CollectionView::dragEnterEvent( QDragEnterEvent* event )
{
    qDebug() << Q_FUNC_INFO;
    event->ignore();
}


void
CollectionView::setupMenus()
{
    m_itemMenu.clear();

    m_playItemAction = m_itemMenu.addAction( tr( "&Play" ) );
    m_addItemsToQueueAction = m_itemMenu.addAction( tr( "Add to &Queue" ) );
    m_itemMenu.addSeparator();
    m_addItemsToPlaylistAction = m_itemMenu.addAction( tr( "&Add to Playlist" ) );

    connect( m_playItemAction,           SIGNAL( triggered() ), SLOT( playItem() ) );
    connect( m_addItemsToQueueAction,    SIGNAL( triggered() ), SLOT( addItemsToQueue() ) );
    connect( m_addItemsToPlaylistAction, SIGNAL( triggered() ), SLOT( addItemsToPlaylist() ) );
}


void
CollectionView::onCustomContextMenu( const QPoint& pos )
{
    qDebug() << Q_FUNC_INFO;
    setupMenus();

    QModelIndex idx = indexAt( pos );
    idx = idx.sibling( idx.row(), 0 );
    setContextMenuIndex( idx );

    if ( !idx.isValid() )
        return;

    m_itemMenu.exec( mapToGlobal( pos ) );
}


void
CollectionView::paintEvent( QPaintEvent* event )
{
    TrackView::paintEvent( event );
    QPainter painter( viewport() );

    if ( !model()->trackCount() )
    {
        overlay()->setText( tr( "This collection is empty." ) );
        overlay()->paint( &painter );
    }
}
