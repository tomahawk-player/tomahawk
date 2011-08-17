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

#include <QAction>
#include <QApplication>
#include <QContextMenuEvent>
#include <QDragEnterEvent>
#include <QHeaderView>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QSize>
#include <QFileDialog>

#include "playlist.h"
#include "viewmanager.h"
#include "sourcesproxymodel.h"
#include "sourcelist.h"
#include "sourcetree/items/playlistitems.h"
#include "sourcetree/items/collectionitem.h"
#include "audio/audioengine.h"
#include "sourceplaylistinterface.h"
#include "tomahawksettings.h"
#include "globalactionmanager.h"
#include "dropjob.h"

#include "utils/logger.h"
#include "items/genericpageitems.h"

using namespace Tomahawk;

#define TREEVIEW_INDENT_ADD -7


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

        editor->setGeometry( editor->geometry().adjusted( 2*TREEVIEW_INDENT_ADD, 0, 0, 0 ) );
    }
    virtual bool editorEvent( QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index );

private:
    QAbstractItemView* m_parent;
    mutable int m_iconHeight;
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
    setIndentation( 14 );
    setSortingEnabled( true );
    sortByColumn( 0, Qt::AscendingOrder );

#ifdef Q_OS_MAC
    setVerticalScrollMode( QTreeView::ScrollPerPixel );
#endif

    // TODO animation conflicts with the expanding-playlists-when-collection-is-null
    // so investigate
//     setAnimated( true );

    setItemDelegate( new SourceDelegate( this ) );

    setContextMenuPolicy( Qt::CustomContextMenu );
    connect( this, SIGNAL( customContextMenuRequested( QPoint ) ), SLOT( onCustomContextMenu( QPoint ) ) );

    m_model = new SourcesModel( this );
    m_proxyModel = new SourcesProxyModel( m_model, this );
    connect( m_proxyModel, SIGNAL( selectRequest( QModelIndex ) ), this, SLOT( selectRequest( QModelIndex ) ), Qt::QueuedConnection );
    connect( m_proxyModel, SIGNAL( expandRequest( QModelIndex ) ), this, SLOT( expandRequest( QModelIndex ) ), Qt::QueuedConnection );

    setModel( m_proxyModel );

    header()->setStretchLastSection( false );
    header()->setResizeMode( 0, QHeaderView::Stretch );

    connect( this, SIGNAL( clicked( QModelIndex ) ), SLOT( onItemActivated( QModelIndex ) ) );
    connect( this, SIGNAL( expanded( QModelIndex ) ), this, SLOT( onItemExpanded( QModelIndex ) ) );
//     connect( selectionModel(), SIGNAL( selectionChanged( QItemSelection, QItemSelection ) ), SLOT( onSelectionChanged() ) );

    showOfflineSources( TomahawkSettings::instance()->showOfflineSources() );

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
    m_roPlaylistMenu.clear();
    m_latchMenu.clear();

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

    m_latchOnAction = m_latchMenu.addAction( tr( "&Listen Along" ) );

    if ( type == SourcesModel::Collection )
    {
        CollectionItem* item = itemFromIndex< CollectionItem >( m_contextMenuIndex );
        source_ptr source = item->source();
        if ( !source.isNull() )
        {
            PlaylistInterface* pi = AudioEngine::instance()->playlist();
            if ( pi && dynamic_cast< SourcePlaylistInterface* >( pi ) )
            {
                SourcePlaylistInterface* sourcepi = dynamic_cast< SourcePlaylistInterface* >( pi );
                if ( !sourcepi->source().isNull() && sourcepi->source()->id() == source->id() )
                {
                    m_latchOnAction->setText( tr( "&Catch Up" ) );
                    m_latchMenu.addSeparator();
                    m_latchOffAction = m_latchMenu.addAction( tr( "&Stop Listening Along" ) );
                    connect( m_latchOffAction,       SIGNAL( triggered() ), SLOT( latchOff() ) );
                }
            }
        }
    }

    m_loadPlaylistAction = m_playlistMenu.addAction( tr( "&Load Playlist" ) );
    m_renamePlaylistAction = m_playlistMenu.addAction( tr( "&Rename Playlist" ) );
    m_playlistMenu.addSeparator();

    m_copyPlaylistAction = m_playlistMenu.addAction( tr( "&Copy Link" ) );
    m_deletePlaylistAction = m_playlistMenu.addAction( tr( "&Delete %1" ).arg( SourcesModel::rowTypeToString( type ) ) );

    QString addToText = QString( "Add to my %1" );
    if ( type == SourcesModel::StaticPlaylist )
        addToText = addToText.arg( "Playlists" );
    if ( type == SourcesModel::AutomaticPlaylist )
        addToText = addToText.arg( "Automatic Playlists" );
    else if ( type == SourcesModel::Station )
        addToText = addToText.arg( "Stations" );

    m_addToLocalAction = m_roPlaylistMenu.addAction( tr( addToText.toUtf8(), "Adds the given playlist, dynamic playlist, or station to the users's own list" ) );

    m_roPlaylistMenu.addAction( m_copyPlaylistAction );
    m_deletePlaylistAction->setEnabled( !readonly );
    m_renamePlaylistAction->setEnabled( !readonly );
    m_addToLocalAction->setEnabled( readonly );

    if ( type == SourcesModel::StaticPlaylist )
        m_copyPlaylistAction->setText( tr( "&Export Playlist" ) );

    connect( m_loadPlaylistAction,   SIGNAL( triggered() ), SLOT( loadPlaylist() ) );
    connect( m_renamePlaylistAction, SIGNAL( triggered() ), SLOT( renamePlaylist() ) );
    connect( m_deletePlaylistAction, SIGNAL( triggered() ), SLOT( deletePlaylist() ) );
    connect( m_copyPlaylistAction,   SIGNAL( triggered() ), SLOT( copyPlaylistLink() ) );
    connect( m_addToLocalAction,     SIGNAL( triggered() ), SLOT( addToLocal() ) );
    connect( m_latchOnAction,        SIGNAL( triggered() ), SLOT( latchOn() ) );
}


void
SourceTreeView::showOfflineSources( bool offlineSourcesShown )
{
    m_proxyModel->showOfflineSources( offlineSourcesShown );
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
    if ( !selectionModel()->selectedIndexes().contains( idx ) )
    {
        scrollTo( idx, QTreeView::EnsureVisible );
        selectionModel()->select( idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Current );
    }
}

void
SourceTreeView::expandRequest( const QModelIndex &idx )
{
    qDebug() << "Expanding idx" << idx;
    expand( idx );
}


void
SourceTreeView::loadPlaylist()
{
    onItemActivated( m_contextMenuIndex );
}


void
SourceTreeView::deletePlaylist( const QModelIndex& idxIn )
{
    qDebug() << Q_FUNC_INFO;

    QModelIndex idx = idxIn.isValid() ? idxIn : m_contextMenuIndex;
    if ( !idx.isValid() )
        return;

    SourcesModel::RowType type = ( SourcesModel::RowType )model()->data( idx, SourcesModel::SourceTreeItemTypeRole ).toInt();
    if ( type == SourcesModel::StaticPlaylist )
    {
        PlaylistItem* item = itemFromIndex< PlaylistItem >( idx );
        playlist_ptr playlist = item->playlist();
        Playlist::remove( playlist );
    } else if( type == SourcesModel::AutomaticPlaylist || type == SourcesModel::Station )
    {
        DynamicPlaylistItem* item = itemFromIndex< DynamicPlaylistItem >( idx );
        dynplaylist_ptr playlist = item->dynPlaylist();
        DynamicPlaylist::remove( playlist );
    }
}


void
SourceTreeView::copyPlaylistLink()
{
    QModelIndex idx = m_contextMenuIndex;
    if ( !idx.isValid() )
        return;

    SourcesModel::RowType type = ( SourcesModel::RowType )model()->data( m_contextMenuIndex, SourcesModel::SourceTreeItemTypeRole ).toInt();
    if( type == SourcesModel::AutomaticPlaylist || type == SourcesModel::Station )
    {
        DynamicPlaylistItem* item = itemFromIndex< DynamicPlaylistItem >( m_contextMenuIndex );
        dynplaylist_ptr playlist = item->dynPlaylist();
        GlobalActionManager::instance()->copyPlaylistToClipboard( playlist );
    } else if ( type == SourcesModel::StaticPlaylist )
    {
        PlaylistItem* item = itemFromIndex< PlaylistItem >( m_contextMenuIndex );
        playlist_ptr playlist = item->playlist();

        QString filename = QFileDialog::getSaveFileName( this, tr( "Save XSPF" ), QDir::homePath(), tr( "Playlists (*.xspf)" ) );
        GlobalActionManager::instance()->savePlaylistToFile( playlist, filename );
    }
}


void
SourceTreeView::addToLocal()
{
    QModelIndex idx = m_contextMenuIndex;
    if ( !idx.isValid() )
        return;

    SourcesModel::RowType type = ( SourcesModel::RowType )model()->data( m_contextMenuIndex, SourcesModel::SourceTreeItemTypeRole ).toInt();
    if( type == SourcesModel::AutomaticPlaylist || type == SourcesModel::Station )
    {
        DynamicPlaylistItem* item = itemFromIndex< DynamicPlaylistItem >( m_contextMenuIndex );
        dynplaylist_ptr playlist = item->dynPlaylist();

        // copy to a link and then generate a new playlist from that
        // this way we cheaply regenerate the needed controls
        QString link = GlobalActionManager::instance()->copyPlaylistToClipboard( playlist );
        GlobalActionManager::instance()->parseTomahawkLink( link );
    }
    else if ( type == SourcesModel::StaticPlaylist )
    {
        PlaylistItem* item = itemFromIndex< PlaylistItem >( m_contextMenuIndex );
        playlist_ptr playlist = item->playlist();

        // just create the new playlist with the same values
        QList< query_ptr > queries;
        foreach( const plentry_ptr& e, playlist->entries() )
            queries << e->query();

        playlist_ptr newpl = Playlist::create( SourceList::instance()->getLocal(), uuid(), playlist->title(), playlist->info(), playlist->creator(), playlist->shared(), queries );
    }
}


void
SourceTreeView::latchOn()
{
    qDebug() << Q_FUNC_INFO;
    QModelIndex idx = m_contextMenuIndex;
    if ( !idx.isValid() )
        return;

    SourcesModel::RowType type = ( SourcesModel::RowType )model()->data( m_contextMenuIndex, SourcesModel::SourceTreeItemTypeRole ).toInt();
    if( type != SourcesModel::Collection )
        return;

    CollectionItem* item = itemFromIndex< CollectionItem >( m_contextMenuIndex );
    source_ptr source = item->source();
    PlaylistInterface* pi = AudioEngine::instance()->playlist();

    if ( pi && dynamic_cast< SourcePlaylistInterface* >( pi ) )
    {
        SourcePlaylistInterface* sourcepi = dynamic_cast< SourcePlaylistInterface* >( pi );
        if ( !sourcepi->source().isNull() && sourcepi->source()->id() == source->id() )
        {
            //it's a catch-up -- if they're trying to catch-up in the same track, don't do anything
            //so that you don't repeat the track and/or cause the retry timer to fire
            if ( !AudioEngine::instance()->currentTrack().isNull() && !sourcepi->hasNextItem() &&
                  AudioEngine::instance()->currentTrack()->id() == sourcepi->currentItem()->id() )
                    return;
        }
    }

    AudioEngine::instance()->playItem( source->getPlaylistInterface().data(), source->getPlaylistInterface()->nextItem() );
}


void
SourceTreeView::latchOff()
{
    qDebug() << Q_FUNC_INFO;
    QModelIndex idx = m_contextMenuIndex;
    if ( !idx.isValid() )
        return;

    SourcesModel::RowType type = ( SourcesModel::RowType )model()->data( m_contextMenuIndex, SourcesModel::SourceTreeItemTypeRole ).toInt();
    if( type != SourcesModel::Collection )
        return;

    AudioEngine::instance()->stop();
    AudioEngine::instance()->setPlaylist( 0 );
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
        else
            m_roPlaylistMenu.exec( mapToGlobal( pos ) );
    }
    else if ( model()->data( m_contextMenuIndex, SourcesModel::SourceTreeItemTypeRole ) == SourcesModel::Collection )
    {
        CollectionItem* item = itemFromIndex< CollectionItem >( m_contextMenuIndex );
        if ( !item->source().isNull() && !item->source()->isLocal() )
            m_latchMenu.exec( mapToGlobal( pos ) );
    }
}


void
SourceTreeView::dragEnterEvent( QDragEnterEvent* event )
{
    qDebug() << Q_FUNC_INFO;
    QTreeView::dragEnterEvent( event );

    if ( DropJob::acceptsMimeData( event->mimeData() ) )
    {
        m_dragging = true;
        m_dropRect = QRect();
        m_dropIndex = QPersistentModelIndex();

        qDebug() << "Accepting Drag Event";
        event->setDropAction( Qt::CopyAction );
        event->accept();
    }
}


void
SourceTreeView::dragLeaveEvent( QDragLeaveEvent* event )
{
    QTreeView::dragLeaveEvent( event );

    m_dragging = false;
    setDirtyRegion( m_dropRect );

    m_dropIndex = QPersistentModelIndex();
}


void
SourceTreeView::dragMoveEvent( QDragMoveEvent* event )
{
    bool accept = false;
    QTreeView::dragMoveEvent( event );

    if ( DropJob::acceptsMimeData( event->mimeData() ) )
    {
        setDirtyRegion( m_dropRect );
        const QPoint pos = event->pos();
        const QModelIndex index = indexAt( pos );
        m_dropIndex = QPersistentModelIndex( index );

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
    m_dropIndex = QPersistentModelIndex();
}


void
SourceTreeView::keyPressEvent( QKeyEvent *event )
{
    if( ( event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace ) && !selectionModel()->selectedIndexes().isEmpty() )
    {
        QModelIndex idx = selectionModel()->selectedIndexes().first();
        if ( model()->data( idx, SourcesModel::SourceTreeItemTypeRole ) == SourcesModel::StaticPlaylist ||
             model()->data( idx, SourcesModel::SourceTreeItemTypeRole ) == SourcesModel::AutomaticPlaylist ||
             model()->data( idx, SourcesModel::SourceTreeItemTypeRole ) == SourcesModel::Station )
        {
            PlaylistItem* item = itemFromIndex< PlaylistItem >( idx );
            Q_ASSERT( item );

            if( item->playlist()->author()->isLocal() ) {
                deletePlaylist( idx );
            }
        }
    }
}


void
SourceTreeView::paintEvent( QPaintEvent* event )
{
    if ( m_dragging && !m_dropRect.isEmpty() )
    {
        QPainter painter( viewport() );
        const QRect itemRect = visualRect( m_dropIndex );

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

        QString tracks;
        QString name = index.data().toString();
        int figWidth = 0;

        if ( status && colItem && !colItem->source().isNull() )
        {
            tracks = QString::number( colItem->source()->trackCount() );
            figWidth = painter->fontMetrics().width( tracks );
            name = colItem->source()->friendlyName();
        }

        QRect iconRect = option.rect.adjusted( 4, 6, -option.rect.width() + option.rect.height() - 12 + 4, -6 );

        QPixmap avatar = colItem->icon().pixmap( iconRect.size() );
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

        textRect = option.rect.adjusted( iconRect.width() + 8, painter->fontMetrics().height() + 6, -figWidth - 24, -4 );
        painter->setFont( normal );
        text = painter->fontMetrics().elidedText( desc, Qt::ElideRight, textRect.width() );
        QTextOption to( Qt::AlignBottom );
        painter->drawText( textRect, text, to );

        if ( status )
        {
            painter->setRenderHint( QPainter::Antialiasing );

            QRect figRect = o.rect.adjusted( o.rect.width() - figWidth - 8, 0, -13, -o.rect.height() + 16 );
            int hd = ( option.rect.height() - figRect.height() ) / 2;
            figRect.adjust( 0, hd, 0, hd );
#ifdef Q_OS_WIN
            figRect.adjust( -3, 0, 3, 0 );
#endif
            painter->setFont( bold );

            QColor figColor( 167, 183, 211 );
            painter->setPen( figColor );
            painter->setBrush( figColor );

            TomahawkUtils::drawBackgroundAndNumbers( painter, tracks, figRect );
        }

        painter->restore();
    }
    else
    {
        QStyledItemDelegate::paint( painter, o, index );

        if ( type == SourcesModel::GenericPage )
        {
            GenericPageItem* gpi = qobject_cast< GenericPageItem* >( item );
            Q_ASSERT( gpi );

            if ( gpi->isTempItem() && o3.state & QStyle::State_MouseOver )
            {
                // draw close icon
                int padding = 3;
                m_iconHeight = ( o3.rect.height() - 2*padding );
                QPixmap p( RESPATH "images/list-remove.png" );
                p = p.scaledToHeight( m_iconHeight, Qt::SmoothTransformation );

                QRect r ( o3.rect.right() - padding - m_iconHeight, padding + o3.rect.y(), m_iconHeight, m_iconHeight );
                painter->drawPixmap( r, p );
            }
        }
        /*QStyleOptionViewItemV4 opt = o;

        // shrink the indentations. count how indented this item is and remove it
        int indentMult = 0;
        QModelIndex counter = index;
        while ( counter.parent().isValid() )
        {
            indentMult++;
            counter = counter.parent();
        }
        int realX = opt.rect.x() + indentMult * TREEVIEW_INDENT_ADD;

        opt.rect.setX( realX );
        const QWidget *widget = opt.widget;
        QStyle *style = widget ? widget->style() : QApplication::style();
        style->drawControl( QStyle::CE_ItemViewItem, &opt, painter, widget ); */
    }

#ifdef Q_WS_MAC
    painter->setFont( savedFont );
#endif
}

bool
SourceDelegate::editorEvent ( QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index )
{

    if ( event->type() == QEvent::MouseButtonRelease )
    {
        SourcesModel::RowType type = static_cast< SourcesModel::RowType >( index.data( SourcesModel::SourceTreeItemTypeRole ).toInt() );
        if ( type == SourcesModel::GenericPage )
        {
            GenericPageItem* gpi = qobject_cast< GenericPageItem* >( index.data( SourcesModel::SourceTreeItemRole ).value< SourceTreeItem* >() );
            Q_ASSERT( gpi );
            if ( gpi->isTempItem() )
            {
                QMouseEvent* ev = static_cast< QMouseEvent* >( event );

                QStyleOptionViewItemV4 o = option;
                initStyleOption( &o, index );
                int padding = 3;
                QRect r ( o.rect.right() - padding - m_iconHeight, padding + o.rect.y(), m_iconHeight, m_iconHeight );

                if ( r.contains( ev->pos() ) )
                {
                    gpi->deleteTempPage();
                }
            }
        }
    }

    return QStyledItemDelegate::editorEvent ( event, model, option, index );
}
