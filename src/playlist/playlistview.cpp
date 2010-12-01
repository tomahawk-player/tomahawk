#include "playlistview.h"

#include <QDebug>
#include <QKeyEvent>

#include "playlist/playlistproxymodel.h"

using namespace Tomahawk;


PlaylistView::PlaylistView( QWidget* parent )
    : TrackView( parent )
{
    setProxyModel( new PlaylistProxyModel( this ) );

    setContextMenuPolicy( Qt::CustomContextMenu );
    connect( this, SIGNAL( customContextMenuRequested( const QPoint& ) ), SLOT( onCustomContextMenu( const QPoint& ) ) );
}


PlaylistView::~PlaylistView()
{
    qDebug() << Q_FUNC_INFO;
}


void
PlaylistView::setModel( TrackModel* model )
{
    TrackView::setModel( model );

    // setColumnHidden( 5, true ); // Hide age column per default
}


void
PlaylistView::setupMenus()
{
    m_itemMenu.clear();

    unsigned int i = 0;
    foreach( const QModelIndex& idx, selectedIndexes() )
        if ( idx.column() == 0 )
            i++;

    m_playItemAction = m_itemMenu.addAction( tr( "&Play" ) );
    m_addItemsToQueueAction = m_itemMenu.addAction( tr( "Add to &Queue" ) );
    m_itemMenu.addSeparator();
    m_addItemsToPlaylistAction = m_itemMenu.addAction( tr( "&Add to Playlist" ) );
    m_itemMenu.addSeparator();
    m_deleteItemsAction = m_itemMenu.addAction( i > 1 ? tr( "&Delete Items" ) : tr( "&Delete Item" ) );

    if ( model() )
        m_deleteItemsAction->setEnabled( !model()->isReadOnly() );

    connect( m_playItemAction,           SIGNAL( triggered() ), SLOT( playItem() ) );
    connect( m_addItemsToQueueAction,    SIGNAL( triggered() ), SLOT( addItemsToQueue() ) );
    connect( m_addItemsToPlaylistAction, SIGNAL( triggered() ), SLOT( addItemsToPlaylist() ) );
    connect( m_deleteItemsAction,        SIGNAL( triggered() ), SLOT( deleteItems() ) );
}


void
PlaylistView::onCustomContextMenu( const QPoint& pos )
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
PlaylistView::keyPressEvent( QKeyEvent* event )
{
    qDebug() << Q_FUNC_INFO;
    TrackView::keyPressEvent( event );

    if ( !model() )
        return;

    if ( event->key() == Qt::Key_Delete )
    {
        qDebug() << "Removing selected items";
        proxyModel()->removeIndexes( selectedIndexes() );
    }
}


void
PlaylistView::addItemsToPlaylist()
{
}


void
PlaylistView::deleteItems()
{
    proxyModel()->removeIndexes( selectedIndexes() );
}
