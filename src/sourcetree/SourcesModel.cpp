/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include "sourcetree/SourcesModel.h"

#include <QMimeData>
#include <QSize>

#include <boost/bind.hpp>

#include "sourcetree/items/ScriptCollectionItem.h"
#include "sourcetree/items/SourceTreeItem.h"
#include "sourcetree/items/SourceItem.h"
#include "sourcetree/items/GroupItem.h"
#include "sourcetree/items/GenericPageItems.h"
#include "sourcetree/items/HistoryItem.h"
#include "sourcetree/items/LovedTracksItem.h"
#include "SourceList.h"
#include "Playlist.h"
#include "collection/Collection.h"
#include "Source.h"
#include "ViewManager.h"
#include "GlobalActionManager.h"
#include "DropJob.h"
#include "items/PlaylistItems.h"
#include "playlist/TreeView.h"
#include "playlist/PlaylistView.h"
#include "playlist/dynamic/widgets/DynamicWidget.h"
#include "utils/ImageRegistry.h"
#include "utils/Logger.h"

using namespace Tomahawk;


SourcesModel::SourcesModel( QObject* parent )
    : QAbstractItemModel( parent )
    , m_rootItem( 0 )
    , m_viewPageDelayedCacheItem( 0 )
{
    m_rootItem = new SourceTreeItem( this, 0, Invalid );

    appendGroups();
    onSourcesAdded( SourceList::instance()->sources() );

    connect( SourceList::instance(), SIGNAL( sourceAdded( Tomahawk::source_ptr ) ),
             SLOT( onSourceAdded( Tomahawk::source_ptr ) ) );
    connect( SourceList::instance(), SIGNAL( sourceRemoved( Tomahawk::source_ptr ) ),
             SLOT( onSourceRemoved( Tomahawk::source_ptr ) ) );
    connect( ViewManager::instance(), SIGNAL( viewPageActivated( Tomahawk::ViewPage* ) ),
             this, SLOT( viewPageActivated( Tomahawk::ViewPage* ) ) );

    foreach ( const collection_ptr& c, SourceList::instance()->scriptCollections() )
    {
        onScriptCollectionAdded( c );
    }

    connect( SourceList::instance(), SIGNAL( scriptCollectionAdded( Tomahawk::collection_ptr ) ),
             this, SLOT( onScriptCollectionAdded( Tomahawk::collection_ptr ) ) );
    connect( SourceList::instance(), SIGNAL( scriptCollectionRemoved( Tomahawk::collection_ptr ) ),
             this, SLOT( onScriptCollectionRemoved( Tomahawk::collection_ptr ) ) );
}


SourcesModel::~SourcesModel()
{
    delete m_rootItem;
}


QString
SourcesModel::rowTypeToString( RowType type )
{
    switch ( type )
    {
        case Group:
            return tr( "Group" );

        case Collection:
            return tr( "Collection" );

        case StaticPlaylist:
            return tr( "Playlist" );

        case AutomaticPlaylist:
            return tr( "Automatic Playlist" );

        case Station:
            return tr( "Station" );

        default:
            return QString( "Unknown" );
    }
}


QVariant
SourcesModel::data( const QModelIndex& index, int role ) const
{
    if ( !index.isValid() )
        return QVariant();

    SourceTreeItem* item = itemFromIndex( index );
    if ( !item )
        return QVariant();

    switch ( role )
    {
    case Qt::SizeHintRole:
        return QSize( 0, 18 );
    case SourceTreeItemRole:
        return QVariant::fromValue< SourceTreeItem* >( item );
    case SourceTreeItemTypeRole:
        return item->type();
    case Qt::DisplayRole:
    case Qt::EditRole:
        return item->text();
    case Qt::DecorationRole:
        return item->icon();
    case SourcesModel::SortRole:
        return item->peerSortValue();
    case SourcesModel::IDRole:
        return item->IDValue();
    case SourcesModel::LatchedOnRole:
    {
        if ( item->type() == Collection )
        {
            SourceItem* cItem = qobject_cast< SourceItem* >( item );
            return cItem->localLatchedOn();
        }
        return false;
    }
    case SourcesModel::LatchedRealtimeRole:
    {
        if ( item->type() == Collection )
        {
            SourceItem* cItem = qobject_cast< SourceItem* >( item );
            return cItem->localLatchMode() == Tomahawk::PlaylistModes::RealTime;
        }
        return false;
    }
    case SourcesModel::CustomActionRole:
    {
        return QVariant::fromValue< QList< QAction* > >( item->customActions() );
    }
    case Qt::ToolTipRole:
        if ( !item->tooltip().isEmpty() )
            return item->tooltip();
    }
    return QVariant();
}


int
SourcesModel::columnCount( const QModelIndex& ) const
{
    return 1;
}


int
SourcesModel::rowCount( const QModelIndex& parent ) const
{
    if( !parent.isValid() )
    {
        return m_rootItem->children().count();
    }

    return itemFromIndex( parent )->children().count();
}


QModelIndex
SourcesModel::parent( const QModelIndex& child ) const
{
    if( !child.isValid() )
    {
        return QModelIndex();
    }

    SourceTreeItem* node = itemFromIndex( child );
    SourceTreeItem* parent = node->parent();
    if( parent == m_rootItem )
        return QModelIndex();

    return createIndex( rowForItem( parent ), 0, parent );
}


QModelIndex
SourcesModel::index( int row, int column, const QModelIndex& parent ) const
{
    if( row < 0 || column < 0 )
        return QModelIndex();

    if( hasIndex( row, column, parent ) )
    {
        SourceTreeItem *parentNode = itemFromIndex( parent );
        SourceTreeItem *childNode = parentNode->children().at( row );
        return createIndex( row, column, childNode );
    }

    return QModelIndex();

}


bool
SourcesModel::setData( const QModelIndex& index, const QVariant& value, int role )
{
    SourceTreeItem* item = itemFromIndex( index );
    return item->setData( value, role );
}


QStringList
SourcesModel::mimeTypes() const
{
    return DropJob::mimeTypes();
}


QMimeData*
SourcesModel::mimeData( const QModelIndexList& ) const
{
    // TODO
    return new QMimeData();
}


bool
SourcesModel::dropMimeData( const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent )
{
    SourceTreeItem* item = 0;
//    qDebug() << "Got mime data dropped:" << row << column << parent << itemFromIndex( parent )->text();
    if( row == -1 && column == -1 )
        item = itemFromIndex( parent );
    else if( column == 0 )
        item = itemFromIndex( index( row, column, parent ) );
    else if( column == -1 ) // column is -1, that means the drop is happening "below" the indices. that means we actually want the one before it
        item = itemFromIndex( index( row - 1, 0, parent ) );

    Q_ASSERT( item );

//    qDebug() << "Dropping on:" << item->text();
    return item->dropMimeData( data, action );
}


Qt::DropActions
SourcesModel::supportedDropActions() const
{
#ifdef Q_WS_MAC
    return Qt::CopyAction | Qt::MoveAction;
#else
    return Qt::CopyAction;
#endif
}


Qt::ItemFlags
SourcesModel::flags( const QModelIndex& index ) const
{
    if ( index.isValid() )
    {
        return itemFromIndex( index )->flags();
    }
    else
        return 0;
}


void
SourcesModel::appendGroups()
{
    beginInsertRows( QModelIndex(), rowCount(), rowCount() + 4 );

    GroupItem* browse = new GroupItem( this, m_rootItem, tr( "Browse" ), 0 );
    new HistoryItem( this, m_rootItem, tr( "Search History" ), 1 );
//    new SourceTreeItem( this, m_rootItem, SourcesModel::Divider, 2 );
    m_myMusicGroup = new GroupItem( this, m_rootItem, tr( "My Music" ), 3 );

    GenericPageItem* dashboard = new GenericPageItem( this, browse, tr( "Dashboard" ), ImageRegistry::instance()->icon( RESPATH "images/dashboard.svg" ),
                                                      boost::bind( &ViewManager::showWelcomePage, ViewManager::instance() ),
                                                      boost::bind( &ViewManager::welcomeWidget, ViewManager::instance() ) );
    dashboard->setSortValue( 0 );

    // super collection
    GenericPageItem* sc = new GenericPageItem( this, browse, tr( "SuperCollection" ), ImageRegistry::instance()->icon( RESPATH "images/supercollection.svg" ),
                                                  boost::bind( &ViewManager::showSuperCollection, ViewManager::instance() ),
                                                  boost::bind( &ViewManager::superCollectionView, ViewManager::instance() ) );
    sc->setSortValue( 1 );

    // browse section
    LovedTracksItem* loved = new LovedTracksItem( this, browse );
    loved->setSortValue( 2 );

    GenericPageItem* recent = new GenericPageItem( this, browse, tr( "Recently Played" ), ImageRegistry::instance()->icon( RESPATH "images/recently-played.svg" ),
                                                   boost::bind( &ViewManager::showRecentPlaysPage, ViewManager::instance() ),
                                                   boost::bind( &ViewManager::recentPlaysWidget, ViewManager::instance() ) );
    recent->setSortValue( 3 );

    GenericPageItem* hot = new GenericPageItem( this, browse, tr( "Charts" ), ImageRegistry::instance()->icon( RESPATH "images/charts.svg" ),
                                                boost::bind( &ViewManager::showWhatsHotPage, ViewManager::instance() ),
                                                boost::bind( &ViewManager::whatsHotWidget, ViewManager::instance() ) );
    hot->setSortValue( 4 );

    GenericPageItem* newReleases = new GenericPageItem( this, browse, tr( "New Releases" ), ImageRegistry::instance()->icon( RESPATH "images/new-releases.svg" ),
                                                boost::bind( &ViewManager::showNewReleasesPage, ViewManager::instance() ),
                                                boost::bind( &ViewManager::newReleasesWidget, ViewManager::instance() ) );
    newReleases->setSortValue( 5 );

    m_collectionsGroup = new GroupItem( this, m_rootItem, tr( "Friends" ), 4 );

    m_cloudGroup = new GroupItem( this, m_rootItem, tr( "Cloud" ), 5 );

    endInsertRows();
}


void
SourcesModel::appendItem( const Tomahawk::source_ptr& source )
{
    GroupItem* parent;
    if ( !source.isNull() && source->isLocal() )
    {
        parent = m_myMusicGroup;
    }
    else
    {
        parent = m_collectionsGroup;
    }

    QModelIndex idx = indexFromItem( parent );
    beginInsertRows( idx, rowCount( idx ), rowCount( idx ) );
    new SourceItem( this, parent, source );
    endInsertRows();

    parent->checkExpandedState();
}


bool
SourcesModel::removeItem( const Tomahawk::source_ptr& source )
{
//    qDebug() << "Removing source item from SourceTree:" << source->friendlyName();

    QModelIndex idx;
    int rows = rowCount();
    for ( int row = 0; row < rows; row++ )
    {
        QModelIndex idx = index( row, 0, QModelIndex() );
        SourceItem* item = static_cast< SourceItem* >( idx.internalPointer() );
        if ( item && item->source() == source )
        {
//            qDebug() << "Found removed source item:" << item->source()->userName();
            beginRemoveRows( QModelIndex(), row, row );
            m_rootItem->removeChild( item );
            endRemoveRows();

//             onItemOffline( idx );

            delete item;
            return true;
        }
    }

    return false;
}


void
SourcesModel::viewPageActivated( Tomahawk::ViewPage* page )
{
    if ( !m_sourcesWithViewPage.isEmpty() )
    {
        // Hide again any offline sources we exposed, since we're showing a different page now. they'll be re-shown if the user selects a playlist that is from an offline user
        QList< source_ptr > temp = m_sourcesWithViewPage;
        m_sourcesWithViewPage.clear();
        foreach ( const source_ptr& s, temp )
        {
            QModelIndex idx = indexFromItem( m_sourcesWithViewPageItems.value( s ) );
            emit dataChanged( idx, idx );
        }
        m_sourcesWithViewPageItems.clear();
    }

    if ( m_sourceTreeLinks.contains( page ) )
    {
        Q_ASSERT( m_sourceTreeLinks[ page ] );
//        qDebug() << "Got view page activated for item:" << m_sourceTreeLinks[ page ]->text();
        QModelIndex idx = indexFromItem( m_sourceTreeLinks[ page ] );

        if ( !idx.isValid() )
            m_sourceTreeLinks.remove( page );
        else
            emit selectRequest( QPersistentModelIndex( idx ) );
    }
    else
    {
        playlist_ptr p = ViewManager::instance()->playlistForPage( page );
        // HACK
        // try to find it if it is a playlist. not pretty at all.... but this happens when ViewManager loads a playlist or dynplaylist NOT from the sidebar but from somewhere else
        // we don't know which sourcetreeitem is related to it, so we have to find it. we also don't know if this page is a playlist or dynplaylist or not, but we can't check as we can't
        // include DynamicWidget.h here (so can't dynamic_cast).
        // this could also be fixed by keeping a master list of playlists/sourcetreeitems... but that's even uglier i think. this is only called the first time a certain viewpage is clicked from external
        // sources.
        SourceTreeItem* item = activatePlaylistPage( page, m_rootItem );
        m_viewPageDelayedCacheItem = page;

        if ( !p.isNull() )
        {
            source_ptr s= p->author();
            if ( !s.isNull() && !s->isOnline() && item )
            {
                m_sourcesWithViewPage << s;

                // show the collection now... yeah.
                if ( !item->parent() || !item->parent()->parent() )
                {
                    tLog() << "Found playlist item with no category parent or collection parent!" << item->text();
                    return;
                }

                SourceTreeItem* collectionOfPlaylist = item->parent()->parent();
                if ( !m_rootItem->children().contains( collectionOfPlaylist ) ) // verification to make sure we're not stranded
                {
                    tLog() << "Got what we assumed to be a parent col of a playlist not as a child of our root node...:" << collectionOfPlaylist;
                    return;
                }

                QModelIndex idx = indexFromItem( collectionOfPlaylist );
                m_sourcesWithViewPageItems[ s ] = collectionOfPlaylist;
                tDebug() << "Emitting dataChanged for offline source:" << idx << idx.isValid() << collectionOfPlaylist << collectionOfPlaylist->text();
                emit dataChanged( idx, idx );
            }
        }
    }
}


SourceTreeItem*
SourcesModel::activatePlaylistPage( ViewPage* p, SourceTreeItem* i )
{
    if( !i )
        return 0;

    if( qobject_cast< PlaylistItem* >( i ) &&
        qobject_cast< PlaylistItem* >( i )->activateCurrent() )
        return i;

    SourceTreeItem* ret = 0;
    for( int k = 0; k < i->children().size(); k++ )
    {
        if( SourceTreeItem* retItem = activatePlaylistPage( p, i->children().at( k ) ) )
            ret = retItem;
    }

    return ret;
}


void
SourcesModel::loadSources()
{
    QList<source_ptr> sources = SourceList::instance()->sources();

    foreach( const source_ptr& source, sources )
        appendItem( source );
}


void
SourcesModel::onSourcesAdded( const QList<source_ptr>& sources )
{
    foreach( const source_ptr& source, sources )
        appendItem( source );
}


void
SourcesModel::onSourceAdded( const source_ptr& source )
{
    appendItem( source );
}


void
SourcesModel::onSourceRemoved( const source_ptr& source )
{
    removeItem( source );
}


void
SourcesModel::onScriptCollectionAdded( const collection_ptr& collection )
{
    if ( m_scriptCollections.contains( collection ) )
        return;

    QModelIndex parent = indexFromItem( m_cloudGroup );
    beginInsertRows( parent, rowCount( parent ), rowCount( parent ) );
    ScriptCollectionItem* item = new ScriptCollectionItem( this,
                                                 m_cloudGroup,
                                                 collection );
    endInsertRows();

    m_scriptCollections.insert( collection, item );
    m_cloudGroup->checkExpandedState();
}


void
SourcesModel::onScriptCollectionRemoved( const collection_ptr& collection )
{
    SourceTreeItem* item = m_scriptCollections.value( collection );
    int row = indexFromItem( item ).row();

    QModelIndex parent = indexFromItem( m_cloudGroup );
    beginRemoveRows( parent, row, row );
    m_cloudGroup->removeChild( item );
    endRemoveRows();

    m_scriptCollectionPages.remove( collection );
    m_scriptCollections.remove( collection );
    item->deleteLater();
}


ViewPage*
SourcesModel::scriptCollectionClicked( const Tomahawk::collection_ptr& collection )
{
    m_scriptCollectionPages.insert( collection, ViewManager::instance()->show( collection ) );
    return m_scriptCollectionPages[ collection ];
}


ViewPage*
SourcesModel::getScriptCollectionPage( const Tomahawk::collection_ptr& collection ) const
{
    return m_scriptCollectionPages[ collection ];
}


void
SourcesModel::itemUpdated()
{
    Q_ASSERT( qobject_cast< SourceTreeItem* >( sender() ) );
    SourceTreeItem* item = qobject_cast< SourceTreeItem* >( sender() );

    if( !item )
        return;

    QModelIndex idx = indexFromItem( item );
    if( idx.isValid() )
        emit dataChanged( idx, idx );
}


void
SourcesModel::onItemRowsAddedBegin( int first, int last )
{
    Q_ASSERT( qobject_cast< SourceTreeItem* >( sender() ) );
    SourceTreeItem* item = qobject_cast< SourceTreeItem* >( sender() );

    if( !item )
        return;

    QModelIndex idx = indexFromItem( item );
    beginInsertRows( idx, first, last );
}


void
SourcesModel::onItemRowsAddedDone()
{
    Q_ASSERT( qobject_cast< SourceTreeItem* >( sender() ) );

    endInsertRows();
}


void
SourcesModel::onItemRowsRemovedBegin( int first, int last )
{
    Q_ASSERT( qobject_cast< SourceTreeItem* >( sender() ) );
    SourceTreeItem* item = qobject_cast< SourceTreeItem* >( sender() );

    if( !item )
        return;

    QModelIndex idx = indexFromItem( item );
    beginRemoveRows( idx, first, last );
}


void
SourcesModel::onItemRowsRemovedDone()
{
    Q_ASSERT( qobject_cast< SourceTreeItem* >( sender() ) );

    endRemoveRows();
}


void
SourcesModel::linkSourceItemToPage( SourceTreeItem* item, ViewPage* p )
{
    // TODO handle removal
    m_sourceTreeLinks[ p ] = item;

    if ( p && m_viewPageDelayedCacheItem == p )
        emit selectRequest( QPersistentModelIndex( indexFromItem( item ) ) );

    if ( QObject* obj = dynamic_cast< QObject* >( p ) )
    {
        if( obj->metaObject()->indexOfSignal( "destroyed(QWidget*)" ) > -1 )
            connect( obj, SIGNAL( destroyed( QWidget* ) ), SLOT( onWidgetDestroyed( QWidget* ) ), Qt::UniqueConnection );
    }
    m_viewPageDelayedCacheItem = 0;
}


void
SourcesModel::onWidgetDestroyed( QWidget* w )
{
    int ret = m_sourceTreeLinks.remove( dynamic_cast< Tomahawk::ViewPage* > ( w ) );
    qDebug() << "REMOVED STALE SOURCE PAGE?" << ret;
}


void
SourcesModel::removeSourceItemLink( SourceTreeItem* item )
{
    QList< ViewPage* > pages = m_sourceTreeLinks.keys( item );
    foreach( ViewPage* p, pages )
        m_sourceTreeLinks.remove( p );
}


SourceTreeItem*
SourcesModel::itemFromIndex( const QModelIndex& idx ) const
{
    if( !idx.isValid() )
        return m_rootItem;

    Q_ASSERT( idx.internalPointer() );

    return reinterpret_cast< SourceTreeItem* >( idx.internalPointer() );
}


QModelIndex
SourcesModel::indexFromItem( SourceTreeItem* item ) const
{
    if( !item || !item->parent() ) // should never happen..
        return QModelIndex();

    // reconstructs a modelindex from a sourcetreeitem that is somewhere in the tree
    // traverses the item to the root node, then rebuilds the qmodeindices from there back down
    // each int is the row of that item in the parent.
    /**
     * In this diagram, if the \param item is G, childIndexList will contain [0, 2, 0]
     *
     *    A
     *      D
     *      E
     *      F
     *        G
     *        H
     *    B
     *    C
     *
     **/
    QList< int > childIndexList;
    SourceTreeItem* curItem = item;
    while( curItem != m_rootItem ) {
        int row  = rowForItem( curItem );
        if( row < 0 ) // something went wrong, bail
            return QModelIndex();

        childIndexList << row;

        curItem = curItem->parent();
    }
//     qDebug() << "build child index list:" << childIndexList;
    // now rebuild the qmodelindex we need
    QModelIndex idx;
    for( int i = childIndexList.size() - 1; i >= 0 ; i-- ) {
        idx = index( childIndexList[ i ], 0, idx );
    }
//     qDebug() << "Got index from item:" << idx << idx.data( Qt::DisplayRole ).toString();
//     qDebug() << "parent:" << idx.parent();
    return idx;
}


int
SourcesModel::rowForItem( SourceTreeItem* item ) const
{
    if ( !item || !item->parent() || !item->parent()->children().contains( item ) )
        return -1;

    return item->parent()->children().indexOf( item );
}


void
SourcesModel::itemSelectRequest( SourceTreeItem* item )
{
    emit selectRequest( QPersistentModelIndex( indexFromItem( item ) ) );
}


void
SourcesModel::itemExpandRequest( SourceTreeItem *item )
{
    emit expandRequest( QPersistentModelIndex( indexFromItem( item ) ) );
}


void
SourcesModel::itemToggleExpandRequest( SourceTreeItem *item )
{
    emit toggleExpandRequest( QPersistentModelIndex( indexFromItem( item ) ) );
}


QList< source_ptr >
SourcesModel::sourcesWithViewPage() const
{
    return m_sourcesWithViewPage;
}
