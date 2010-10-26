#include "playlistview.h"

#include <QDebug>
#include <QKeyEvent>

#include "playlist/playlistproxymodel.h"

using namespace Tomahawk;


PlaylistView::PlaylistView( QWidget* parent )
    : TrackView( parent )
{
    setProxyModel( new PlaylistProxyModel( this ) );
    setupMenus();
}


PlaylistView::~PlaylistView()
{
    qDebug() << Q_FUNC_INFO;
}


void
PlaylistView::setupMenus()
{
    m_playItemAction = m_itemMenu.addAction( tr( "&Play" ) );
    m_itemMenu.addSeparator();
    m_addItemsToPlaylistAction = m_itemMenu.addAction( tr( "&Add to Playlist" ) );
    m_itemMenu.addSeparator();
    m_deleteItemAction = m_itemMenu.addAction( tr( "&Delete Item" ) );

    connect( m_playItemAction,           SIGNAL( triggered() ), SLOT( playItem() ) );
    connect( m_addItemsToPlaylistAction, SIGNAL( triggered() ), SLOT( addItemsToPlaylist() ) );
    connect( m_deleteItemAction,         SIGNAL( triggered() ), SLOT( deleteItem() ) );

    setContextMenuPolicy( Qt::CustomContextMenu );
    connect( this, SIGNAL( customContextMenuRequested( const QPoint& ) ), SLOT( onCustomContextMenu( const QPoint& ) ) );
}


void
PlaylistView::onCustomContextMenu( const QPoint& pos )
{
    qDebug() << Q_FUNC_INFO;

    QModelIndex idx = indexAt( pos );
    idx = idx.sibling( idx.row(), 0 );
    m_contextMenuIndex = idx;

    if ( !idx.isValid() )
        return;

    m_itemMenu.exec( mapToGlobal( pos ) );
}


void
PlaylistView::keyPressEvent( QKeyEvent* event )
{
    qDebug() << Q_FUNC_INFO;
    QTreeView::keyPressEvent( event );

    if ( !model() )
        return;

    if ( event->key() == Qt::Key_Delete )
    {
        qDebug() << "Removing selected items";
        proxyModel()->removeIndexes( selectedIndexes() );
    }
}


void
PlaylistView::playItem()
{
    onItemActivated( m_contextMenuIndex );
}


void
PlaylistView::addItemToPlaylist()
{
}


void
PlaylistView::deleteItem()
{
    proxyModel()->removeIndex( m_contextMenuIndex );
}
