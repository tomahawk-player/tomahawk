/*
 *    Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *    Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
 *    Copyright 2013,      Teo Mrnjavac <teo@kde.org>
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License along
 *    with this program; if not, write to the Free Software Foundation, Inc.,
 *    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "SourceItem.h"

#include "CategoryItems.h"
#include "PlaylistItems.h"
#include "ViewManager.h"
#include "Playlist.h"
#include "GenericPageItems.h"
#include "LovedTracksItem.h"
#include "Source.h"
#include "SourceList.h"
#include "widgets/SocialPlaylistWidget.h"
#include "playlist/FlexibleView.h"
#include "playlist/PlaylistView.h"
#include "playlist/RecentlyAddedModel.h"
#include "playlist/RecentlyPlayedModel.h"
#include "playlist/PlaylistLargeItemDelegate.h"
#include "sip/PeerInfo.h"
#include "sip/SipPlugin.h"
#include "utils/ImageRegistry.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"
#include "TomahawkApp.h"

/// SourceItem

using namespace Tomahawk;

SourceItem::SourceItem( SourcesModel* mdl, SourceTreeItem* parent, const Tomahawk::source_ptr& source )
    : SourceTreeItem( mdl, parent, SourcesModel::Collection )
    , m_source( source )
    , m_playlists( 0 )
    , m_stations( 0 )
    , m_latchedOn( false )
    , m_sourceInfoItem( 0 )
    , m_coolPlaylistsItem( 0 )
    , m_sourceInfoPage( 0 )
    , m_coolPlaylistsPage( 0 )
    , m_latestAdditionsPage( 0 )
    , m_recentPlaysPage( 0 )
    , m_whatsHotPage( 0 )
{
    if ( m_source.isNull() )
    {
        return;
    }

    connect( source.data(), SIGNAL( collectionAdded( Tomahawk::collection_ptr ) ),
             SLOT( onCollectionAdded( Tomahawk::collection_ptr ) ) );
    connect( source.data(), SIGNAL( collectionRemoved( Tomahawk::collection_ptr ) ),
             SLOT( onCollectionRemoved( Tomahawk::collection_ptr ) ) );

    foreach ( const Tomahawk::collection_ptr& collection, source->collections() )
    {
        performAddCollectionItem( collection );
    }

/*    m_sourceInfoItem = new GenericPageItem( model(), this, tr( "New Additions" ), QIcon( RESPATH "images/new-additions.png" ),
                                            boost::bind( &SourceItem::sourceInfoClicked, this ),
                                            boost::bind( &SourceItem::getSourceInfoPage, this ) );*/

    m_latestAdditionsItem = new GenericPageItem( model(), this, tr( "Latest Additions" ), ImageRegistry::instance()->icon( RESPATH "images/new-additions.svg" ),
                                                 boost::bind( &SourceItem::latestAdditionsClicked, this ),
                                                 boost::bind( &SourceItem::getLatestAdditionsPage, this ) );

    m_recentPlaysItem = new GenericPageItem( model(), this, tr( "Recently Played" ), ImageRegistry::instance()->icon( RESPATH "images/recently-played.svg" ),
                                             boost::bind( &SourceItem::recentPlaysClicked, this ),
                                             boost::bind( &SourceItem::getRecentPlaysPage, this ) );

    new LovedTracksItem( model(), this );

//    m_sourceInfoItem->setSortValue( -300 );
    m_latestAdditionsItem->setSortValue( -250 );
    m_recentPlaysItem->setSortValue( -200 );

    // create category items if there are playlists to show, or stations to show
    QList< playlist_ptr > playlists = source->dbCollection()->playlists();
    QList< dynplaylist_ptr > autoplaylists = source->dbCollection()->autoPlaylists();
    QList< dynplaylist_ptr > stations = source->dbCollection()->stations();

    if ( !playlists.isEmpty() || !autoplaylists.isEmpty() || source->isLocal() )
    {
        m_playlists = new CategoryItem( model(), this, SourcesModel::PlaylistsCategory, source->isLocal() );
        onPlaylistsAdded( playlists );
        onAutoPlaylistsAdded( autoplaylists );
    }
    if ( !stations.isEmpty() || source->isLocal() )
    {
        m_stations = new CategoryItem( model(), this, SourcesModel::StationsCategory, source->isLocal() );
        onStationsAdded( stations );
    }

/*    if ( ViewManager::instance()->pageForCollection( source->collection() ) )
        model()->linkSourceItemToPage( this, ViewManager::instance()->pageForCollection( source->collection() ) );*/

    // load auto playlists and stations!

    connect( source.data(), SIGNAL( stats( QVariantMap ) ), SIGNAL( updated() ) );
    connect( source.data(), SIGNAL( syncedWithDatabase() ), SIGNAL( updated() ) );
    connect( source.data(), SIGNAL( stateChanged() ), SIGNAL( updated() ) );
    connect( source.data(), SIGNAL( offline() ), SIGNAL( updated() ) );
    connect( source.data(), SIGNAL( online() ), SIGNAL( updated() ) );

    connect( SourceList::instance(), SIGNAL( sourceLatchedOn( Tomahawk::source_ptr, Tomahawk::source_ptr ) ), SLOT( latchedOn( Tomahawk::source_ptr, Tomahawk::source_ptr ) ) );
    connect( SourceList::instance(), SIGNAL( sourceLatchedOff( Tomahawk::source_ptr, Tomahawk::source_ptr ) ), SLOT( latchedOff( Tomahawk::source_ptr, Tomahawk::source_ptr ) ) );

    connect( source->dbCollection().data(), SIGNAL( playlistsAdded( QList<Tomahawk::playlist_ptr> ) ),
             SLOT( onPlaylistsAdded( QList<Tomahawk::playlist_ptr> ) ), Qt::QueuedConnection );
    connect( source->dbCollection().data(), SIGNAL( autoPlaylistsAdded( QList< Tomahawk::dynplaylist_ptr > ) ),
             SLOT( onAutoPlaylistsAdded( QList<Tomahawk::dynplaylist_ptr> ) ), Qt::QueuedConnection );
    connect( source->dbCollection().data(), SIGNAL( stationsAdded( QList<Tomahawk::dynplaylist_ptr> ) ),
             SLOT( onStationsAdded( QList<Tomahawk::dynplaylist_ptr> ) ), Qt::QueuedConnection );

    if ( m_source->isLocal() )
        QTimer::singleShot( 0, this, SLOT( requestExpanding() ) );
}


Tomahawk::source_ptr
SourceItem::source() const
{
    return m_source;
}


QString
SourceItem::text() const
{
    return m_source.isNull() ? tr( "SuperCollection" ) : m_source->friendlyName();
}


QString
SourceItem::tooltip() const
{
    if ( m_source.isNull() || m_source->peerInfos().isEmpty() )
        return QString();

    QString t;

    bool showDebugInfo = APP->arguments().contains( "--verbose" );
    if ( showDebugInfo )
    {
        // This is kind of debug output for now.
        t.append( "<PRE>" );

        QString narf("%1: %2\n");
        t.append( narf.arg( "id" ).arg( m_source->id() ) );
        t.append( narf.arg( "username" ).arg( m_source->nodeId() ) );
        t.append( narf.arg( "friendlyname" ).arg( m_source->friendlyName() ) );
        t.append( narf.arg( "dbfriendlyname" ).arg( m_source->dbFriendlyName() ) );

        t.append("\n");
        foreach( Tomahawk::peerinfo_ptr p, m_source->peerInfos() )
        {
            QString line( p->sipPlugin()->serviceName() + p->sipPlugin()->friendlyName() + ": " + p->id() + " " + p->friendlyName() );
            t.append( line + "\n\n" );
        }
        t.append( "</PRE>" );
    }

    if ( !m_source->currentTrack().isNull() )
        t.append( m_source->textStatus() );

    return t;
}


int
SourceItem::IDValue() const
{
    if ( m_source.isNull() )
        return -1;
    if ( m_source->isLocal() )
        return 0;

    return m_source->id();
}


int
SourceItem::peerSortValue() const
{
    if ( m_source.isNull() || m_source->isLocal() )
        return -1;

    return 1;
}


void
SourceItem::activate()
{
    ViewPage* p = 0;
    if ( source().isNull() )
        p = ViewManager::instance()->showSuperCollection();
    else
        emit toggleExpandRequest( this );

    model()->linkSourceItemToPage( this, p );
}


QIcon
SourceItem::icon() const
{
    return pixmap();
}


QPixmap
SourceItem::pixmap( const QSize& size ) const
{
    if ( m_source.isNull() )
    {
       return TomahawkUtils::defaultPixmap( TomahawkUtils::SuperCollection, TomahawkUtils::Original, size );
    }
    else
    {
        if ( m_source->avatar().isNull() )
            return TomahawkUtils::defaultPixmap( TomahawkUtils::DefaultSourceAvatar, TomahawkUtils::RoundedCorners );
        else
            return m_source->avatar( TomahawkUtils::RoundedCorners, size );
    }
}


bool
SourceItem::localLatchedOn() const
{
    // Don't show a listen icon if this is the local collection and we are latched on to someone who went offline
    // we are technically still latched on (if they come back online we'll be still listening along) but it's not visible
    // in the UI and confusing to the user why the red headphones are still there

    if ( !m_source.isNull() && m_source->isLocal() &&
         !m_latchedOnTo.isNull() && !m_latchedOnTo->isOnline() )
        return false;

    return m_latchedOn;
}


Tomahawk::PlaylistModes::LatchMode
SourceItem::localLatchMode() const
{
    if ( !m_source.isNull() && !m_source->isLocal() )
        return m_source->playlistInterface()->latchMode();

    return Tomahawk::PlaylistModes::StayOnSong;
}


void
SourceItem::latchedOff( const source_ptr& from, const source_ptr& to )
{
    if ( from->isLocal() && ( m_source == to || m_source == from ) )
    {
        m_latchedOn = false;
        disconnect( m_latchedOnTo->playlistInterface().data(), SIGNAL( latchModeChanged( Tomahawk::PlaylistModes::LatchMode ) ) );
        m_latchedOnTo.clear();
        emit updated();
    }
}


void
SourceItem::latchedOn( const source_ptr& from, const source_ptr& to )
{
    if ( from->isLocal() && ( m_source == to || m_source == from ) )
    {
        m_latchedOn = true;
        m_latchedOnTo = to;
        connect( m_latchedOnTo->playlistInterface().data(), SIGNAL( latchModeChanged( Tomahawk::PlaylistModes::LatchMode ) ), SLOT( latchModeChanged( Tomahawk::PlaylistModes::LatchMode ) ) );
        emit updated();
    }
}


void
SourceItem::latchModeChanged( Tomahawk::PlaylistModes::LatchMode mode )
{
    Q_UNUSED( mode );
    emit updated();
}


void
SourceItem::onCollectionAdded( const collection_ptr& collection )
{
    if ( m_collectionItems.contains( collection ) )
        return;

    beginRowsAdded( model()->rowCount( model()->indexFromItem( this ) ),
                    model()->rowCount( model()->indexFromItem( this ) ) );
    performAddCollectionItem( collection );
    endRowsAdded();
}


void
SourceItem::onCollectionRemoved( const collection_ptr& collection )
{
    GenericPageItem* item = m_collectionItems.value( collection );
    int row = model()->indexFromItem( item ).row();

    beginRowsRemoved( row, row );
    removeChild( item );
    endRowsRemoved();

    m_collectionPages.remove( collection );
    m_collectionItems.remove( collection );
    item->deleteLater();
}


void
SourceItem::playlistsAddedInternal( SourceTreeItem* parent, const QList< dynplaylist_ptr >& playlists )
{
    QList< SourceTreeItem* > items;
    int addOffset = playlists.first()->author()->isLocal() ? 1 : 0;

    int from = parent->children().count() - addOffset;
    parent->beginRowsAdded( from, from + playlists.count() - 1 );
    foreach ( const dynplaylist_ptr& p, playlists )
    {
        DynamicPlaylistItem* plItem = new DynamicPlaylistItem( model(), parent, p, parent->children().count() - addOffset );
//        qDebug() << "Dynamic Playlist added:" << p->title() << p->creator() << p->info();
        p->loadRevision();
        items << plItem;

        if ( p->mode() == Static )
        {
            if ( m_source->isLocal() )
                connect( p.data(), SIGNAL( aboutToBeDeleted( Tomahawk::dynplaylist_ptr ) ),
                        SLOT( onAutoPlaylistDeleted( Tomahawk::dynplaylist_ptr ) ), Qt::QueuedConnection );
            else
                connect( p.data(), SIGNAL( deleted( Tomahawk::dynplaylist_ptr ) ),
                        SLOT( onAutoPlaylistDeleted( Tomahawk::dynplaylist_ptr ) ), Qt::QueuedConnection );
        }
        else
        {
            if ( m_source->isLocal() )
                connect( p.data(), SIGNAL( aboutToBeDeleted( Tomahawk::dynplaylist_ptr ) ),
                        SLOT( onStationDeleted( Tomahawk::dynplaylist_ptr ) ), Qt::QueuedConnection );
            else
                connect( p.data(), SIGNAL( deleted( Tomahawk::dynplaylist_ptr ) ),
                        SLOT( onStationDeleted( Tomahawk::dynplaylist_ptr ) ), Qt::QueuedConnection );
        }
    }
    parent->endRowsAdded();
}


void
SourceItem::performAddCollectionItem( const collection_ptr& collection )
{
    GenericPageItem* item = new GenericPageItem( model(),
                                                 this,
                                                 collection->itemName(),
                                                 collection->icon(),
                                                 boost::bind( &SourceItem::collectionClicked, this, collection ),
                                                 boost::bind( &SourceItem::getCollectionPage, this, collection ) );

    if ( collection->backendType() == Collection::DatabaseCollectionType )
        item->setSortValue( -350 );
    else
        item->setSortValue( -340 );

    m_collectionItems.insert( collection, item );
}


template< typename T >
void
SourceItem::playlistDeletedInternal( SourceTreeItem* parent, const T& p )
{
    Q_ASSERT( parent ); // How can we delete playlists if we have none?
    int curCount = parent->children().count();
    for( int i = 0; i < curCount; i++ )
    {
        PlaylistItem* pl = qobject_cast< PlaylistItem* >( parent->children().at( i ) );
        if ( pl && pl->playlist() == p )
        {
            parent->beginRowsRemoved( i, i );
            parent->removeChild( pl );
            parent->endRowsRemoved();

            delete pl;
            break;
        }
    }

    if ( ( parent == m_playlists || parent == m_stations ) &&
         parent->children().isEmpty() && parent->parent() ) // Don't leave an empty Playlist or Station category
    {
        int idx = parent->parent()->children().indexOf( parent );
        if ( idx < 0 )
            return;

        parent->parent()->beginRowsRemoved( idx, idx );
        parent->parent()->removeChild( parent );
        parent->parent()->endRowsRemoved();

        if ( parent == m_playlists )
            m_playlists = 0;
        else if ( parent == m_stations )
            m_stations = 0;
        delete parent;
    }
}


void
SourceItem::onPlaylistsAdded( const QList< playlist_ptr >& playlists )
{
    if ( playlists.isEmpty() )
        return;

    if ( !m_playlists )
    {
        // add the category too
        int cur = children().count();
        beginRowsAdded( cur, cur );
        m_playlists = new CategoryItem( model(), this, SourcesModel::PlaylistsCategory, source()->isLocal() );
        endRowsAdded();
    }

    QList< SourceTreeItem* > items;
    int addOffset = playlists.first()->author()->isLocal() ? 1 : 0;

    int from = m_playlists->children().count() - addOffset;
    m_playlists->beginRowsAdded( from, from + playlists.count() - 1 );
    foreach ( const playlist_ptr& p, playlists )
    {
        PlaylistItem* plItem = new PlaylistItem( model(), m_playlists, p, m_playlists->children().count() - addOffset );
        p->loadRevision();
        items << plItem;

        if ( m_source->isLocal() )
            connect( p.data(), SIGNAL( aboutToBeDeleted( Tomahawk::playlist_ptr ) ),
                     SLOT( onPlaylistDeleted( Tomahawk::playlist_ptr ) ), Qt::QueuedConnection );
        else
            connect( p.data(), SIGNAL( deleted( Tomahawk::playlist_ptr ) ),
                     SLOT( onPlaylistDeleted( Tomahawk::playlist_ptr ) ), Qt::QueuedConnection );

    }
    m_playlists->endRowsAdded();
}


void
SourceItem::onPlaylistDeleted( const playlist_ptr& playlist )
{
    playlistDeletedInternal( m_playlists, playlist );
}


void
SourceItem::onAutoPlaylistsAdded( const QList< dynplaylist_ptr >& playlists )
{
    if ( playlists.isEmpty() )
        return;

    if ( !m_playlists )
    {
        // add the category too
        int cur = children().count();
        beginRowsAdded( cur, cur );
        m_playlists = new CategoryItem( model(), this, SourcesModel::PlaylistsCategory, source()->isLocal() );
        endRowsAdded();
    }

    playlistsAddedInternal( m_playlists, playlists );
}


void
SourceItem::onAutoPlaylistDeleted( const dynplaylist_ptr& playlist )
{
    if ( !m_playlists )
        qDebug() << "NO playlist category item for a deleting playlist...";

    playlistDeletedInternal( m_playlists, playlist );
}


void
SourceItem::onStationsAdded( const QList< dynplaylist_ptr >& stations )
{
    if ( stations.isEmpty() )
        return;

    if ( !m_stations )
    {
        // add the category too
        int cur = children().count();
        beginRowsAdded( cur, cur );
        m_stations = new CategoryItem( model(), this, SourcesModel::StationsCategory, source()->isLocal() );
        endRowsAdded();
    }

    playlistsAddedInternal( m_stations, stations );
}


void
SourceItem::onStationDeleted( const dynplaylist_ptr& station )
{
    playlistDeletedInternal( m_stations, station );
}


void
SourceItem::requestExpanding()
{
    emit expandRequest( this );
}


ViewPage*
SourceItem::sourceInfoClicked()
{
    if ( m_source.isNull() )
        return 0;

    m_sourceInfoPage = ViewManager::instance()->show( m_source );
    return m_sourceInfoPage;
}


ViewPage*
SourceItem::getSourceInfoPage() const
{
    return m_sourceInfoPage;
}


ViewPage*
SourceItem::collectionClicked( const Tomahawk::collection_ptr& collection )
{
    if ( m_source.isNull() )
        return 0;

    m_collectionPages[ collection ] = ViewManager::instance()->show( collection );
    return m_collectionPages[ collection ];
}


ViewPage*
SourceItem::getCollectionPage( const Tomahawk::collection_ptr& collection ) const
{    
    return m_collectionPages[ collection ];
}


ViewPage*
SourceItem::coolPlaylistsClicked()
{
    if ( !m_source.isNull() )
        return 0;

    if ( !m_coolPlaylistsPage )
        m_coolPlaylistsPage = new SocialPlaylistWidget( ViewManager::instance()->widget() );

    ViewManager::instance()->show( m_coolPlaylistsPage );
    return m_coolPlaylistsPage;
}


ViewPage*
SourceItem::getCoolPlaylistsPage() const
{
    return m_coolPlaylistsPage;
}


ViewPage*
SourceItem::latestAdditionsClicked()
{
    if ( !m_latestAdditionsPage )
    {
        FlexibleView* pv = new FlexibleView( ViewManager::instance()->widget() );
        pv->setPixmap( TomahawkUtils::defaultPixmap( TomahawkUtils::NewAdditions, TomahawkUtils::Original ) );

        RecentlyAddedModel* raModel = new RecentlyAddedModel( pv );
        raModel->setTitle( tr( "Latest Additions" ) );

        if ( m_source->isLocal() )
            raModel->setDescription( tr( "Latest additions to your collection" ) );
        else
            raModel->setDescription( tr( "Latest additions to %1's collection" ).arg( m_source->friendlyName() ) );

        PlaylistLargeItemDelegate* del = new PlaylistLargeItemDelegate( PlaylistLargeItemDelegate::LatestAdditions, pv->trackView(), pv->trackView()->proxyModel() );
        connect( del, SIGNAL( updateIndex( QModelIndex ) ), pv->trackView(), SLOT( update( QModelIndex ) ) );
        pv->trackView()->setItemDelegate( del );

        pv->setPlayableModel( raModel );
        pv->trackView()->sortByColumn( PlayableModel::Age, Qt::DescendingOrder );
        pv->detailedView()->sortByColumn( PlayableModel::Age, Qt::DescendingOrder );
        pv->setEmptyTip( tr( "Sorry, we could not find any recent additions!" ) );
        raModel->setSource( m_source );

        pv->setGuid( QString( "latestadditions/%1" ).arg( m_source->nodeId() ) );

        m_latestAdditionsPage = pv;
    }

    ViewManager::instance()->show( m_latestAdditionsPage );
    return m_latestAdditionsPage;
}


ViewPage*
SourceItem::getLatestAdditionsPage() const
{
    return m_latestAdditionsPage;
}


ViewPage*
SourceItem::recentPlaysClicked()
{
    if ( !m_recentPlaysPage )
    {
        FlexibleView* pv = new FlexibleView( ViewManager::instance()->widget() );
        pv->setPixmap( TomahawkUtils::defaultPixmap( TomahawkUtils::RecentlyPlayed ) );

        RecentlyPlayedModel* raModel = new RecentlyPlayedModel( pv );
        raModel->setTitle( tr( "Recently Played Tracks" ) );

        if ( m_source->isLocal() )
            raModel->setDescription( tr( "Your recently played tracks" ) );
        else
            raModel->setDescription( tr( "%1's recently played tracks" ).arg( m_source->friendlyName() ) );

        PlaylistLargeItemDelegate* del = new PlaylistLargeItemDelegate( PlaylistLargeItemDelegate::RecentlyPlayed, pv->trackView(), pv->trackView()->proxyModel() );
        connect( del, SIGNAL( updateIndex( QModelIndex ) ), pv->trackView(), SLOT( update( QModelIndex ) ) );
        pv->trackView()->setItemDelegate( del );

        pv->setPlayableModel( raModel );
        pv->setEmptyTip( tr( "Sorry, we could not find any recent plays!" ) );
        raModel->setSource( m_source );

        pv->setGuid( QString( "recentplays/%1" ).arg( m_source->nodeId() ) );

        m_recentPlaysPage = pv;
    }

    ViewManager::instance()->show( m_recentPlaysPage );
    return m_recentPlaysPage;
}


ViewPage*
SourceItem::getRecentPlaysPage() const
{
    return m_recentPlaysPage;
}


CategoryItem*
SourceItem::stationsCategory() const
{
    return m_stations;
}


CategoryItem*
SourceItem::playlistsCategory() const
{
    return m_playlists;
}


void
SourceItem::setStationsCategory(CategoryItem* item)
{
    m_stations = item;
}


void
SourceItem::setPlaylistsCategory(CategoryItem* item)
{
    m_playlists = item;
}
