#include "sourcetreeview.h"

#include "playlist.h"
#include "playlist/collectionmodel.h"
#include "playlist/playlistmanager.h"
#include "sourcetreeitem.h"
#include "sourcesmodel.h"
#include "sourcelist.h"
#include "tomahawk/tomahawkapp.h"

#include <QAction>
#include <QContextMenuEvent>
#include <QDragEnterEvent>
#include <QHeaderView>
#include <QPainter>
#include <QStyledItemDelegate>

using namespace Tomahawk;


class SourceDelegate : public QStyledItemDelegate
{
public:
    SourceDelegate( QAbstractItemView* parent = 0 ) : QStyledItemDelegate( parent ), m_parent( parent ) {}

protected:
    void paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const;
    void updateEditorGeometry( QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index ) const
    {
        if ( SourcesModel::indexType( index ) == SourcesModel::PlaylistSource || SourcesModel::indexType( index ) == SourcesModel::DynamicPlaylistSource )
            editor->setGeometry( option.rect.adjusted( 32, 0, 0, 0 ) );
        else
            QStyledItemDelegate::updateEditorGeometry( editor, option, index );
    }

private:
    QAbstractItemView* m_parent;
};


SourceTreeView::SourceTreeView( QWidget* parent )
    : QTreeView( parent )
    , m_collectionModel( new CollectionModel( this ) )
    , m_dragging( false )
{
    setFrameShape( QFrame::NoFrame );
    setContentsMargins( 0, 0, 0, 0 );

    setHeaderHidden( true );
    setRootIsDecorated( false );
    setExpandsOnDoubleClick( false );

    setSelectionBehavior( QAbstractItemView::SelectRows );
    setDragDropMode( QAbstractItemView::DropOnly );
    setAcceptDrops( true );
    setDropIndicatorShown( false );
    setAllColumnsShowFocus( true );
    setUniformRowHeights( false );
    setIndentation( 0 );
    setAnimated( false );

    setItemDelegate( new SourceDelegate( this ) );

    setContextMenuPolicy( Qt::CustomContextMenu );
    connect( this, SIGNAL( customContextMenuRequested( const QPoint& ) ), SLOT( onCustomContextMenu( const QPoint& ) ) );

    m_model = new SourcesModel( this );
    setModel( m_model );

    header()->setStretchLastSection( false );
    header()->setResizeMode( 0, QHeaderView::Stretch );

    connect( m_model, SIGNAL( clicked( QModelIndex ) ), SIGNAL( clicked( QModelIndex ) ) );
    connect( this,  SIGNAL( clicked( QModelIndex ) ), SLOT( onItemActivated( QModelIndex ) ) );

    connect( selectionModel(), SIGNAL( selectionChanged( const QItemSelection&, const QItemSelection& ) ), SLOT( onSelectionChanged() ) );
    connect( SourceList::instance(), SIGNAL( sourceRemoved( Tomahawk::source_ptr ) ), SLOT( onSourceOffline( Tomahawk::source_ptr ) ) );

    m_model->appendItem( source_ptr() );
}


void
SourceTreeView::setupMenus()
{
    m_playlistMenu.clear();

    m_loadPlaylistAction = m_playlistMenu.addAction( tr( "&Load Playlist" ) );
    m_renamePlaylistAction = m_playlistMenu.addAction( tr( "&Rename Playlist" ) );
    m_playlistMenu.addSeparator();
    m_deletePlaylistAction = m_playlistMenu.addAction( tr( "&Delete Playlist" ) );

    bool readonly = true;
    SourcesModel::SourceType type = SourcesModel::indexType( m_contextMenuIndex );
    if ( type == SourcesModel::PlaylistSource || type == SourcesModel::DynamicPlaylistSource )
    {
        playlist_ptr playlist = SourcesModel::indexToDynamicPlaylist( m_contextMenuIndex );
        if( playlist.isNull() ) 
        {
            playlist = SourcesModel::indexToPlaylist( m_contextMenuIndex );
        } 
        if ( !playlist.isNull() )
        {
            readonly = !playlist->author()->isLocal();
        }
    }

    if ( readonly )
    {
        m_deletePlaylistAction->setEnabled( !readonly );
        m_renamePlaylistAction->setEnabled( !readonly );
    }

    connect( m_loadPlaylistAction,   SIGNAL( triggered() ), SLOT( loadPlaylist() ) );
    connect( m_renamePlaylistAction, SIGNAL( triggered() ), SLOT( renamePlaylist() ) );
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

    SourcesModel::SourceType type = SourcesModel::indexType( index );
    if ( type == SourcesModel::CollectionSource )
    {
        SourceTreeItem* item = SourcesModel::indexToTreeItem( index );
        if ( item )
        {
            if ( item->source().isNull() )
            {
                PlaylistManager::instance()->showSuperCollection();
            }
            else
            {
                qDebug() << "SourceTreeItem toggled:" << item->source()->userName();

                PlaylistManager::instance()->show( item->source()->collection() );
            }
        }
    }
    else if ( type == SourcesModel::PlaylistSource )
    {
        playlist_ptr playlist = SourcesModel::indexToPlaylist( index );
        if ( !playlist.isNull() )
        {
            qDebug() << "Playlist activated:" << playlist->title();

            PlaylistManager::instance()->show( playlist );
        }
    }
    else if ( type == SourcesModel::DynamicPlaylistSource  )
    {
        dynplaylist_ptr playlist = SourcesModel::indexToDynamicPlaylist( index );
        if ( !playlist.isNull() )
        {
            qDebug() << "Dynamic Playlist activated:" << playlist->title();
            
            PlaylistManager::instance()->show( playlist );
        }
    }
}


void
SourceTreeView::onSelectionChanged()
{
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

    SourcesModel::SourceType type = SourcesModel::indexType( idx );
    if ( type == SourcesModel::PlaylistSource )
    {
        playlist_ptr playlist = SourcesModel::indexToPlaylist( idx );
        if ( !playlist.isNull() )
        {
            qDebug() << "Playlist about to be deleted:" << playlist->title();
            Playlist::remove( playlist );
        }
    } else if( type == SourcesModel::DynamicPlaylistSource ) {
        dynplaylist_ptr playlist = SourcesModel::indexToDynamicPlaylist( idx );       
        if( !playlist.isNull() )
            DynamicPlaylist::remove( playlist );
    }
}

void SourceTreeView::renamePlaylist()
{
    edit( m_contextMenuIndex );
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

            if ( SourcesModel::indexType( index ) == SourcesModel::PlaylistSource )
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
            if ( SourcesModel::indexType( index ) == SourcesModel::PlaylistSource )
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


void
SourceTreeView::drawRow( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    QTreeView::drawRow( painter, option, index );
}


void
SourceDelegate::paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    QStyleOptionViewItem o = option;
    o.rect.adjust( 12, 0, 0, 0 );

    if ( ( option.state & QStyle::State_Enabled ) == QStyle::State_Enabled )
    {
        o.state = QStyle::State_Enabled;

        if ( ( SourcesModel::indexType( index ) == SourcesModel::PlaylistSource || SourcesModel::indexType( index ) == SourcesModel::DynamicPlaylistSource ) &&
           ( option.state & QStyle::State_Selected ) == QStyle::State_Selected )
        {
            o.palette.setColor( QPalette::Text, o.palette.color( QPalette::HighlightedText ) );
        }
    }

    QStyledItemDelegate::paint( painter, option, index.model()->index( 0, 0 ) );
    QStyledItemDelegate::paint( painter, o, index );
}
