/*
 *    Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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

#include "sourceitem.h"

#include "categoryitems.h"
#include "playlistitems.h"
#include "viewmanager.h"
#include "playlist.h"
#include "genericpageitems.h"
#include "utils/tomahawkutilsgui.h"
#include "utils/logger.h"
#include "widgets/SocialPlaylistWidget.h"
#include "playlist/customplaylistview.h"
#include "source.h"
#include "sourcelist.h"

/// SourceItem

using namespace Tomahawk;

SourceItem::SourceItem( SourcesModel* mdl, SourceTreeItem* parent, const Tomahawk::source_ptr& source )
    : SourceTreeItem( mdl, parent, SourcesModel::Collection )
    , m_source( source )
    , m_playlists( 0 )
    , m_stations( 0 )
    , m_latchedOn( false )
    , m_sourceInfoItem( 0   )
    , m_coolPlaylistsItem( 0 )
    , m_collectionPage( 0 )
    , m_sourceInfoPage( 0 )
    , m_coolPlaylistsPage( 0 )
    , m_lovedTracksPage( 0 )
    , m_whatsHotPage( 0 )
{
    if ( m_source.isNull() )
    {
        m_superCol = TomahawkUtils::createAvatarFrame( QPixmap( RESPATH "images/supercollection.png" ) );
        return;
    }

    m_lovedTracksItem = new GenericPageItem( model(), this, tr( "Loved Tracks" ), QIcon( RESPATH "images/loved_playlist.png" ),
                                             boost::bind( &SourceItem::lovedTracksClicked, this ),
                                             boost::bind( &SourceItem::getLovedTracksPage, this ) );
    m_lovedTracksItem->setSortValue( -250 );

    m_collectionItem = new GenericPageItem( model(), this, tr( "Collection" ), QIcon( RESPATH "images/drop-song.png" ), //FIXME different icon
                                            boost::bind( &SourceItem::collectionClicked, this ),
                                            boost::bind( &SourceItem::getCollectionPage, this ) );
    m_collectionItem->setSortValue( -350 );

    m_sourceInfoItem = new GenericPageItem( model(), this, tr( "New Additions" ), QIcon( RESPATH "images/new-additions.png" ),
                                            boost::bind( &SourceItem::sourceInfoClicked, this ),
                                            boost::bind( &SourceItem::getSourceInfoPage, this ) );
    m_sourceInfoItem->setSortValue( -300 );

    // create category items if there are playlists to show, or stations to show
    QList< playlist_ptr > playlists = source->collection()->playlists();
    QList< dynplaylist_ptr > autoplaylists = source->collection()->autoPlaylists();
    QList< dynplaylist_ptr > stations = source->collection()->stations();

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

    if( ViewManager::instance()->pageForCollection( source->collection() ) )
        model()->linkSourceItemToPage( this, ViewManager::instance()->pageForCollection( source->collection() ) );

    m_defaultAvatar = TomahawkUtils::createAvatarFrame( QPixmap( RESPATH "images/user-avatar.png" ) );

    // load auto playlists and stations!

    connect( source.data(), SIGNAL( stats( QVariantMap ) ), SIGNAL( updated() ) );
    connect( source.data(), SIGNAL( syncedWithDatabase() ), SIGNAL( updated() ) );
    connect( source.data(), SIGNAL( playbackStarted( Tomahawk::query_ptr ) ), SIGNAL( updated() ) );
    connect( source.data(), SIGNAL( stateChanged() ), SIGNAL( updated() ) );
    connect( source.data(), SIGNAL( offline() ), SIGNAL( updated() ) );
    connect( source.data(), SIGNAL( online() ), SIGNAL( updated() ) );
    connect( SourceList::instance(), SIGNAL( sourceLatchedOn( Tomahawk::source_ptr, Tomahawk::source_ptr ) ), SLOT( latchedOn( Tomahawk::source_ptr, Tomahawk::source_ptr ) ) );
    connect( SourceList::instance(), SIGNAL( sourceLatchedOff( Tomahawk::source_ptr, Tomahawk::source_ptr ) ), SLOT( latchedOff( Tomahawk::source_ptr, Tomahawk::source_ptr ) ) );

    connect( source->collection().data(), SIGNAL( playlistsAdded( QList<Tomahawk::playlist_ptr> ) ),
             SLOT( onPlaylistsAdded( QList<Tomahawk::playlist_ptr> ) ), Qt::QueuedConnection );
    connect( source->collection().data(), SIGNAL( autoPlaylistsAdded( QList< Tomahawk::dynplaylist_ptr > ) ),
             SLOT( onAutoPlaylistsAdded( QList<Tomahawk::dynplaylist_ptr> ) ), Qt::QueuedConnection );
    connect( source->collection().data(), SIGNAL( stationsAdded( QList<Tomahawk::dynplaylist_ptr> ) ),
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
    return m_source.isNull() ? tr( "Super Collection" ) : m_source->friendlyName();
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
    if ( m_source.isNull() )
       return m_superCol;
    else
    {
        if ( m_source->avatar().isNull() )
            return m_defaultAvatar;
        else
            return m_source->avatar( Source::FancyStyle );
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


void
SourceItem::latchedOff( const source_ptr& from, const source_ptr& to )
{
    if ( from->isLocal() && ( m_source == to || m_source == from ) )
    {
        m_latchedOn = false;
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
        emit updated();
    }
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


template< typename T >
void
SourceItem::playlistDeletedInternal( SourceTreeItem* parent, const T& p )
{
    Q_ASSERT( parent ); // How can we delete playlists if we have none?
    int curCount = parent->children().count();
    for( int i = 0; i < curCount; i++ )
    {
        PlaylistItem* pl = qobject_cast< PlaylistItem* >( parent->children().at( i ) );
        if( pl && pl->playlist() == p )
        {
            parent->beginRowsRemoved( i, i );
            parent->removeChild( pl );
            parent->endRowsRemoved();

            delete pl;
            break;
        }
    }

    if( ( parent == m_playlists || parent == m_stations ) &&
         parent->children().isEmpty() && parent->parent() ) // Don't leave an empty Playlist or Station category
    {
        int idx = parent->parent()->children().indexOf( parent );
        if( idx < 0 )
            return;

        parent->parent()->beginRowsRemoved( idx, idx );
        parent->parent()->removeChild( parent );
        parent->parent()->endRowsRemoved();

        if( parent == m_playlists )
            m_playlists = 0;
        else if( parent == m_stations )
            m_stations = 0;
        delete parent;
    }
}


void
SourceItem::onPlaylistsAdded( const QList< playlist_ptr >& playlists )
{
//    qDebug() << Q_FUNC_INFO << m_source->friendlyName() << playlists.count();

    if( playlists.isEmpty() )
        return;

    if( !m_playlists )
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
    foreach( const playlist_ptr& p, playlists )
    {
        PlaylistItem* plItem = new PlaylistItem( model(), m_playlists, p, m_playlists->children().count() - addOffset );
//        qDebug() << "Playlist added:" << p->title() << p->creator() << p->info();
        p->loadRevision();
        items << plItem;

        if( m_source->isLocal() )
            connect( p.data(), SIGNAL( aboutToBeDeleted( Tomahawk::playlist_ptr ) ),
                    SLOT( onPlaylistDeleted( Tomahawk::playlist_ptr ) ), Qt::QueuedConnection );
        else
            connect( p.data(), SIGNAL( deleted( Tomahawk::playlist_ptr ) ),
                    SLOT( onPlaylistDeleted( Tomahawk::playlist_ptr ) ), Qt::QueuedConnection );

    }
    m_playlists->endRowsAdded();
}


void
SourceItem::onPlaylistDeleted( const  playlist_ptr& playlist )
{
    playlistDeletedInternal( m_playlists, playlist );
}


void
SourceItem::onAutoPlaylistsAdded( const QList< dynplaylist_ptr >& playlists )
{
    if( playlists.isEmpty() )
        return;

    if( !m_playlists )
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
    if( !m_playlists )
        qDebug() << "NO playlist category item for a deleting playlist..";

    playlistDeletedInternal( m_playlists, playlist );
}


void
SourceItem::onStationsAdded( const QList< dynplaylist_ptr >& stations )
{
    if( stations.isEmpty() )
        return;

    if( !m_stations )
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
    if( m_source.isNull() )
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
SourceItem::collectionClicked()
{
    if( m_source.isNull() )
        return 0;

    m_collectionPage = ViewManager::instance()->show( m_source->collection() );
    return m_collectionPage;
}


ViewPage*
SourceItem::getCollectionPage() const
{
    return m_collectionPage;;
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
SourceItem::lovedTracksClicked()
{
    if ( !m_lovedTracksPage )
        m_lovedTracksPage = new CustomPlaylistView( m_source.isNull() ? CustomPlaylistView::AllLovedTracks : CustomPlaylistView::SourceLovedTracks, m_source, ViewManager::instance()->widget() );

    ViewManager::instance()->show( m_lovedTracksPage );
    return m_lovedTracksPage;
}


ViewPage*
SourceItem::getLovedTracksPage() const
{
    return m_lovedTracksPage;
}
