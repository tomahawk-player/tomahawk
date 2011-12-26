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

#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QContextMenuEvent>
#include <QtGui/QDragEnterEvent>
#include <QtGui/QHeaderView>
#include <QtGui/QPainter>
#include <QtGui/QStyledItemDelegate>
#include <QtCore/QSize>
#include <QtGui/QFileDialog>

#include "actioncollection.h"
#include "playlist.h"
#include "viewmanager.h"
#include "sourcesproxymodel.h"
#include "sourcelist.h"
#include "sourcedelegate.h"
#include "sourcetree/items/playlistitems.h"
#include "sourcetree/items/sourceitem.h"
#include "audio/audioengine.h"
#include "sourceplaylistinterface.h"
#include "tomahawksettings.h"
#include "globalactionmanager.h"
#include "dropjob.h"
#include "utils/logger.h"
#include "items/genericpageitems.h"
#include "items/temporarypageitem.h"
#include "database/databasecommand_socialaction.h"
#include "database/database.h"
#include "LatchManager.h"

using namespace Tomahawk;


SourceTreeView::SourceTreeView( QWidget* parent )
    : QTreeView( parent )
    , m_latchManager( new LatchManager( this ) )
    , m_dragging( false )
{
    setProperty( "flattenBranches", QVariant( true ) );

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
    setIndentation( 0 );
    setSortingEnabled( true );
    sortByColumn( 0, Qt::AscendingOrder );
    setVerticalScrollMode( QTreeView::ScrollPerPixel );
    setMouseTracking( true );

    // TODO animation conflicts with the expanding-playlists-when-collection-is-null
    // so investigate
//     setAnimated( true );

    m_delegate = new SourceDelegate( this );
    connect( m_delegate, SIGNAL( latchOn( Tomahawk::source_ptr ) ), SLOT( latchOnOrCatchUp( Tomahawk::source_ptr ) ), Qt::QueuedConnection );
    connect( m_delegate, SIGNAL( latchOff( Tomahawk::source_ptr ) ), SLOT( latchOff( Tomahawk::source_ptr ) ), Qt::QueuedConnection );

    setItemDelegate( m_delegate );

    setContextMenuPolicy( Qt::CustomContextMenu );
    connect( this, SIGNAL( customContextMenuRequested( QPoint ) ), SLOT( onCustomContextMenu( QPoint ) ) );

    m_model = new SourcesModel( this );
    m_proxyModel = new SourcesProxyModel( m_model, this );
    connect( m_proxyModel, SIGNAL( selectRequest( QPersistentModelIndex ) ), SLOT( selectRequest( QPersistentModelIndex ) ) );
    connect( m_proxyModel, SIGNAL( expandRequest( QPersistentModelIndex ) ), SLOT( expandRequest( QPersistentModelIndex ) ) );
    connect( m_proxyModel, SIGNAL( toggleExpandRequest( QPersistentModelIndex ) ), SLOT( toggleExpandRequest( QPersistentModelIndex ) ) );

    setModel( m_proxyModel );

    header()->setStretchLastSection( false );
    header()->setResizeMode( 0, QHeaderView::Stretch );

    connect( this, SIGNAL( clicked( QModelIndex ) ), SLOT( onItemActivated( QModelIndex ) ) );
    connect( this, SIGNAL( expanded( QModelIndex ) ), SLOT( onItemExpanded( QModelIndex ) ) );
//     connect( selectionModel(), SIGNAL( selectionChanged( QItemSelection, QItemSelection ) ), SLOT( onSelectionChanged() ) );

    showOfflineSources( TomahawkSettings::instance()->showOfflineSources() );

    // Light-blue sourcetree on osx
#ifdef Q_WS_MAC
    setStyleSheet( "SourceTreeView:active { background: #DDE4EB; } "
                   "SourceTreeView        { background: #EDEDED; } " );
#endif

    connect( this, SIGNAL( latchRequest( Tomahawk::source_ptr ) ), m_latchManager, SLOT( latchRequest( Tomahawk::source_ptr ) ) );
    connect( this, SIGNAL( unlatchRequest( Tomahawk::source_ptr ) ), m_latchManager, SLOT( unlatchRequest( Tomahawk::source_ptr ) ) );
    connect( this, SIGNAL( catchUpRequest() ), m_latchManager, SLOT( catchUpRequest() ) );

    connect( ActionCollection::instance(), SIGNAL( privacyModeChanged() ), SLOT( repaint() ) );
}


SourceTreeView::~SourceTreeView()
{
}


void
SourceTreeView::setupMenus()
{
    m_playlistMenu.clear();
    m_roPlaylistMenu.clear();
    m_latchMenu.clear();
    m_privacyMenu.clear();

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

    QAction* latchOnAction = ActionCollection::instance()->getAction( "latchOn" );
    m_latchMenu.addAction( latchOnAction );

    m_privacyMenu.addAction( ActionCollection::instance()->getAction( "togglePrivacy" ) );

    if ( type == SourcesModel::Collection )
    {
        SourceItem* item = itemFromIndex< SourceItem >( m_contextMenuIndex );
        source_ptr source = item->source();
        if ( !source.isNull() )
        {
            if ( m_latchManager->isLatched( source ) )
            {
                m_latchMenu.addSeparator();
                QAction *latchOffAction = ActionCollection::instance()->getAction( "latchOff" );
                m_latchMenu.addAction( latchOffAction );
                connect( latchOffAction, SIGNAL( triggered() ), SLOT( latchOff() ), Qt::QueuedConnection );
            }
        }
    }

    QAction *loadPlaylistAction = ActionCollection::instance()->getAction( "loadPlaylist" );
    m_playlistMenu.addAction( loadPlaylistAction );
    QAction *renamePlaylistAction = ActionCollection::instance()->getAction( "renamePlaylist" );
    m_playlistMenu.addAction( renamePlaylistAction );
    m_playlistMenu.addSeparator();

    QAction *copyPlaylistAction = m_playlistMenu.addAction( tr( "&Copy Link" ) );
    QAction *deletePlaylistAction = m_playlistMenu.addAction( tr( "&Delete %1" ).arg( SourcesModel::rowTypeToString( type ) ) );

    QString addToText = QString( "Add to my %1" );
    if ( type == SourcesModel::StaticPlaylist )
        addToText = addToText.arg( "Playlists" );
    if ( type == SourcesModel::AutomaticPlaylist )
        addToText = addToText.arg( "Automatic Playlists" );
    else if ( type == SourcesModel::Station )
        addToText = addToText.arg( "Stations" );

    QAction *addToLocalAction = m_roPlaylistMenu.addAction( tr( addToText.toUtf8(), "Adds the given playlist, dynamic playlist, or station to the users's own list" ) );

    m_roPlaylistMenu.addAction( copyPlaylistAction );
    deletePlaylistAction->setEnabled( !readonly );
    renamePlaylistAction->setEnabled( !readonly );
    addToLocalAction->setEnabled( readonly );

    if ( type == SourcesModel::StaticPlaylist )
        copyPlaylistAction->setText( tr( "&Export Playlist" ) );

    connect( loadPlaylistAction,   SIGNAL( triggered() ), SLOT( loadPlaylist() ) );
    connect( renamePlaylistAction, SIGNAL( triggered() ), SLOT( renamePlaylist() ) );
    connect( deletePlaylistAction, SIGNAL( triggered() ), SLOT( deletePlaylist() ) );
    connect( copyPlaylistAction,   SIGNAL( triggered() ), SLOT( copyPlaylistLink() ) );
    connect( addToLocalAction,     SIGNAL( triggered() ), SLOT( addToLocal() ) );
    connect( latchOnAction,        SIGNAL( triggered() ), SLOT( latchOnOrCatchUp() ), Qt::QueuedConnection );
}


void
SourceTreeView::showOfflineSources( bool offlineSourcesShown )
{
    m_proxyModel->showOfflineSources( offlineSourcesShown );
}


void
SourceTreeView::onItemActivated( const QModelIndex& index )
{
    if ( !index.isValid() || !index.flags().testFlag( Qt::ItemIsEnabled ) )
        return;

    SourceTreeItem* item = itemFromIndex< SourceTreeItem >( index );
    item->activate();
}


void
SourceTreeView::onItemExpanded( const QModelIndex& idx )
{
    // make sure to expand children nodes for collections
    if( idx.data( SourcesModel::SourceTreeItemTypeRole ) == SourcesModel::Collection )
    {
       for( int i = 0; i < model()->rowCount( idx ); i++ )
       {
           setExpanded( model()->index( i, 0, idx ), true );
       }
    }
}


void
SourceTreeView::selectRequest( const QPersistentModelIndex& idx )
{
    if ( !selectionModel()->selectedIndexes().contains( idx ) )
    {
        scrollTo( idx, QTreeView::EnsureVisible );
        selectionModel()->select( idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Current );
    }
}


void
SourceTreeView::expandRequest( const QPersistentModelIndex &idx )
{
    expand( idx );
}


void
SourceTreeView::toggleExpandRequest( const QPersistentModelIndex &idx )
{
    if ( isExpanded( idx ) )
        collapse( idx );
    else
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
    }
    else if( type == SourcesModel::AutomaticPlaylist || type == SourcesModel::Station )
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
    }
    else if ( type == SourcesModel::StaticPlaylist )
    {
        PlaylistItem* item = itemFromIndex< PlaylistItem >( m_contextMenuIndex );
        playlist_ptr playlist = item->playlist();

        QString filename = QFileDialog::getSaveFileName( TomahawkUtils::tomahawkWindow(), tr( "Save XSPF" ),
                                                         TomahawkSettings::instance()->playlistDefaultPath(), tr( "Playlists (*.xspf)" ) );
        if ( !filename.isEmpty() )
        {
            QFileInfo playlistAbsoluteFilePath = filename;
            TomahawkSettings::instance()->setPlaylistDefaultPath( playlistAbsoluteFilePath.absolutePath() );
            GlobalActionManager::instance()->savePlaylistToFile( playlist, filename );
        }
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
SourceTreeView::latchOnOrCatchUp()
{
    disconnect( this, SLOT( latchOnOrCatchUp() ) );
    if ( !m_contextMenuIndex.isValid() )
        return;

    SourcesModel::RowType type = ( SourcesModel::RowType )model()->data( m_contextMenuIndex, SourcesModel::SourceTreeItemTypeRole ).toInt();
    if( type != SourcesModel::Collection )
        return;

    SourceItem* item = itemFromIndex< SourceItem >( m_contextMenuIndex );
    source_ptr source = item->source();

    latchOnOrCatchUp( source );
}


void
SourceTreeView::latchOff()
{
    disconnect( this, SLOT( latchOff() ) );
    qDebug() << Q_FUNC_INFO;
    if ( !m_contextMenuIndex.isValid() )
        return;

    SourcesModel::RowType type = ( SourcesModel::RowType )model()->data( m_contextMenuIndex, SourcesModel::SourceTreeItemTypeRole ).toInt();
    if( type != SourcesModel::Collection )
        return;

    const SourceItem* item = itemFromIndex< SourceItem >( m_contextMenuIndex );
    const source_ptr source = item->source();

    latchOff( source );
}


void
SourceTreeView::latchOnOrCatchUp( const Tomahawk::source_ptr& source )
{
    if ( m_latchManager->isLatched( source ) )
        emit catchUpRequest();
    else
        emit latchRequest( source );
}


void
SourceTreeView::latchOff( const Tomahawk::source_ptr& source )
{
    emit unlatchRequest( source );
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
        SourceItem* item = itemFromIndex< SourceItem >( m_contextMenuIndex );
        if ( !item->source().isNull() && !item->source()->isLocal() )
            m_latchMenu.exec( mapToGlobal( pos ) );
        else if ( !item->source().isNull() )
            m_privacyMenu.exec( mapToGlobal( pos ) );
    }
}


void
SourceTreeView::dragEnterEvent( QDragEnterEvent* event )
{
    qDebug() << Q_FUNC_INFO;
    QTreeView::dragEnterEvent( event );

    if ( DropJob::acceptsMimeData( event->mimeData(), DropJob::Track | DropJob::Artist | DropJob::Album | DropJob::Playlist,  DropJob::Create ) )
    {
           m_dragging = true;
           m_dropRect = QRect();
           m_dropIndex = QPersistentModelIndex();

           qDebug() << Q_FUNC_INFO << "Accepting Drag Event";
           event->setDropAction( Qt::CopyAction );
           event->acceptProposedAction();
     }
}


void
SourceTreeView::dragLeaveEvent( QDragLeaveEvent* event )
{
    QTreeView::dragLeaveEvent( event );

    m_dragging = false;
    setDirtyRegion( m_dropRect );

    m_delegate->dragLeaveEvent();
    dataChanged( m_dropIndex, m_dropIndex );
    m_dropIndex = QPersistentModelIndex();
}


void
SourceTreeView::dragMoveEvent( QDragMoveEvent* event )
{
    bool accept = false;

    // Don't highlight the drop for a playlist, as it won't get added to the playlist but created generally
    if ( DropJob::isDropType( DropJob::Playlist, event->mimeData()  ) )
    {
        event->accept();
        return;
    }

    QTreeView::dragMoveEvent( event );

    if ( DropJob::acceptsMimeData( event->mimeData(),  DropJob::Track, DropJob::Append ) )
    {
        setDirtyRegion( m_dropRect );
        const QPoint pos = event->pos();
        const QModelIndex index = indexAt( pos );
        dataChanged(m_dropIndex, m_dropIndex);
        m_dropIndex = QPersistentModelIndex( index );

        if ( index.isValid() )
        {
            const QRect rect = visualRect( index );
            m_dropRect = rect;

            SourceTreeItem* item = itemFromIndex< SourceTreeItem >( index );
            if( item->willAcceptDrag( event->mimeData() ) )
            {

                accept = true;
                m_delegate->hovered( index, event->mimeData() );
                dataChanged(index, index);
            }
            else
                m_delegate->hovered( QModelIndex(), 0 );
        }
        else
        {
            m_dropRect = QRect();
        }

        if ( accept || DropJob::isDropType( DropJob::Playlist, event->mimeData() ) )
        {
            // Playlists are accepted always since they can be dropped anywhere
            //tDebug() << Q_FUNC_INFO << "Accepting";
            event->setDropAction( Qt::CopyAction );
            event->accept();
        }
        else
        {
//             tDebug() << Q_FUNC_INFO << "Ignoring";
            event->ignore();
        }
    }
    else if ( DropJob::acceptsMimeData( event->mimeData(),  DropJob::Playlist | DropJob::Artist | DropJob::Album, DropJob::Create ) )
    {
        event->setDropAction( Qt::CopyAction );
        event->accept();
    }
    setDirtyRegion( m_dropRect );
}


void
SourceTreeView::dropEvent( QDropEvent* event )
{
    const QPoint pos = event->pos();
    const QModelIndex index = indexAt( pos );

    if ( model()->data( index, SourcesModel::SourceTreeItemTypeRole ).toInt() == SourcesModel::StaticPlaylist
         || model()->data( index, SourcesModel::SourceTreeItemTypeRole ).toInt() == SourcesModel::CategoryAdd )
    {
        SourceTreeItem* item = itemFromIndex< SourceTreeItem >( index );
        Q_ASSERT( item );

        item->setDropType( m_delegate->hoveredDropType() );
        qDebug() << Q_FUNC_INFO << "dropType is " << m_delegate->hoveredDropType();
    }

    // if it's a playlist drop, accept it anywhere in the sourcetree by manually parsing it.
    if ( DropJob::isDropType( DropJob::Playlist, event->mimeData()  ) )
    {
        qDebug() << Q_FUNC_INFO << "Current Event";
        DropJob *dropThis = new DropJob;
        dropThis->setDropTypes( DropJob::Playlist );
        dropThis->setDropAction( DropJob::Create );
        dropThis->parseMimeData( event->mimeData() );

        // Don't add it to the playlist under drop, it's a new playlist now
        return;
    }

    // Need to fake the dropevent because the treeview would reject it if it is outside the item (on the tree)
    if ( pos.x() < 100 )
    {
        QDropEvent* newEvent = new QDropEvent( pos + QPoint( 100, 0 ), event->possibleActions(), event->mimeData(), event->mouseButtons(), event->keyboardModifiers(), event->type() );
        QTreeView::dropEvent( newEvent );
        delete newEvent;
    }
    else
    {
        QTreeView::dropEvent( event );
    }

    m_dragging = false;
    m_dropIndex = QPersistentModelIndex();
    m_delegate->dragLeaveEvent();
    dataChanged( index, index );
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


void
SourceTreeView::update( const QModelIndex &index )
{
    dataChanged( index, index );
}
