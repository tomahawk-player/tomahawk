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
CollectionView::setModel( TrackModel* model )
{
    TrackView::setModel( model );
    setGuid( "collectionview" );

    connect( model, SIGNAL( trackCountChanged( unsigned int ) ), SLOT( onTrackCountChanged( unsigned int ) ) );
}


void
CollectionView::dragEnterEvent( QDragEnterEvent* event )
{
    event->ignore();
}


void
CollectionView::setupMenus()
{
    m_itemMenu.clear();

    m_playItemAction = m_itemMenu.addAction( tr( "&Play" ) );
    m_addItemsToQueueAction = m_itemMenu.addAction( tr( "Add to &Queue" ) );
//    m_itemMenu.addSeparator();
//    m_addItemsToPlaylistAction = m_itemMenu.addAction( tr( "&Add to Playlist" ) );

    connect( m_playItemAction,           SIGNAL( triggered() ), SLOT( playItem() ) );
    connect( m_addItemsToQueueAction,    SIGNAL( triggered() ), SLOT( addItemsToQueue() ) );
//    connect( m_addItemsToPlaylistAction, SIGNAL( triggered() ), SLOT( addItemsToPlaylist() ) );
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
CollectionView::onTrackCountChanged( unsigned int tracks )
{
    if ( tracks == 0 )
    {
        overlay()->setText( tr( "This collection is empty." ) );
        overlay()->show();
    }
    else
        overlay()->hide();
}


bool
CollectionView::jumpToCurrentTrack()
{
    scrollTo( proxyModel()->currentItem(), QAbstractItemView::PositionAtCenter );
}
