/*
 *
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

#include "playlistitems.h"

#include <QMimeData>

#include "query.h"
#include "viewmanager.h"
#include "playlist/dynamic/GeneratorInterface.h"
#include "categoryitems.h"
#include "sourceitem.h"
#include "utils/tomahawkutils.h"
#include "utils/logger.h"
#include "dropjob.h"
#include "source.h"
#include "audio/audioengine.h"

using namespace Tomahawk;


PlaylistItem::PlaylistItem( SourcesModel* mdl, SourceTreeItem* parent, const playlist_ptr& pl, int index )
    : SourceTreeItem( mdl, parent, SourcesModel::StaticPlaylist, index )
    , m_loaded( false )
    , m_playlist( pl )
{
    connect( pl.data(), SIGNAL( revisionLoaded( Tomahawk::PlaylistRevision ) ),
             SLOT( onPlaylistLoaded( Tomahawk::PlaylistRevision ) ), Qt::QueuedConnection );
    connect( pl.data(), SIGNAL( changed() ),
             SIGNAL( updated() ), Qt::QueuedConnection );

    m_icon = QIcon( RESPATH "images/playlist-icon.png" );

    if( ViewManager::instance()->pageForPlaylist( pl ) )
        model()->linkSourceItemToPage( this, ViewManager::instance()->pageForPlaylist( pl ) );
}


QString
PlaylistItem::text() const
{
    return m_playlist->title();
}


Tomahawk::playlist_ptr
PlaylistItem::playlist() const
{
    return m_playlist;
}


void
PlaylistItem::onPlaylistLoaded( Tomahawk::PlaylistRevision revision )
{
    Q_UNUSED( revision );

    m_loaded = true;
    emit updated();
}


void
PlaylistItem::onPlaylistChanged()
{
    emit updated();
}


int
PlaylistItem::peerSortValue() const
{
//    return m_playlist->createdOn();
    return 0;
}


int
PlaylistItem::IDValue() const
{
    return m_playlist->createdOn();
}

bool
PlaylistItem::isBeingPlayed() const
{
    if ( ViewManager::instance()->pageForPlaylist( m_playlist ) )
        return AudioEngine::instance()->currentTrackPlaylist() == ViewManager::instance()->pageForPlaylist( m_playlist )->playlistInterface();
    return false;
}


Qt::ItemFlags
PlaylistItem::flags() const
{
    Qt::ItemFlags flags = SourceTreeItem::flags();
    flags |= Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;

    if ( !m_loaded )
        flags &= !Qt::ItemIsEnabled;
    if ( playlist()->author()->isLocal() )
        flags |= Qt::ItemIsEditable;

    if ( playlist()->busy() )
    {
        flags &= !Qt::ItemIsEnabled;
        flags &= !Qt::ItemIsEditable;
        flags &= !Qt::ItemIsDropEnabled;
    }

    return flags;
}


void
PlaylistItem::activate()
{
    ViewPage* p = ViewManager::instance()->show( m_playlist );
    model()->linkSourceItemToPage( this, p );
}


void
PlaylistItem::setLoaded( bool loaded )
{
    m_loaded = loaded;
}


bool
PlaylistItem::willAcceptDrag( const QMimeData* data ) const
{
    Q_UNUSED( data );
    return !m_playlist.isNull() && m_playlist->author()->isLocal() && DropJob::acceptsMimeData( data, DropJob::Track ) && !m_playlist->busy();
}

PlaylistItem::DropTypes
PlaylistItem::supportedDropTypes( const QMimeData* data ) const
{
    if ( data->hasFormat( "application/tomahawk.query.list" ) )
        return DropTypeThisTrack | DropTypeThisAlbum | DropTypeAllFromArtist | DropTypeLocalItems | DropTypeTop50;
    else if ( data->hasFormat( "application/tomahawk.result.list" ) )
        return DropTypeThisTrack | DropTypeThisAlbum | DropTypeAllFromArtist | DropTypeLocalItems | DropTypeTop50;
    else if ( data->hasFormat( "application/tomahawk.metadata.album" ) )
        return DropTypeThisAlbum | DropTypeAllFromArtist | DropTypeLocalItems | DropTypeTop50;
    else if ( data->hasFormat( "application/tomahawk.metadata.artist" ) )
        return DropTypeAllFromArtist | DropTypeLocalItems | DropTypeTop50;
    else if ( data->hasFormat( "application/tomahawk.mixed" ) )
    {
        return DropTypesNone;
    }
    else if ( data->hasFormat( "text/plain" ) )
    {
        return DropTypesNone;
    }
    return DropTypesNone;
}


bool
PlaylistItem::dropMimeData( const QMimeData* data, Qt::DropAction action )
{
    Q_UNUSED( action );

    if ( m_playlist->busy() )
        return false;

    QList< Tomahawk::query_ptr > queries;

    if ( data->hasFormat( "application/tomahawk.playlist.id" ) &&
        data->data( "application/tomahawk.playlist.id" ) == m_playlist->guid() )
        return false; // don't allow dropping on ourselves

    if ( !DropJob::acceptsMimeData( data, DropJob::Track ) )
        return false;

    DropJob *dj = new DropJob();
    dj->setDropTypes( DropJob::Track );
    dj->setDropAction( DropJob::Append );

    connect( dj, SIGNAL( tracks( QList< Tomahawk::query_ptr > ) ), this, SLOT( parsedDroppedTracks( QList< Tomahawk::query_ptr > ) ) );

    if ( dropType() == DropTypeAllFromArtist )
        dj->setGetWholeArtists( true );
    if ( dropType() == DropTypeThisAlbum )
        dj->setGetWholeAlbums( true );

    if ( dropType() == DropTypeLocalItems )
    {
        dj->setGetWholeArtists( true );
        dj->tracksFromMimeData( data, false, true );
    }
    else if ( dropType() == DropTypeTop50 )
    {
        dj->setGetWholeArtists( true );
        dj->tracksFromMimeData( data, false, false, true );
    }
    else
        dj->tracksFromMimeData( data, false, false );

    // TODO can't know if it works or not yet...
    return true;
}


void
PlaylistItem::parsedDroppedTracks( const QList< query_ptr >& tracks )
{
    qDebug() << "adding" << tracks.count() << "tracks";
    if ( tracks.count() && !m_playlist.isNull() && m_playlist->author()->isLocal() )
    {
        qDebug() << "on playlist:" << m_playlist->title() << m_playlist->guid() << m_playlist->currentrevision();

        m_playlist->addEntries( tracks, m_playlist->currentrevision() );
    }
}


QIcon
PlaylistItem::icon() const
{
    return m_icon;
}


bool
PlaylistItem::setData( const QVariant& v, bool role )
{
    Q_UNUSED( role );

    if ( m_playlist->author()->isLocal() )
    {
        m_playlist->rename( v.toString() );

        return true;
    }
    return false;
}

SourceTreeItem*
PlaylistItem::activateCurrent()
{
    if( ViewManager::instance()->pageForPlaylist( m_playlist ) == ViewManager::instance()->currentPage() )
    {
        model()->linkSourceItemToPage( this, ViewManager::instance()->currentPage() );
        emit selectRequest( this );

        return this;
    }

    return 0;
}


DynamicPlaylistItem::DynamicPlaylistItem( SourcesModel* mdl, SourceTreeItem* parent, const dynplaylist_ptr& pl, int index )
    : PlaylistItem( mdl, parent, pl.staticCast< Playlist >(), index )
    , m_dynplaylist( pl )
{
    setRowType( m_dynplaylist->mode() == Static ? SourcesModel::AutomaticPlaylist : SourcesModel::Station );

    connect( pl.data(), SIGNAL( dynamicRevisionLoaded( Tomahawk::DynamicPlaylistRevision ) ),
             SLOT( onDynamicPlaylistLoaded( Tomahawk::DynamicPlaylistRevision ) ), Qt::QueuedConnection );

    m_stationIcon = QIcon( RESPATH "images/station.png" );
    m_automaticPlaylistIcon = QIcon( RESPATH "images/automatic-playlist.png" );

    if( ViewManager::instance()->pageForDynPlaylist( pl ) )
        model()->linkSourceItemToPage( this, ViewManager::instance()->pageForDynPlaylist( pl ) );
}


DynamicPlaylistItem::~DynamicPlaylistItem()
{
}


void
DynamicPlaylistItem::activate()
{
    ViewPage* p = ViewManager::instance()->show( m_dynplaylist );
    model()->linkSourceItemToPage( this, p );
}


void
DynamicPlaylistItem::onDynamicPlaylistLoaded( DynamicPlaylistRevision revision )
{
    setLoaded( true );
    checkReparentHackNeeded( revision );
    // END HACK
    emit updated();
}


int
DynamicPlaylistItem::peerSortValue() const
{
//    return m_dynplaylist->createdOn();
    return 0;
}


int
DynamicPlaylistItem::IDValue() const
{
    return m_dynplaylist->createdOn();
}


void
DynamicPlaylistItem::checkReparentHackNeeded( const DynamicPlaylistRevision& revision )
{
    // HACK HACK HACK  workaround for an ugly hack where we have to be compatible with older tomahawks (pre-0.1.0) that created dynplaylists as OnDemand then changed them to Static if they were static.
    //  we need to reparent this playlist to the correct category :-/.
    CategoryItem* cat = qobject_cast< CategoryItem* >( parent() );

//    qDebug() << "with category" << cat;
//    if( cat ) qDebug() << "and cat type:" << cat->categoryType();
    if( cat )
    {
        CategoryItem* from = cat;
        CategoryItem* to = 0;
        if( cat->categoryType() == SourcesModel::PlaylistsCategory && revision.mode == OnDemand ) { // WRONG
            SourceItem* col = qobject_cast< SourceItem* >( cat->parent() );
            to = col->stationsCategory();
            if( !to ) { // you have got to be fucking kidding me
                int fme = col->children().count();
                col->beginRowsAdded( fme, fme );
                to = new CategoryItem( model(), col, SourcesModel::StationsCategory, false );
                col->appendChild( to ); // we f'ing know it's not local b/c we're not getting into this mess ourselves
                col->endRowsAdded();
                col->setStationsCategory( to );
            }
        } else if( cat->categoryType() == SourcesModel::StationsCategory && revision.mode == Static ) { // WRONG
            SourceItem* col = qobject_cast< SourceItem* >( cat->parent() );
            to = col->playlistsCategory();
//            qDebug() << "TRYING TO HACK TO:" << to;
            if( !to ) { // you have got to be fucking kidding me
                int fme = col->children().count();
                col->beginRowsAdded( fme, fme );
                to = new CategoryItem( model(), col, SourcesModel::PlaylistsCategory, false );
                col->appendChild( to ); // we f'ing know it's not local b/c we're not getting into this mess ourselves
                col->endRowsAdded();
                col->setPlaylistsCategory( to );
            }
        }
        if( to ) {
//            qDebug() << "HACKING! moving dynamic playlist from" << from->text() << "to:" << to->text();
            // remove and add
            int idx = from->children().indexOf( this );
            from->beginRowsRemoved( idx, idx );
            from->removeChild( this );
            from->endRowsRemoved();

            idx = to->children().count();
            to->beginRowsAdded( idx, idx );
            to->appendChild( this );
            to->endRowsAdded();

            setParentItem( to );
        }
    }
}


dynplaylist_ptr
DynamicPlaylistItem::dynPlaylist() const
{
    return m_dynplaylist;
}


QString
DynamicPlaylistItem::text() const
{
    return m_dynplaylist->title();
}


bool
DynamicPlaylistItem::willAcceptDrag( const QMimeData* data ) const
{
    Q_UNUSED( data );
    return false;
}


QIcon
DynamicPlaylistItem::icon() const
{
    if ( m_dynplaylist->mode() == OnDemand )
    {
        return m_stationIcon;
    }
    else
    {
        return m_automaticPlaylistIcon;
    }
}

SourceTreeItem*
DynamicPlaylistItem::activateCurrent()
{
    if( ViewManager::instance()->pageForDynPlaylist( m_dynplaylist ) == ViewManager::instance()->currentPage() )
    {
        model()->linkSourceItemToPage( this, ViewManager::instance()->currentPage() );
        emit selectRequest( this );

        return this;
    }

    return 0;
}

bool
DynamicPlaylistItem::isBeingPlayed() const
{
    if ( ViewManager::instance()->pageForDynPlaylist( m_dynplaylist ) )
        return AudioEngine::instance()->currentTrackPlaylist() == ViewManager::instance()->pageForDynPlaylist( m_dynplaylist )->playlistInterface();
    return false;
}

