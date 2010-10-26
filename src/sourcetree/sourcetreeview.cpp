#include "sourcetreeview.h"

#include "tomahawk/tomahawkapp.h"
#include "tomahawk/playlist.h"
#include "collectionmodel.h"
#include "playlistmanager.h"
#include "sourcetreeitem.h"
#include "sourcesmodel.h"

#include <QAction>
#include <QContextMenuEvent>
#include <QDragEnterEvent>
#include <QHeaderView>
#include <QPainter>

using namespace Tomahawk;


SourceTreeView::SourceTreeView( QWidget* parent )
    : QTreeView( parent )
    , m_collectionModel( new CollectionModel( this ) )
    , m_dragging( false )
{
    setHeaderHidden( true );
    setRootIsDecorated( false );
    setExpandsOnDoubleClick( false );

    setSelectionBehavior( QAbstractItemView::SelectRows );
    setDragDropMode( QAbstractItemView::DropOnly );
    setAcceptDrops( true );
    setDropIndicatorShown( false );
    setAllColumnsShowFocus( false );

    setContextMenuPolicy( Qt::CustomContextMenu );
    connect( this, SIGNAL( customContextMenuRequested( const QPoint& ) ), SLOT( onCustomContextMenu( const QPoint& ) ) );

    m_model = new SourcesModel( this );
    setModel( m_model );

    header()->setStretchLastSection( false );
    header()->setResizeMode( 0, QHeaderView::Stretch );

    connect( m_model, SIGNAL( clicked( QModelIndex ) ), SIGNAL( clicked( QModelIndex ) ) );
    connect( this,  SIGNAL( clicked( QModelIndex ) ), SLOT( onItemActivated( QModelIndex ) ) );

    connect( selectionModel(), SIGNAL( selectionChanged( const QItemSelection&, const QItemSelection& ) ), SLOT( onSelectionChanged() ) );
    connect( &APP->sourcelist(), SIGNAL( sourceRemoved( Tomahawk::source_ptr ) ), SLOT( onSourceOffline( Tomahawk::source_ptr ) ) );
}


void
SourceTreeView::setupMenus()
{
    m_playlistMenu.clear();

    m_loadPlaylistAction = m_playlistMenu.addAction( tr( "&Load Playlist" ) );
    m_playlistMenu.addSeparator();
    m_deletePlaylistAction = m_playlistMenu.addAction( tr( "&Delete Playlist" ) );

    bool readonly = true;
    int type = SourcesModel::indexType( m_contextMenuIndex );
    if ( type == 1 )
    {
        playlist_ptr playlist = SourcesModel::indexToPlaylist( m_contextMenuIndex );
        if ( !playlist.isNull() )
        {
            readonly = !playlist->author()->isLocal();
        }
    }

    if ( readonly )
    {
        m_deletePlaylistAction->setEnabled( !readonly );
    }

    connect( m_loadPlaylistAction,   SIGNAL( triggered() ), SLOT( loadPlaylist() ) );
    connect( m_deletePlaylistAction, SIGNAL( triggered() ), SLOT( deletePlaylist() ) );
}


void
SourceTreeView::onSourceOffline( Tomahawk::source_ptr src )
{
    qDebug() << Q_FUNC_INFO;
}


void
SourceTreeView::onItemActivated( const QModelIndex& index )
{
    if ( !index.isValid() )
        return;

    int type = SourcesModel::indexType( index );
    if ( type == 0 )
    {
        SourceTreeItem* item = SourcesModel::indexToTreeItem( index );
        if ( item )
        {
            if ( APP->playlistManager()->isSuperCollectionVisible() )
            {
                qDebug() << "SourceTreeItem toggled:" << item->source()->userName();
                APP->playlistManager()->show( item->source()->collection() );

                if ( APP->playlistManager()->superCollections().contains( item->source()->collection() ) )
                {
                    emit onOnline( index );
                }
                else
                {
                    emit onOffline( index );
                }
            }
        }
    }
    else if ( type == 1 )
    {
        playlist_ptr playlist = SourcesModel::indexToPlaylist( index );
        if ( !playlist.isNull() )
        {
            qDebug() << "Playlist activated:" << playlist->title();

            APP->playlistManager()->show( playlist );
        }
    }
}


void
SourceTreeView::onSelectionChanged()
{
    QModelIndexList si = selectedIndexes();

    foreach( const QModelIndex& idx, si )
    {
        int type = SourcesModel::indexType( idx );
        if ( type == 0 )
            selectionModel()->select( idx, QItemSelectionModel::Deselect );
    }
}


void
SourceTreeView::loadPlaylist()
{
    onItemActivated( m_contextMenuIndex );
}


void
SourceTreeView::deletePlaylist()
{
    qDebug() << Q_FUNC_INFO;

    QModelIndex idx = m_contextMenuIndex;
    if ( !idx.isValid() )
        return;

    int type = SourcesModel::indexType( idx );
    if ( type == 1 )
    {
        playlist_ptr playlist = SourcesModel::indexToPlaylist( idx );
        if ( !playlist.isNull() )
        {
            qDebug() << "Playlist about to be deleted:" << playlist->title();
            Playlist::remove( playlist );
        }
    }
}


void
SourceTreeView::onCustomContextMenu( const QPoint& pos )
{
    qDebug() << Q_FUNC_INFO;

    QModelIndex idx = m_contextMenuIndex = indexAt( pos );
    if ( !idx.isValid() )
        return;

    setupMenus();

    if ( SourcesModel::indexType( idx ) )
    {
        m_playlistMenu.exec( mapToGlobal( pos ) );
    }
}


void
SourceTreeView::dragEnterEvent( QDragEnterEvent* event )
{
    qDebug() << Q_FUNC_INFO;
    QTreeView::dragEnterEvent( event );

    if ( event->mimeData()->hasFormat( "application/tomahawk.query.list" ) )
    {
        m_dragging = true;
        m_dropRect = QRect();

        qDebug() << "Accepting Drag Event";
        event->setDropAction( Qt::CopyAction );
        event->accept();
    }
}


void
SourceTreeView::dragMoveEvent( QDragMoveEvent* event )
{
    bool accept = false;
    QTreeView::dragMoveEvent( event );

    if ( event->mimeData()->hasFormat( "application/tomahawk.query.list" ) )
    {
        setDirtyRegion( m_dropRect );
        const QPoint pos = event->pos();
        const QModelIndex index = indexAt( pos );

        if ( index.isValid() )
        {
            const QRect rect = visualRect( index );
            m_dropRect = rect;

            if ( SourcesModel::indexType( index ) == 1 )
            {
                playlist_ptr playlist = SourcesModel::indexToPlaylist( index );
                if ( !playlist.isNull() && playlist->author()->isLocal() )
                    accept = true;
            }
        } else {
            m_dropRect = QRect();
        }

        if ( accept )
        {
            event->setDropAction( Qt::CopyAction );
            event->accept();
        }
        else
            event->ignore();

        setDirtyRegion( m_dropRect );
    }
}


void
SourceTreeView::dropEvent( QDropEvent* event )
{
    bool accept = false;
    const QPoint pos = event->pos();
    const QModelIndex index = indexAt( pos );

    if ( event->mimeData()->hasFormat( "application/tomahawk.query.list" ) )
    {
        const QPoint pos = event->pos();
        const QModelIndex index = indexAt( pos );

        if ( index.isValid() )
        {
            if ( SourcesModel::indexType( index ) == 1 )
            {
                playlist_ptr playlist = SourcesModel::indexToPlaylist( index );
                if ( !playlist.isNull() && playlist->author()->isLocal() )
                {
                    accept = true;
                    QByteArray itemData = event->mimeData()->data( "application/tomahawk.query.list" );
                    QDataStream stream( &itemData, QIODevice::ReadOnly );
                    QList<Tomahawk::query_ptr> queries;

                    while ( !stream.atEnd() )
                    {
                        qlonglong qptr;
                        stream >> qptr;

                        Tomahawk::query_ptr* query = reinterpret_cast<Tomahawk::query_ptr*>(qptr);
                        if ( query && !query->isNull() )
                        {
                            qDebug() << "Dropped query item:" << query->data()->artist() << "-" << query->data()->track();
                            queries << *query;
                        }
                    }

                    qDebug() << "on playlist:" << playlist->title() << playlist->guid();

                    SourceTreeItem* treeItem = SourcesModel::indexToTreeItem( index );
                    if ( treeItem )
                    {
                        QString rev = treeItem->currentlyLoadedPlaylistRevision( playlist->guid() );
                        playlist->addEntries( queries, rev );
                    }
                }
            }
        }

        if ( accept )
        {
            event->setDropAction( Qt::CopyAction );
            event->accept();
        }
        else
            event->ignore();
    }

    QTreeView::dropEvent( event );
    m_dragging = false;
}


void
SourceTreeView::paintEvent( QPaintEvent* event )
{
    if ( m_dragging && !m_dropRect.isEmpty() )
    {
        QPainter painter( viewport() );
        const QModelIndex index = indexAt( m_dropRect.topLeft() );
        const QRect itemRect = visualRect( index );

        QStyleOptionViewItemV4 opt;
        opt.initFrom( this );
        opt.rect = itemRect;
        opt.state = QStyle::State_Enabled | QStyle::State_Selected;

        style()->drawPrimitive( QStyle::PE_PanelItemViewRow, &opt, &painter, this );
    }
    QTreeView::paintEvent( event );
}
