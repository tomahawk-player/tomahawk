/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "sourcetreeview.h"

#include "playlist.h"
#include "viewmanager.h"
#include "sourcesproxymodel.h"
#include "sourcelist.h"
#include "sourcetree/items/playlistitems.h"
#include "sourcetree/items/collectionitem.h"

#include <QAction>
#include <QApplication>
#include <QContextMenuEvent>
#include <QDragEnterEvent>
#include <QHeaderView>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QSize>

using namespace Tomahawk;


class SourceDelegate : public QStyledItemDelegate
{
public:
    SourceDelegate( QAbstractItemView* parent = 0 ) : QStyledItemDelegate( parent ), m_parent( parent ) {}

protected:
    virtual QSize sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const;
    virtual void paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const;
    virtual void updateEditorGeometry( QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index ) const
    {
        if ( index.data( SourcesModel::SourceTreeItemTypeRole ).toInt() == SourcesModel::StaticPlaylist )
            editor->setGeometry( option.rect.adjusted( 20, 0, 0, 0 ) );
        else
            QStyledItemDelegate::updateEditorGeometry( editor, option, index );
    }

private:
    QAbstractItemView* m_parent;
};


SourceTreeView::SourceTreeView( QWidget* parent )
    : QTreeView( parent )
    , m_dragging( false )
{
    setFrameShape( QFrame::NoFrame );
    setAttribute( Qt::WA_MacShowFocusRect, 0 );
    setContentsMargins( 0, 0, 0, 0 );
    setMinimumWidth( 220 );

    setHeaderHidden( true );
    setRootIsDecorated( true );
    setExpandsOnDoubleClick( false );

    setSelectionBehavior( QAbstractItemView::SelectRows );
    setDragDropMode( QAbstractItemView::DropOnly );
    setAcceptDrops( true );
    setDropIndicatorShown( false );
    setAllColumnsShowFocus( true );
    setUniformRowHeights( false );
    setIndentation( 16 );
    setSortingEnabled( true );
    sortByColumn( 1, Qt::AscendingOrder );

    // TODO animation conflicts with the expanding-playlists-when-collection-is-null
    // so investigate
//     setAnimated( true );

    setItemDelegate( new SourceDelegate( this ) );

    setContextMenuPolicy( Qt::CustomContextMenu );
    connect( this, SIGNAL( customContextMenuRequested( QPoint ) ), SLOT( onCustomContextMenu( QPoint ) ) );

    m_model = new SourcesModel( this );
    m_proxyModel = new SourcesProxyModel( m_model, this );
    connect( m_proxyModel, SIGNAL( selectRequest( QModelIndex ) ), this, SLOT( selectRequest( QModelIndex ) ), Qt::QueuedConnection );

    setModel( m_proxyModel );

    header()->setStretchLastSection( false );
    header()->setResizeMode( 0, QHeaderView::Stretch );

    connect( m_model, SIGNAL( clicked( QModelIndex ) ), SIGNAL( clicked( QModelIndex ) ) );
    connect( this, SIGNAL( clicked( QModelIndex ) ), SLOT( onItemActivated( QModelIndex ) ) );
    connect( this, SIGNAL( expanded( QModelIndex ) ), this, SLOT( onItemExpanded( QModelIndex ) ) );
//     connect( selectionModel(), SIGNAL( selectionChanged( QItemSelection, QItemSelection ) ), SLOT( onSelectionChanged() ) );

    hideOfflineSources();

    // Light-blue sourcetree on osx
#ifdef Q_WS_MAC
    setStyleSheet( "SourceTreeView:active { background: #DDE4EB; } "
                   "SourceTreeView        { background: #EDEDED; } " );
#endif

}


void
SourceTreeView::setupMenus()
{
    m_playlistMenu.clear();

    bool readonly = true;
    SourcesModel::RowType type = ( SourcesModel::RowType )model()->data( m_contextMenuIndex, SourcesModel::SourceTreeItemTypeRole ).toInt();
    if ( type == SourcesModel::StaticPlaylist || type == SourcesModel::AutomaticPlaylist || type == SourcesModel::Station )
    {

        PlaylistItem* item = itemFromIndex< PlaylistItem >( m_contextMenuIndex );
        playlist_ptr playlist = item->playlist();
        if ( !playlist.isNull() )
        {
            readonly = !playlist->author()->isLocal();
        }
    }

    m_loadPlaylistAction = m_playlistMenu.addAction( tr( "&Load Playlist" ) );
    m_renamePlaylistAction = m_playlistMenu.addAction( tr( "&Rename Playlist" ) );
    m_playlistMenu.addSeparator();
    m_deletePlaylistAction = m_playlistMenu.addAction( tr( "&Delete %1" ).arg( SourcesModel::rowTypeToString( type ) ) );

    m_deletePlaylistAction->setEnabled( !readonly );
    m_renamePlaylistAction->setEnabled( !readonly );

    connect( m_loadPlaylistAction,   SIGNAL( triggered() ), SLOT( loadPlaylist() ) );
    connect( m_renamePlaylistAction, SIGNAL( triggered() ), SLOT( renamePlaylist() ) );
    connect( m_deletePlaylistAction, SIGNAL( triggered() ), SLOT( deletePlaylist() ) );
}


void
SourceTreeView::showOfflineSources()
{
    m_proxyModel->showOfflineSources();
}


void
SourceTreeView::hideOfflineSources()
{
    m_proxyModel->hideOfflineSources();
}


void
SourceTreeView::onItemActivated( const QModelIndex& index )
{
    if ( !index.isValid() )
        return;

    SourceTreeItem* item = itemFromIndex< SourceTreeItem >( index );
    item->activate();
}


void
SourceTreeView::onItemExpanded( const QModelIndex& idx )
{
    // make sure to expand children nodes for collections
    if( idx.data( SourcesModel::SourceTreeItemTypeRole ) == SourcesModel::Collection ) {
       for( int i = 0; i < model()->rowCount( idx ); i++ ) {
           setExpanded( model()->index( i, 0, idx ), true );
       }
    }
}


void
SourceTreeView::selectRequest( const QModelIndex& idx )
{
    if( !selectionModel()->selectedIndexes().contains( idx ) )
        selectionModel()->select( idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Current );

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

    SourcesModel::RowType type = ( SourcesModel::RowType )model()->data( m_contextMenuIndex, SourcesModel::SourceTreeItemTypeRole ).toInt();
    if ( type == SourcesModel::StaticPlaylist )
    {
        PlaylistItem* item = itemFromIndex< PlaylistItem >( m_contextMenuIndex );
        playlist_ptr playlist = item->playlist();
        Playlist::remove( playlist );
    } else if( type == SourcesModel::AutomaticPlaylist || type == SourcesModel::Station )
    {
        DynamicPlaylistItem* item = itemFromIndex< DynamicPlaylistItem >( m_contextMenuIndex );
        dynplaylist_ptr playlist = item->dynPlaylist();
        DynamicPlaylist::remove( playlist );
    }
}


void
SourceTreeView::renamePlaylist()
{
    if( !m_contextMenuIndex.isValid() && !selectionModel()->selectedIndexes().isEmpty() )
        edit( selectionModel()->selectedIndexes().first() );
    else
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

    if ( model()->data( m_contextMenuIndex, SourcesModel::SourceTreeItemTypeRole ) == SourcesModel::StaticPlaylist ||
         model()->data( m_contextMenuIndex, SourcesModel::SourceTreeItemTypeRole ) == SourcesModel::AutomaticPlaylist ||
         model()->data( m_contextMenuIndex, SourcesModel::SourceTreeItemTypeRole ) == SourcesModel::Station )
    {
        PlaylistItem* item = itemFromIndex< PlaylistItem >( m_contextMenuIndex );
        if( item->playlist()->author()->isLocal() )
            m_playlistMenu.exec( mapToGlobal( pos ) );
    }
}


void
SourceTreeView::dragEnterEvent( QDragEnterEvent* event )
{
    qDebug() << Q_FUNC_INFO;
    QTreeView::dragEnterEvent( event );

    if ( event->mimeData()->hasFormat( "application/tomahawk.query.list" )
      || event->mimeData()->hasFormat( "application/tomahawk.result.list" ) )
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

    if ( event->mimeData()->hasFormat( "application/tomahawk.query.list" )
      || event->mimeData()->hasFormat( "application/tomahawk.result.list" ) )
    {
        setDirtyRegion( m_dropRect );
        const QPoint pos = event->pos();
        const QModelIndex index = indexAt( pos );

        if ( index.isValid() )
        {
            const QRect rect = visualRect( index );
            m_dropRect = rect;

            const SourceTreeItem* item = itemFromIndex< SourceTreeItem >( index );
            if( item->willAcceptDrag( event->mimeData() ) )
                accept = true;
        }
        else
        {
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


template< typename T > T*
SourceTreeView::itemFromIndex( const QModelIndex& index ) const
{
    Q_ASSERT( model()->data( index, SourcesModel::SourceTreeItemRole ).value< SourceTreeItem* >() );

    T* item = qobject_cast< T* >( model()->data( index, SourcesModel::SourceTreeItemRole ).value< SourceTreeItem* >() );
    Q_ASSERT( item );

    return item;
}


QSize
SourceDelegate::sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    if ( index.data( SourcesModel::SourceTreeItemTypeRole ).toInt() == SourcesModel::Collection )
        return QSize( option.rect.width(), 44 );
    else
        return QStyledItemDelegate::sizeHint( option, index );
}


void
SourceDelegate::paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    QStyleOptionViewItem o = option;

#ifdef Q_WS_MAC
    QFont savedFont = painter->font();
    QFont smaller = savedFont;
    smaller.setPointSize( smaller.pointSize() - 2 );
    painter->setFont( smaller );
    o.font = smaller;
#endif

    if ( ( option.state & QStyle::State_Enabled ) == QStyle::State_Enabled )
    {
        o.state = QStyle::State_Enabled;

        if ( ( option.state & QStyle::State_Selected ) == QStyle::State_Selected )
        {
            o.palette.setColor( QPalette::Text, o.palette.color( QPalette::HighlightedText ) );
        }
    }

    SourcesModel::RowType type = static_cast< SourcesModel::RowType >( index.data( SourcesModel::SourceTreeItemTypeRole ).toInt() );
    SourceTreeItem* item = index.data( SourcesModel::SourceTreeItemRole ).value< SourceTreeItem* >();
    Q_ASSERT( item );

    QStyleOptionViewItemV4 o3 = option;
    if ( type != SourcesModel::Collection && type != SourcesModel::Category )
        o3.rect.setX( 0 );

    QApplication::style()->drawControl( QStyle::CE_ItemViewItem, &o3, painter );

    if ( type == SourcesModel::Collection )
    {
        painter->save();

        QFont normal = painter->font();
        QFont bold = painter->font();
        bold.setBold( true );

        CollectionItem* colItem = qobject_cast< CollectionItem* >( item );
        Q_ASSERT( colItem );
        bool status = !( !colItem || colItem->source().isNull() || !colItem->source()->isOnline() );
        QPixmap avatar( RESPATH "images/user-avatar.png" );

        QString tracks;
        QString name = index.data().toString();
        int figWidth = 0;

        if ( status && colItem && !colItem->source().isNull() )
        {
            tracks = QString::number( colItem->source()->trackCount() );
            figWidth = painter->fontMetrics().width( tracks );
            if ( !colItem->source()->avatar().isNull() )
                avatar = colItem->source()->avatar();
            name = colItem->source()->friendlyName();
        }

        QRect iconRect = option.rect.adjusted( 4, 6, -option.rect.width() + option.rect.height() - 12 + 4, -6 );

        painter->drawPixmap( iconRect, avatar.scaledToHeight( iconRect.height(), Qt::SmoothTransformation ) );

        if ( ( option.state & QStyle::State_Selected ) == QStyle::State_Selected )
        {
            painter->setPen( o.palette.color( QPalette::HighlightedText ) );
        }

        QRect textRect = option.rect.adjusted( iconRect.width() + 8, 6, -figWidth - 24, 0 );
        if ( status || colItem->source().isNull() )
            painter->setFont( bold );
        QString text = painter->fontMetrics().elidedText( name, Qt::ElideRight, textRect.width() );
        painter->drawText( textRect, text );

        QString desc = status ? colItem->source()->textStatus() : tr( "Offline" );
        if ( colItem->source().isNull() )
            desc = tr( "All available tracks" );
        if ( status && desc.isEmpty() && !colItem->source()->currentTrack().isNull() )
            desc = colItem->source()->currentTrack()->artist() + " - " + colItem->source()->currentTrack()->track();
        if ( desc.isEmpty() )
            desc = tr( "Online" );

        textRect = option.rect.adjusted( iconRect.width() + 8, painter->fontMetrics().height() + 10, -figWidth - 24, 0 );
        painter->setFont( normal );
        text = painter->fontMetrics().elidedText( desc, Qt::ElideRight, textRect.width() );
        painter->drawText( textRect, text );

        if ( status )
        {
            painter->setRenderHint( QPainter::Antialiasing );

            QRect figRect = o.rect.adjusted( o.rect.width() - figWidth - 18, 0, -10, -o.rect.height() + 16 );
            int hd = ( option.rect.height() - figRect.height() ) / 2;
            figRect.adjust( 0, hd, 0, hd );

            QColor figColor( 167, 183, 211 );
            painter->setPen( figColor );
            painter->setBrush( figColor );

            QPen origpen = painter->pen();
            QPen pen = origpen;
            pen.setWidth( 1.0 );
            painter->setPen( pen );
            painter->drawRect( figRect );

            QPainterPath ppath;
            ppath.moveTo( QPoint( figRect.x(), figRect.y() ) );
            ppath.quadTo( QPoint( figRect.x() - 8, figRect.y() + figRect.height() / 2 ), QPoint( figRect.x(), figRect.y() + figRect.height() ) );
            painter->drawPath( ppath );
            ppath.moveTo( QPoint( figRect.x() + figRect.width(), figRect.y() ) );
            ppath.quadTo( QPoint( figRect.x() + figRect.width() + 8, figRect.y() + figRect.height() / 2 ), QPoint( figRect.x() + figRect.width(), figRect.y() + figRect.height() ) );
            painter->drawPath( ppath );

            painter->setPen( origpen );

            QTextOption to( Qt::AlignCenter );
            painter->setFont( bold );
            painter->setPen( Qt::white );
            painter->drawText( figRect, tracks, to );
        }

        painter->restore();
    }
    else
    {
        QStyledItemDelegate::paint( painter, o, index );
    }

#ifdef Q_WS_MAC
    painter->setFont( savedFont );
#endif
}
